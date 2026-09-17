#include <mln/renderer/globe.hpp>

#include <mln/gfx/color_mode.hpp>
#include <mln/gfx/context.hpp>
#include <mln/gfx/cull_face_mode.hpp>
#include <mln/gfx/depth_mode.hpp>
#include <mln/gfx/drawable.hpp>
#include <mln/gfx/drawable_builder.hpp>
#include <mln/gfx/shader_registry.hpp>
#include <mln/gfx/upload_pass.hpp>
#include <mln/map/transform_state.hpp>
#include <mln/renderer/paint_parameters.hpp>
#include <mln/shaders/globe_layer_ubo.hpp>
#include <mln/shaders/shader_defines.hpp>
#include <mln/shaders/shader_program_base.hpp>
#include <mln/util/constants.hpp>
#include <mln/util/convert.hpp>
#include <mln/util/tile_cover.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <set>

namespace mln {

using namespace shaders;

namespace {

constexpr int32_t globeMeshSize = 32;
constexpr uint16_t globeTextureSize = util::tileSize_I;
constexpr uint8_t globeMaxZoom = 12;
constexpr std::size_t globeMaxTiles = 128;
constexpr auto globeShaderGroupName = "GlobeShader";

} // namespace

RenderGlobe::RenderGlobe() = default;

RenderGlobe::~RenderGlobe() = default;

bool RenderGlobe::isRenderable() const {
    return layerGroup && !layerGroup->empty();
}

void RenderGlobe::buildMesh() {
    if (sharedVertices) {
        return;
    }

    sharedVertices = std::make_shared<GlobeVertexVector>();
    sharedIndices = std::make_shared<GlobeIndexVector>();

    const double delta = static_cast<double>(util::EXTENT) / globeMeshSize;

    for (int32_t y = 0; y <= globeMeshSize; y++) {
        for (int32_t x = 0; x <= globeMeshSize; x++) {
            sharedVertices->emplace_back(GlobeLayoutVertex{
                {{static_cast<int16_t>(std::lround(x * delta)), static_cast<int16_t>(std::lround(y * delta))}}});
        }
    }

    const int32_t stride = globeMeshSize + 1;
    for (int32_t y = 0; y < globeMeshSize; y++) {
        for (int32_t x = 0; x < globeMeshSize; x++) {
            const int32_t i = y * stride + x;
            sharedIndices->emplace_back(static_cast<uint16_t>(i),
                                        static_cast<uint16_t>(i + stride),
                                        static_cast<uint16_t>(i + stride + 1));
            sharedIndices->emplace_back(static_cast<uint16_t>(i),
                                        static_cast<uint16_t>(i + stride + 1),
                                        static_cast<uint16_t>(i + 1));
        }
    }

    segments.clear();
    segments.emplace_back(0, 0, sharedVertices->elements(), sharedIndices->elements());
}

void RenderGlobe::update(gfx::ShaderRegistry& shaders,
                         gfx::Context& context,
                         const TransformState& state,
                         UniqueChangeRequestVec& changes) {
    if (!state.isGlobeRendering() || state.getSize().isEmpty()) {
        teardown(changes);
        return;
    }

    if (!shader) {
        shader = context.getGenericShader(shaders, globeShaderGroupName);
    }
    if (!shader) {
        teardown(changes);
        return;
    }

    buildMesh();

    if (!layerGroup) {
        auto layerGroup_ = context.createTileLayerGroup(
            std::numeric_limits<int32_t>::max() - 1, /*initialCapacity=*/64, "globe");
        if (!layerGroup_) {
            return;
        }
        layerGroup = std::move(layerGroup_);
    }

    auto* tileLayerGroup = static_cast<TileLayerGroup*>(layerGroup.get());

    const auto zoom = static_cast<uint8_t>(
        std::clamp(std::floor(state.getZoom()), 0.0, static_cast<double>(globeMaxZoom)));
    auto coveringTiles = util::globeTileCover(state, zoom, Range<uint8_t>{0, globeMaxZoom}, zoom);
    if (coveringTiles.size() > globeMaxTiles) {
        coveringTiles.erase(coveringTiles.begin() + static_cast<std::ptrdiff_t>(globeMaxTiles), coveringTiles.end());
    }

    const std::set<OverscaledTileID> visible(coveringTiles.begin(), coveringTiles.end());

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

    for (const auto& tileID : coveringTiles) {
        auto targetIt = renderTargets.find(tileID);
        if (targetIt == renderTargets.end()) {
            auto renderTarget = std::make_shared<TileRenderTarget>(context,
                                                                   Size{globeTextureSize, globeTextureSize},
                                                                   gfx::TextureChannelDataType::UnsignedByte,
                                                                   tileID.toUnwrapped());
            targetIt = renderTargets.emplace(tileID, std::move(renderTarget)).first;
            changes.emplace_back(std::make_unique<AddRenderTargetRequest>(targetIt->second));
        }

        if (tileLayerGroup->getDrawableCount(RenderPass::Opaque, tileID) > 0) {
            continue;
        }

        auto vertexAttrs = context.createVertexAttributeArray();
        if (const auto& attr = vertexAttrs->set(idGlobePosVertexAttribute)) {
            attr->setSharedRawData(sharedVertices,
                                   offsetof(GlobeLayoutVertex, a1),
                                   0,
                                   sizeof(GlobeLayoutVertex),
                                   gfx::AttributeDataType::Short2);
        }

        auto builder = context.createDrawableBuilder("globe");
        builder->setShader(shader);
        builder->setIs3D(true);
        builder->setEnableDepth(true);
        builder->setDepthType(gfx::DepthMaskType::ReadWrite);
        builder->setColorMode(gfx::ColorMode::unblended());
        builder->setCullFaceMode(gfx::CullFaceMode::disabled());
        builder->setRenderPass(RenderPass::Opaque);
        builder->setVertexAttributes(std::move(vertexAttrs));
        builder->setRawVertices({}, sharedVertices->elements(), gfx::AttributeDataType::Short2);
        builder->setSegments(gfx::Triangles(), sharedIndices, segments.data(), segments.size());
        builder->setTexture(targetIt->second->getTexture(), idGlobeImageTexture);
        builder->flush(context);

        for (auto& drawable : builder->clearDrawables()) {
            drawable->setTileID(tileID);
            tileLayerGroup->addDrawable(RenderPass::Opaque, tileID, std::move(drawable));
        }
    }
}

void RenderGlobe::teardown(UniqueChangeRequestVec& changes) {
    for (const auto& [tileID, renderTarget] : renderTargets) {
        changes.emplace_back(std::make_unique<RemoveRenderTargetRequest>(renderTarget));
    }
    renderTargets.clear();

    if (layerGroup) {
        layerGroup->clearDrawables();
    }
}

void RenderGlobe::upload(gfx::UploadPass& uploadPass) {
    if (layerGroup) {
        layerGroup->upload(uploadPass);
    }
}

void RenderGlobe::updateUniforms(PaintParameters& parameters) {
    if (!layerGroup) {
        return;
    }

    const auto& state = parameters.state;
    const auto matrix = util::cast<float>(state.getGlobeMatrix());
    const auto& plane = state.getGlobeClippingPlane();
    const std::array<float, 4> clippingPlane = {static_cast<float>(plane[0]),
                                                static_cast<float>(plane[1]),
                                                static_cast<float>(plane[2]),
                                                static_cast<float>(plane[3])};

    static_cast<TileLayerGroup*>(layerGroup.get())->visitDrawables([&](gfx::Drawable& drawable) {
        if (!drawable.getTileID()) {
            return;
        }
        const UnwrappedTileID tileID = drawable.getTileID()->toUnwrapped();
        const vec4 mercatorCoords = TransformState::getTileMercatorCoords(tileID);

        const GlobeDrawableUBO drawableUBO = {.matrix = matrix,
                                              .tile_mercator_coords = {static_cast<float>(mercatorCoords[0]),
                                                                       static_cast<float>(mercatorCoords[1]),
                                                                       static_cast<float>(mercatorCoords[2]),
                                                                       static_cast<float>(mercatorCoords[3])},
                                              .clipping_plane = clippingPlane,
                                              .elevation = 1.0f,
                                              .pad1 = 0,
                                              .pad2 = 0,
                                              .pad3 = 0};

        drawable.mutableUniformBuffers().createOrUpdate(idGlobeDrawableUBO, &drawableUBO, parameters.context);
    });
}

void RenderGlobe::render(RenderOrchestrator& orchestrator, PaintParameters& parameters) {
    if (layerGroup) {
        layerGroup->render(orchestrator, parameters);
    }
}

} // namespace mln
