precision mediump float;

// 输入顶点属性
in vec3 a_position;
in vec2 a_texCoord;

void main() {
    gl_Position = vec4(a_position, 1.0);
}