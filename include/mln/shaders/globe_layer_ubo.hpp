#pragma once

#include <mln/shaders/layer_ubo.hpp>

namespace mln {
namespace shaders {

struct alignas(16) GlobeDrawableUBO {
    /*   0 */ std::array<float, 4 * 4> matrix;
    /*  64 */ std::array<float, 4> tile_mercator_coords;
    /*  80 */ std::array<float, 4> clipping_plane;
    /*  96 */ float elevation;
    /* 100 */ float pad1;
    /* 104 */ float pad2;
    /* 108 */ float pad3;
    /* 112 */
};
static_assert(sizeof(GlobeDrawableUBO) == 7 * 16);

} // namespace shaders
} // namespace mln
