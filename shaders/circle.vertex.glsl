layout (location = 0) in vec2 a_pos;
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

#pragma mapbox: define highp vec4 color
#pragma mapbox: define mediump float radius
#pragma mapbox: define lowp float blur
#pragma mapbox: define lowp float opacity
#pragma mapbox: define highp vec4 stroke_color
#pragma mapbox: define mediump float stroke_width
#pragma mapbox: define lowp float stroke_opacity

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
    #pragma mapbox: initialize highp vec4 color
    #pragma mapbox: initialize mediump float radius
    #pragma mapbox: initialize lowp float blur
    #pragma mapbox: initialize lowp float opacity
    #pragma mapbox: initialize highp vec4 stroke_color
    #pragma mapbox: initialize mediump float stroke_width
    #pragma mapbox: initialize lowp float stroke_opacity

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
