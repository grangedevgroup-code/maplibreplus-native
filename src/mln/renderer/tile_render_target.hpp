#pragma once

#include <mln/renderer/render_target.hpp>
#include <mln/tile/tile_id.hpp>
#include <mln/util/mat4.hpp>

namespace mln {

class TileRenderTarget final : public RenderTarget {
public:
    TileRenderTarget(gfx::Context& context, Size size, gfx::TextureChannelDataType type, UnwrappedTileID tileID);
    ~TileRenderTarget() override;

    const UnwrappedTileID& getTileID() const { return tileID; }

    void upload(gfx::UploadPass&) override;
    void render(RenderOrchestrator&, const RenderTree&, PaintParameters&) override;

private:
    mat4 tileProjMatrix(const PaintParameters&) const;

    UnwrappedTileID tileID;
};

using TileRenderTargetPtr = std::shared_ptr<TileRenderTarget>;

} // namespace mln
