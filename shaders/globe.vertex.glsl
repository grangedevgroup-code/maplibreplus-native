layout (location = 0) in vec2 a_pos;

layout (std140) uniform GlobeDrawableUBO {
    highp mat4 u_matrix;
    highp vec4 u_tile_mercator_coords;
    highp vec4 u_clipping_plane;
    highp float u_elevation;
    lowp float drawable_pad1;
    lowp float drawable_pad2;
    lowp float drawable_pad3;
};

out vec2 v_texture_pos;

void main() {
    vec2 mercator = u_tile_mercator_coords.xy + u_tile_mercator_coords.zw * a_pos;

    float spherical_x = mercator.x * PI * 2.0 + PI;
    float t = exp(PI - (mercator.y * PI * 2.0));
    float t2 = t * t;
    float denom = t2 + 1.0;
    float sin_sy = (t2 - 1.0) / denom;
    float cos_sy = (2.0 * t) / denom;

    vec3 sphere = vec3(
        sin(spherical_x) * cos_sy,
        sin_sy,
        cos(spherical_x) * cos_sy
    );

    v_texture_pos = a_pos / 8192.0;

    vec4 position = u_matrix * vec4(sphere * u_elevation, 1.0);
    position.z = (1.0 - (dot(sphere, u_clipping_plane.xyz) + u_clipping_plane.w)) * position.w;
    gl_Position = position;
}
