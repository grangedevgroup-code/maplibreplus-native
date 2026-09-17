#pragma once

#include <mln/renderer/change_request.hpp>
#include <mln/renderer/layer_group.hpp>
#include <mln/renderer/tile_render_target.hpp>
#include <mln/gfx/index_vector.hpp>
#include <mln/gfx/vertex_vector.hpp>
#include <mln/shaders/attributes.hpp>
#include <mln/shaders/segment.hpp>
#include <mln/tile/tile_id.hpp>

#include <map>
#include <memory>

namespace mln {

class PaintParameters;
class RenderOrchestrator;
class TransformState;

namespace gfx {
class Context;
class ShaderRegistry;
class ShaderProgramBase;
class UploadPass;
using ShaderProgramBasePtr = std::shared_ptr<ShaderProgramBase>;
} // namespace gfx

using GlobeLayoutVertex = gfx::Vertex<TypeList<attributes::pos>>;
using GlobeVertexVector = gfx::VertexVector<GlobeLayoutVertex>;
using GlobeIndexVector = gfx::IndexVector<gfx::Triangles>;

class RenderGlobe {
public:
    RenderGlobe();
    ~RenderGlobe();

    bool isRenderable() const;

    void update(gfx::ShaderRegistry&, gfx::Context&, const TransformState&, UniqueChangeRequestVec& changes);

    void teardown(UniqueChangeRequestVec& changes);

    void upload(gfx::UploadPass&);
    void updateUniforms(PaintParameters&);
    void render(RenderOrchestrator&, PaintParameters&);

private:
    void buildMesh();

    gfx::ShaderProgramBasePtr shader;
    LayerGroupBasePtr layerGroup;

    std::shared_ptr<GlobeVertexVector> sharedVertices;
    std::shared_ptr<GlobeIndexVector> sharedIndices;
    SegmentVector segments;

    std::map<OverscaledTileID, TileRenderTargetPtr> renderTargets;
};

} // namespace mln
