precision mediump float;

in vec2 v_texCoord;

out vec4 FragColor;

uniform sampler2D u_texture;

void main() {
    FragColor = texture(u_texture, v_texCoord);
}
