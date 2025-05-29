precision mediump float;

in vec2 v_texCoord;

out vec4 FragColor;

uniform sampler2D u_texture;
uniform vec4 u_color;

void main() {
    vec4 texColor = texture(u_texture, v_texCoord);
    FragColor = vec4(texColor.r * u_color.r, texColor.g * u_color.g, texColor.b * u_color.b, texColor.a) * u_color.a;
}
