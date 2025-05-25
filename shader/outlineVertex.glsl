precision mediump float;

// 输入顶点属性
in vec3 a_position;
in vec2 a_texCoord;

// 统一变量
uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;

// 输出到片元着色器的变量
out vec2 v_texCoord;

void main() {
    gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * vec4(a_position, 1.0);
    v_texCoord = a_texCoord;
}