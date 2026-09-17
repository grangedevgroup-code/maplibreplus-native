#include <mln/renderer/terrain.hpp>

#include <mln/geometry/dem_data.hpp>
#include <mln/gfx/context.hpp>
#include <mln/gfx/color_mode.hpp>
#include <mln/gfx/cull_face_mode.hpp>
#include <mln/gfx/depth_mode.hpp>
#include <mln/gfx/drawable.hpp>
#include <mln/gfx/drawable_builder.hpp>
#include <mln/gfx/shader_registry.hpp>
#include <mln/gfx/terrain_drawable_data.hpp>
#include <mln/gfx/texture2d.hpp>
#include <mln/gfx/upload_pass.hpp>
#include <mln/map/transform_state.hpp>
#include <mln/renderer/buckets/hillshade_bucket.hpp>
#include <mln/renderer/layer_tweaker.hpp>
#include <mln/renderer/paint_parameters.hpp>
#include <mln/renderer/render_source.hpp>
#include <mln/renderer/render_tile.hpp>
#include <mln/shaders/shader_program_base.hpp>
#include <mln/shaders/shader_defines.hpp>
#include <mln/shaders/terrain_layer_ubo.hpp>
#include <mln/style/types.hpp>
#include <mln/tile/raster_dem_tile.hpp>
#include <mln/tile/tile.hpp>
#include <mln/util/constants.hpp>
#include <mln/util/convert.hpp>
#include <mln/util/projection.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <set>

namespace mln {

using namespace shaders;

namespace {

constexpr int32_t terrainMeshSize = 128;
constexpr uint16_t terrainTextureSize = util::tileSize_I * 2;
constexpr auto terrainShaderGroupName = "TerrainShader";

int16_t clampToShort(double value) {
    return static_cast<int16_t>(std::clamp(value, -32768.0, 32767.0));
}

TerrainLayoutVertex terrainVertex(double x, double y, int16_t skirt) {
    return TerrainLayoutVertex{{{clampToShort(x), clampToShort(y), skirt}}};
}

mat4 demTileMatrix() {
    mat4 matrix = matrix::identity4();
    matrix::scale(matrix, matrix, 1.0 / util::EXTENT, 1.0 / util::EXTENT, 0.0);
    return matrix;
}

} // namespace

RenderTerrain::RenderTerrain() = default;

RenderTerrain::~RenderTerrain() = default;

void RenderTerrain::setOptions(const style::Terrain& options_) {
    options = options_;
}

bool RenderTerrain::isRenderable() const {
    return options.valid() && layerGroup && !layerGroup->empty();
}

double RenderTerrain::getSkirtLength(double zoom) {
    return 2.0 * M_PI * util::EARTH_RADIUS_M / std::pow(2.0, std::max(zoom, 0.0)) / 5.0;
}

void RenderTerrain::buildMesh() {
    if (sharedVertices) {
        return;
    }

    sharedVertices = std::make_shared<TerrainVertexVector>();
    sharedIndices = std::make_shared<TerrainIndexVector>();

    const double delta = static_cast<double>(util::EXTENT) / terrainMeshSize;

    for (int32_t y = 0; y <= terrainMeshSize; y++) {
        for (int32_t x = 0; x <= terrainMeshSize; x++) {
            sharedVertices->emplace_back(terrainVertex(x * delta, y * delta, 0));
        }
    }

    const int32_t meshSize2 = terrainMeshSize * terrainMeshSize;
    for (int32_t y = 0; y < meshSize2; y += terrainMeshSize + 1) {
        for (int32_t x = 0; x < terrainMeshSize; x++) {
            sharedIndices->emplace_back(static_cast<uint16_t>(x + y),
                                        static_cast<uint16_t>(terrainMeshSize + x + y + 1),
                                        static_cast<uint16_t>(terrainMeshSize + x + y + 2));
            sharedIndices->emplace_back(static_cast<uint16_t>(x + y),
                                        static_cast<uint16_t>(terrainMeshSize + x + y + 2),
                                        static_cast<uint16_t>(x + y + 1));
        }
    }

    const auto offsetTop = static_cast<int32_t>(sharedVertices->elements());
    const int32_t offsetTopEdge = 0;
    const int32_t offsetBottom = offsetTop + (terrainMeshSize + 1);
    const int32_t offsetBottomEdge = (terrainMeshSize + 1) * terrainMeshSize;

    for (int32_t x = 0; x <= terrainMeshSize; x++) {
        sharedVertices->emplace_back(terrainVertex(x * delta, 0.0, 1));
    }
    for (int32_t x = 0; x <= terrainMeshSize; x++) {
        sharedVertices->emplace_back(terrainVertex(x * delta, util::EXTENT, 1));
    }
    for (int32_t x = 0; x < terrainMeshSize; x++) {
        sharedIndices->emplace_back(static_cast<uint16_t>(offsetBottomEdge + x),
                                    static_cast<uint16_t>(offsetBottom + x),
                                    static_cast<uint16_t>(offsetBottom + x + 1));
        sharedIndices->emplace_back(static_cast<uint16_t>(offsetBottomEdge + x),
                                    static_cast<uint16_t>(offsetBottom + x + 1),
                                    static_cast<uint16_t>(offsetBottomEdge + x + 1));
        sharedIndices->emplace_back(static_cast<uint16_t>(offsetTopEdge + x),
                                    static_cast<uint16_t>(offsetTop + x + 1),
                                    static_cast<uint16_t>(offsetTop + x));
        sharedIndices->emplace_back(static_cast<uint16_t>(offsetTopEdge + x),
                                    static_cast<uint16_t>(offsetTopEdge + x + 1),
                                    static_cast<uint16_t>(offsetTop + x + 1));
    }

    const auto offsetLeft = static_cast<int32_t>(sharedVertices->elements());
    const int32_t offsetRight = offsetLeft + (terrainMeshSize + 1) * 2;
    for (int32_t x = 0; x < 2; x++) {
        for (int32_t y = 0; y <= terrainMeshSize; y++) {
            for (int16_t z = 0; z < 2; z++) {
                sharedVertices->emplace_back(terrainVertex(x * util::EXTENT, y * delta, z));
            }
        }
    }
    for (int32_t y = 0; y < terrainMeshSize * 2; y += 2) {
        sharedIndices->emplace_back(static_cast<uint16_t>(offsetLeft + y),
                                    static_cast<uint16_t>(offsetLeft + y + 1),
                                    static_cast<uint16_t>(offsetLeft + y + 3));
        sharedIndices->emplace_back(static_cast<uint16_t>(offsetLeft + y),
                                    static_cast<uint16_t>(offsetLeft + y + 3),
                                    static_cast<uint16_t>(offsetLeft + y + 2));
        sharedIndices->emplace_back(static_cast<uint16_t>(offsetRight + y),
                                    static_cast<uint16_t>(offsetRight + y + 3),
                                    static_cast<uint16_t>(offsetRight + y + 1));
        sharedIndices->emplace_back(static_cast<uint16_t>(offsetRight + y),
                                    static_cast<uint16_t>(offsetRight + y + 2),
                                    static_cast<uint16_t>(offsetRight + y + 3));
    }

    segments.clear();
    segments.emplace_back(0, 0, sharedVertices->elements(), sharedIndices->elements());
}

void RenderTerrain::update(gfx::ShaderRegistry& shaders,
                           gfx::Context& context,
                           const TransformState& state,
                           RenderSource* demSource,
                           UniqueChangeRequestVec& changes) {
    if (!options.valid() || !demSource) {
        teardown(changes);
        return;
    }

    const auto renderTiles = demSource->getRenderTiles();
    if (!renderTiles || renderTiles->empty()) {
        teardown(changes);
        return;
    }

    if (!shader) {
        shader = context.getGenericShader(shaders, terrainShaderGroupName);
    }
    if (!shader) {
        teardown(changes);
        return;
    }

    buildMesh();

    if (!layerGroup) {
        auto layerGroup_ = context.createTileLayerGroup(
            std::numeric_limits<int32_t>::max(), /*initialCapacity=*/64, "terrain");
        if (!layerGroup_) {
            return;
        }
        layerGroup = std::move(layerGroup_);
    }

    auto* tileLayerGroup = static_cast<TileLayerGroup*>(layerGroup.get());

    std::set<OverscaledTileID> visible;
    for (const RenderTile& tile : *renderTiles) {
        visible.insert(tile.getOverscaledTileID());
    }

    tileLayerGroup->removeDrawablesIf([&](gfx::Drawable& drawable) {
        return !drawable.getTileID() || !visible.contains(*drawable.getTileID());
    });

    for (auto it = renderTargets.begin(); it != renderTargets.end();) {
        if (visible.contains(it->first)) {
            ++it;
        } else {
            changes.emplace_back(std::make_unique<RemoveRenderTargetRequest>(it->second));
            it = renderTargets.erase(it);
        }
    }

    for (auto it = demByTile.begin(); it != demByTile.end();) {
        it = visible.contains(it->first) ? std::next(it) : demByTile.erase(it);
    }

    const float exaggeration = options.getExaggeration();
    const auto eleDelta = static_cast<float>(getSkirtLength(state.getZoom()));

    minElevation = 0;
    maxElevation = 0;

    for (const RenderTile& tile : *renderTiles) {
        const auto& tileID = tile.getOverscaledTileID();

        const Tile& tileData = tile.getTile();
        if (!tileData.isRenderable()) {
            continue;
        }

        auto* bucket = static_cast<const RasterDEMTile&>(tileData).getBucket();
        if (!bucket || !bucket->hasData()) {
            continue;
        }

        const DEMData& dem = bucket->getDEMData();
        if (!demByTile.contains(tileID)) {
            demByTile.emplace(tileID, std::make_shared<const DEMData>(dem));
        }

        auto targetIt = renderTargets.find(tileID);
        if (targetIt == renderTargets.end()) {
            auto renderTarget = std::make_shared<TileRenderTarget>(context,
                                                                     Size{terrainTextureSize, terrainTextureSize},
                                                                     gfx::TextureChannelDataType::UnsignedByte,
                                                                     tileID.toUnwrapped());
            targetIt = renderTargets.emplace(tileID, std::move(renderTarget)).first;
            changes.emplace_back(std::make_unique<AddRenderTargetRequest>(targetIt->second));
        }

        if (tileLayerGroup->getDrawableCount(RenderPass::Opaque, tileID) > 0) {
            continue;
        }

        auto vertexAttrs = context.createVertexAttributeArray();
        if (const auto& attr = vertexAttrs->set(idTerrainPosVertexAttribute)) {
            attr->setSharedRawData(sharedVertices,
                                   offsetof(TerrainLayoutVertex, a1),
                                   0,
                                   sizeof(TerrainLayoutVertex),
                                   gfx::AttributeDataType::Short3);
        }

        auto builder = context.createDrawableBuilder("terrain");
        builder->setShader(shader);
        builder->setIs3D(true);
        builder->setEnableDepth(true);
        builder->setDepthType(gfx::DepthMaskType::ReadWrite);
        builder->setColorMode(gfx::ColorMode::unblended());
        builder->setCullFaceMode(gfx::CullFaceMode::disabled());
        builder->setRenderPass(RenderPass::Opaque);
        builder->setVertexAttributes(std::move(vertexAttrs));
        builder->setRawVertices({}, sharedVertices->elements(), gfx::AttributeDataType::Short3);
        builder->setSegments(gfx::Triangles(), sharedIndices, segments.data(), segments.size());

        builder->setTexture(targetIt->second->getTexture(), idTerrainImageTexture);

        auto demTexture = context.createTexture2D();
        demTexture->setImage(dem.getImagePtr());
        demTexture->setSamplerConfiguration({.filter = gfx::TextureFilterType::Nearest,
                                             .wrapU = gfx::TextureWrapType::Clamp,
                                             .wrapV = gfx::TextureWrapType::Clamp});
        builder->setTexture(demTexture, idTerrainDemTexture);

        builder->flush(context);

        for (auto& drawable : builder->clearDrawables()) {
            drawable->setTileID(tileID);
            drawable->setData(std::make_unique<gfx::TerrainDrawableData>(
                dem.dim, dem.getUnpackVector(), exaggeration, eleDelta));
            tileLayerGroup->addDrawable(RenderPass::Opaque, tileID, std::move(drawable));
        }
    }

    for (const auto& [tileID, demData] : demByTile) {
        for (int32_t y = 0; y < demData->dim; y += 8) {
            for (int32_t x = 0; x < demData->dim; x += 8) {
                const double elevation = demData->get(x, y) * exaggeration;
                minElevation = std::min(minElevation, elevation);
                maxElevation = std::max(maxElevation, elevation);
            }
        }
    }
}

void RenderTerrain::teardown(UniqueChangeRequestVec& changes) {
    for (const auto& [tileID, renderTarget] : renderTargets) {
        changes.emplace_back(std::make_unique<RemoveRenderTargetRequest>(renderTarget));
    }
    renderTargets.clear();
    demByTile.clear();

    if (layerGroup) {
        layerGroup->clearDrawables();
    }
}

void RenderTerrain::upload(gfx::UploadPass& uploadPass) {
    if (layerGroup) {
        layerGroup->upload(uploadPass);
    }
}

void RenderTerrain::updateUniforms(PaintParameters& parameters) {
    if (!layerGroup) {
        return;
    }

    const mat4 terrainMatrix = demTileMatrix();

    static_cast<TileLayerGroup*>(layerGroup.get())->visitDrawables([&](gfx::Drawable& drawable) {
        if (!drawable.getTileID() || !drawable.getData()) {
            return;
        }
        const auto& data = static_cast<const gfx::TerrainDrawableData&>(*drawable.getData());
        const UnwrappedTileID tileID = drawable.getTileID()->toUnwrapped();
        const auto matrix = LayerTweaker::getTileMatrix(
            tileID, parameters, {0.f, 0.f}, style::TranslateAnchorType::Map, false, false, drawable);

        const TerrainDrawableUBO drawableUBO = {.matrix = util::cast<float>(matrix),
                                                .terrain_matrix = util::cast<float>(terrainMatrix),
                                                .terrain_unpack = data.unpack,
                                                .terrain_dim = static_cast<float>(data.dim),
                                                .terrain_exaggeration = data.exaggeration,
                                                .ele_delta = data.eleDelta,
                                                .pad1 = 0};

        drawable.mutableUniformBuffers().createOrUpdate(idTerrainDrawableUBO, &drawableUBO, parameters.context);
    });
}

void RenderTerrain::render(RenderOrchestrator& orchestrator, PaintParameters& parameters) {
    if (layerGroup) {
        layerGroup->render(orchestrator, parameters);
    }
}

double RenderTerrain::getElevation(const LatLng& latLng, double zoom) const {
    if (demByTile.empty()) {
        return 0.0;
    }

    const Point<double> mercator = Projection::project(latLng, 1.0) / util::tileSize_D;

    for (const auto& [tileID, demData] : demByTile) {
        const double tileScale = static_cast<double>(1ull << tileID.canonical.z);
        const double tileX = mercator.x * tileScale - tileID.canonical.x - tileID.wrap * tileScale;
        const double tileY = mercator.y * tileScale - tileID.canonical.y;
        if (tileX < 0.0 || tileX >= 1.0 || tileY < 0.0 || tileY >= 1.0) {
            continue;
        }

        const double px = tileX * demData->dim - 0.5;
        const double py = tileY * demData->dim - 0.5;
        const auto x0 = static_cast<int32_t>(std::floor(px));
        const auto y0 = static_cast<int32_t>(std::floor(py));
        const double fx = px - x0;
        const double fy = py - y0;

        const auto sample = [&](int32_t x, int32_t y) {
            return static_cast<double>(
                demData->get(std::clamp(x, -1, demData->dim), std::clamp(y, -1, demData->dim)));
        };

        const double top = sample(x0, y0) * (1.0 - fx) + sample(x0 + 1, y0) * fx;
        const double bottom = sample(x0, y0 + 1) * (1.0 - fx) + sample(x0 + 1, y0 + 1) * fx;
        return (top * (1.0 - fy) + bottom * fy) * options.getExaggeration();
    }

    (void)zoom;
    return 0.0;
}

} // namespace mln
