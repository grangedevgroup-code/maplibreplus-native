// Generated code, do not modify this file!
#pragma once
#include <mln/shaders/shader_source.hpp>

namespace mln {
namespace shaders {

template <>
struct ShaderSource<BuiltIn::CircleShader, gfx::Backend::Type::OpenGL> {
    static constexpr const char* name = "CircleShader";
    static constexpr const char* vertex = R"(layout (location = 0) in vec2 a_pos;
out vec3 v_data;

layout (std140) uniform GlobalPaintParamsUBO {
    highp vec2 u_pattern_atlas_texsize;
    highp vec2 u_units_to_pixels;
    highp vec2 u_world_size;
    highp float u_camera_to_center_distance;
    highp float u_symbol_fade_change;
    highp float u_aspect_ratio;
    highp float u_pixel_ratio;
    highp float u_map_zoom;
    lowp float global_pad1;
};

layout (std140) uniform CircleDrawableUBO {
    highp mat4 u_matrix;
    highp vec2 u_extrude_scale;
    // Interpolations
    lowp float u_color_t;
    lowp float u_radius_t;
    lowp float u_blur_t;
    lowp float u_opacity_t;
    lowp float u_stroke_color_t;
    lowp float u_stroke_width_t;
    lowp float u_stroke_opacity_t;
    lowp float drawable_pad1;
    lowp float drawable_pad2;
    lowp float drawable_pad3;

    highp mat4 u_terrain_matrix;
    highp vec4 u_terrain_unpack;
    highp float u_terrain_dim;
    highp float u_terrain_exaggeration;
    highp float u_terrain_elevation;
    highp float u_terrain_mode;
};

layout (std140) uniform CircleEvaluatedPropsUBO {
    highp vec4 u_color;
    highp vec4 u_stroke_color;
    mediump float u_radius;
    lowp float u_blur;
    lowp float u_opacity;
    mediump float u_stroke_width;
    lowp float u_stroke_opacity;
    bool u_scale_with_map;
    bool u_pitch_with_map;
    lowp float props_pad1;
};

#ifndef HAS_UNIFORM_u_color
layout (location = 1) in highp vec4 a_color;
out highp vec4 color;
#endif
#ifndef HAS_UNIFORM_u_radius
layout (location = 2) in mediump vec2 a_radius;
out mediump float radius;
#endif
#ifndef HAS_UNIFORM_u_blur
layout (location = 3) in lowp vec2 a_blur;
out lowp float blur;
#endif
#ifndef HAS_UNIFORM_u_opacity
layout (location = 4) in lowp vec2 a_opacity;
out lowp float opacity;
#endif
#ifndef HAS_UNIFORM_u_stroke_color
layout (location = 5) in highp vec4 a_stroke_color;
out highp vec4 stroke_color;
#endif
#ifndef HAS_UNIFORM_u_stroke_width
layout (location = 6) in mediump vec2 a_stroke_width;
out mediump float stroke_width;
#endif
#ifndef HAS_UNIFORM_u_stroke_opacity
layout (location = 7) in lowp vec2 a_stroke_opacity;
out lowp float stroke_opacity;
#endif

uniform sampler2D u_terrain_dem;

float terrain_texel_elevation(ivec2 pos) {
    vec4 rgb = (texelFetch(u_terrain_dem, pos, 0) * 255.0) * u_terrain_unpack;
    return rgb.r + rgb.g + rgb.b - u_terrain_unpack.a;
}

float terrain_elevation_at(vec2 pos) {
    if (u_terrain_mode < 0.5) {
        return 0.0;
    }
    if (u_terrain_mode < 1.5) {
        return u_terrain_elevation;
    }
    vec2 coord = (u_terrain_matrix * vec4(pos, 0.0, 1.0)).xy * u_terrain_dim + 0.5;
    vec2 f = fract(coord);
    ivec2 c = ivec2(floor(coord));
    ivec2 hi = textureSize(u_terrain_dem, 0) - 1;
    float tl = terrain_texel_elevation(clamp(c, ivec2(0), hi));
    float tr = terrain_texel_elevation(clamp(c + ivec2(1, 0), ivec2(0), hi));
    float bl = terrain_texel_elevation(clamp(c + ivec2(0, 1), ivec2(0), hi));
    float br = terrain_texel_elevation(clamp(c + ivec2(1, 1), ivec2(0), hi));
    return mix(mix(tl, tr, f.x), mix(bl, br, f.x), f.y) * u_terrain_exaggeration;
}

vec4 terrain_raise(vec4 position, vec2 pos) {
    if (u_terrain_mode < 0.5) {
        return position;
    }
    float elevation = terrain_elevation_at(pos);
    vec4 ground = u_matrix * vec4(pos, 0.0, 1.0);
    vec4 raised = u_matrix * vec4(pos, elevation, 1.0);
    if (ground.w <= 0.0 || raised.w <= 0.0) {
        return vec4(-2.0, -2.0, -2.0, 1.0);
    }
    vec2 shift = raised.xy / raised.w - ground.xy / ground.w;
    if (dot(shift, shift) > 4.0) {
        return vec4(-2.0, -2.0, -2.0, 1.0);
    }
    position.xy += shift * position.w;
    return position;
}

void main(void) {
    #ifndef HAS_UNIFORM_u_color
color = unpack_mix_color(a_color, u_color_t);
#else
highp vec4 color = u_color;
#endif
    #ifndef HAS_UNIFORM_u_radius
radius = unpack_mix_vec2(a_radius, u_radius_t);
#else
mediump float radius = u_radius;
#endif
    #ifndef HAS_UNIFORM_u_blur
blur = unpack_mix_vec2(a_blur, u_blur_t);
#else
lowp float blur = u_blur;
#endif
    #ifndef HAS_UNIFORM_u_opacity
opacity = unpack_mix_vec2(a_opacity, u_opacity_t);
#else
lowp float opacity = u_opacity;
#endif
    #ifndef HAS_UNIFORM_u_stroke_color
stroke_color = unpack_mix_color(a_stroke_color, u_stroke_color_t);
#else
highp vec4 stroke_color = u_stroke_color;
#endif
    #ifndef HAS_UNIFORM_u_stroke_width
stroke_width = unpack_mix_vec2(a_stroke_width, u_stroke_width_t);
#else
mediump float stroke_width = u_stroke_width;
#endif
    #ifndef HAS_UNIFORM_u_stroke_opacity
stroke_opacity = unpack_mix_vec2(a_stroke_opacity, u_stroke_opacity_t);
#else
lowp float stroke_opacity = u_stroke_opacity;
#endif

    // unencode the extrusion vector that we snuck into the a_pos vector
    vec2 extrude = vec2(mod(a_pos, 2.0) * 2.0 - 1.0);

    // multiply a_pos by 0.5, since we had it * 2 in order to sneak
    // in extrusion data
    vec2 circle_center = floor(a_pos * 0.5);
    if (u_pitch_with_map) {
        vec2 corner_position = circle_center;
        if (u_scale_with_map) {
            corner_position += extrude * (radius + stroke_width) * u_extrude_scale;
        } else {
            // Pitching the circle with the map effectively scales it with the map
            // To counteract the effect for pitch-scale: viewport, we rescale the
            // whole circle based on the pitch scaling effect at its central point
            vec4 projected_center = u_matrix * vec4(circle_center, 0, 1);
            corner_position += extrude * (radius + stroke_width) * u_extrude_scale * (projected_center.w / u_camera_to_center_distance);
        }

        gl_Position = u_matrix * vec4(corner_position, 0, 1);
    } else {
        gl_Position = u_matrix * vec4(circle_center, 0, 1);

        if (u_scale_with_map) {
            gl_Position.xy += extrude * (radius + stroke_width) * u_extrude_scale * u_camera_to_center_distance;
        } else {
            gl_Position.xy += extrude * (radius + stroke_width) * u_extrude_scale * gl_Position.w;
        }
    }

    gl_Position = terrain_raise(gl_Position, circle_center);

    // This is a minimum blur distance that serves as a faux-antialiasing for
    // the circle. since blur is a ratio of the circle's size and the intent is
    // to keep the blur at roughly 1px, the two are inversely related.
    lowp float antialiasblur = 1.0 / DEVICE_PIXEL_RATIO / (radius + stroke_width);

    v_data = vec3(extrude.x, extrude.y, antialiasblur);
}
)";
    static constexpr const char* fragment = R"(in vec3 v_data;

layout (std140) uniform CircleEvaluatedPropsUBO {
    highp vec4 u_color;
    highp vec4 u_stroke_color;
    mediump float u_radius;
    lowp float u_blur;
    lowp float u_opacity;
    mediump float u_stroke_width;
    lowp float u_stroke_opacity;
    bool u_scale_with_map;
    bool u_pitch_with_map;
    lowp float props_pad1;
};

#ifndef HAS_UNIFORM_u_color
in highp vec4 color;
#endif
#ifndef HAS_UNIFORM_u_radius
in mediump float radius;
#endif
#ifndef HAS_UNIFORM_u_blur
in lowp float blur;
#endif
#ifndef HAS_UNIFORM_u_opacity
in lowp float opacity;
#endif
#ifndef HAS_UNIFORM_u_stroke_color
in highp vec4 stroke_color;
#endif
#ifndef HAS_UNIFORM_u_stroke_width
in mediump float stroke_width;
#endif
#ifndef HAS_UNIFORM_u_stroke_opacity
in lowp float stroke_opacity;
#endif

void main() {
    #ifdef HAS_UNIFORM_u_color
highp vec4 color = u_color;
#endif
    #ifdef HAS_UNIFORM_u_radius
mediump float radius = u_radius;
#endif
    #ifdef HAS_UNIFORM_u_blur
lowp float blur = u_blur;
#endif
    #ifdef HAS_UNIFORM_u_opacity
lowp float opacity = u_opacity;
#endif
    #ifdef HAS_UNIFORM_u_stroke_color
highp vec4 stroke_color = u_stroke_color;
#endif
    #ifdef HAS_UNIFORM_u_stroke_width
mediump float stroke_width = u_stroke_width;
#endif
    #ifdef HAS_UNIFORM_u_stroke_opacity
lowp float stroke_opacity = u_stroke_opacity;
#endif

    vec2 extrude = v_data.xy;
    float extrude_length = length(extrude);

    lowp float antialiasblur = v_data.z;
    float antialiased_blur = -max(blur, antialiasblur);

    float opacity_t = smoothstep(0.0, antialiased_blur, extrude_length - 1.0);

    float color_t = stroke_width < 0.01 ? 0.0 : smoothstep(
        antialiased_blur,
        0.0,
        extrude_length - radius / (radius + stroke_width)
    );

    fragColor = opacity_t * mix(color * opacity, stroke_color * stroke_opacity, color_t);

#ifdef OVERDRAW_INSPECTOR
    fragColor = vec4(1.0);
#endif
}
)";
};

} // namespace shaders
} // namespace mln
