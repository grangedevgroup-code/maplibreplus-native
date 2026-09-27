#pragma once

#include <array>
#include <mln/gfx/texture2d.hpp>
#include <mln/renderer/change_request.hpp>
#include <mln/renderer/layer_group.hpp>
#include <mln/renderer/tile_render_target.hpp>
#include <mln/gfx/index_vector.hpp>
#include <mln/gfx/vertex_vector.hpp>
#include <mln/shaders/attributes.hpp>
#include <mln/shaders/segment.hpp>
#include <mln/util/geo.hpp>
#include <mln/style/terrain.hpp>
#include <mln/tile/tile_id.hpp>

#include <map>
#include <memory>
#include <string>

namespace mln {

class DEMData;
class PaintParameters;
class RenderOrchestrator;
class RenderSource;
class TransformState;

namespace gfx {
class Context;
class ShaderRegistry;
class ShaderProgramBase;
class UploadPass;
using ShaderProgramBasePtr = std::shared_ptr<ShaderProgramBase>;
} // namespace gfx

using TerrainLayoutVertex = gfx::Vertex<TypeList<attributes::pos3d>>;
using TerrainVertexVector = gfx::VertexVector<TerrainLayoutVertex>;
using TerrainIndexVector = gfx::IndexVector<gfx::Triangles>;

struct TerrainDemLookup {
    gfx::Texture2DPtr texture;
    std::array<float, 16> matrix{};
    std::array<float, 4> unpack{};
    float dim = 0.0f;
    float exaggeration = 1.0f;
    float elevation = 0.0f;
    float mode = 0.0f;
};

class RenderTerrain {
public:
    RenderTerrain();
    ~RenderTerrain();

    void setOptions(const style::Terrain&);
    const style::Terrain& getOptions() const { return options; }

    bool isRenderable() const;

    void update(gfx::ShaderRegistry&,
                gfx::Context&,
                const TransformState&,
                RenderSource* demSource,
                UniqueChangeRequestVec& changes);

    void teardown(UniqueChangeRequestVec& changes);

    void upload(gfx::UploadPass&);
    void updateUniforms(PaintParameters&);
    void render(RenderOrchestrator&, PaintParameters&);

    double getElevation(const LatLng&, double zoom) const;
    TerrainDemLookup demFor(const UnwrappedTileID&) const;
    double getMinElevation() const { return minElevation; }
    double getMaxElevation() const { return maxElevation; }

    static double getSkirtLength(double zoom);

private:
    void buildMesh();

    style::Terrain options;

    gfx::ShaderProgramBasePtr shader;
    LayerGroupBasePtr layerGroup;

    std::shared_ptr<TerrainVertexVector> sharedVertices;
    std::shared_ptr<TerrainIndexVector> sharedIndices;
    SegmentVector segments;

    std::map<OverscaledTileID, TileRenderTargetPtr> renderTargets;
    std::map<OverscaledTileID, std::shared_ptr<const DEMData>> demByTile;
    std::map<OverscaledTileID, gfx::Texture2DPtr> demTextures;

    double minElevation = 0;
    double maxElevation = 0;
};

} // namespace mln
