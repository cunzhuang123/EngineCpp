
precision mediump float;

in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D u_firstTexture;
uniform sampler2D u_secondTexture;
uniform float u_time;

void main() {
    vec4 first = texture(u_firstTexture, v_texCoord);
    vec4 second = texture(u_secondTexture, v_texCoord);
    FragColor = first*(1.0 - u_time) + second*u_time;
}