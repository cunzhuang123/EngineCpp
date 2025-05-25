precision mediump float;

in vec2 v_texCoord;
in vec4 v_color;

out vec4 FragColor;

uniform sampler2D u_texture;

void main() {
    vec4 texColor = texture(u_texture, v_texCoord);
    FragColor = vec4(texColor.r * v_color.r, texColor.g * v_color.g, texColor.b * v_color.b, texColor.a) * v_color.a;
}
