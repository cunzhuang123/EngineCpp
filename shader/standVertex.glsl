precision mediump float;

in vec3 a_position;
in vec2 a_texCoord;

uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform vec4 u_color;

out vec2 v_texCoord;
out vec4 v_color;

void main() {
    gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * vec4(a_position, 1.0);
    v_texCoord = a_texCoord;
    v_color = u_color;
}
