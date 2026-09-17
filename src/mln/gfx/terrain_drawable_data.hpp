#pragma once

#include <mln/gfx/drawable_data.hpp>
#include <mln/util/tileset.hpp>

#include <array>
#include <memory>

namespace mln {
namespace gfx {

class TerrainDrawableData : public DrawableData {
public:
    TerrainDrawableData(int32_t dim_, std::array<float, 4> unpack_, float exaggeration_, float eleDelta_)
        : dim(dim_),
          unpack(unpack_),
          exaggeration(exaggeration_),
          eleDelta(eleDelta_) {}

    int32_t dim;
    std::array<float, 4> unpack;
    float exaggeration;
    float eleDelta;
};

using UniqueTerrainDrawableData = std::unique_ptr<TerrainDrawableData>;

} // namespace gfx
} // namespace mln
