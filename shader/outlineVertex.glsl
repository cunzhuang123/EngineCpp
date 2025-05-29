precision highp  float;

// 输入顶点属性
in vec3 a_position;
in vec2 a_texCoord;

// 输出到片元着色器的变量
out vec2 v_texCoord;
void main() {
    gl_Position = vec4(a_position, 1.0);
    v_texCoord = a_texCoord;
}