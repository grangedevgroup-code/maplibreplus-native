uniform sampler2D u_terrain_image;

in vec2 v_texture_pos;

void main() {
    fragColor = texture(u_terrain_image, vec2(v_texture_pos.x, 1.0 - v_texture_pos.y));
#ifdef OVERDRAW_INSPECTOR
    fragColor = vec4(1.0);
#endif
}
