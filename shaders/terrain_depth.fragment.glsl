in float v_depth;

const highp vec4 bitSh = vec4(256.0 * 256.0 * 256.0, 256.0 * 256.0, 256.0, 1.0);
const highp vec4 bitMsk = vec4(0.0, vec3(1.0 / 256.0));

void main() {
    highp vec4 comp = fract(v_depth * bitSh);
    comp -= comp.xxyz * bitMsk;
    fragColor = comp;
}
