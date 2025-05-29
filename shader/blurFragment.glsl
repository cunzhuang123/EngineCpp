precision mediump float;

in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D u_texture;
uniform float u_blurDegree;

// 高斯函数
float gaussian(float x, float sigma) {
    return exp(-(x * x) / (2.0 * sigma * sigma)) / (2.0 * 3.1415926535897932384626433832795 * sigma * sigma);
}

void main() {
    float sigma = u_blurDegree;
    int radius = int(ceil(sigma * 3.0));
    
    float total = 0.0;
    vec4 color = vec4(0.0);
    
    float texelOffset = 1.0 / 512.0;
    
    for(int i = -20; i <= 20; i++) {
        if(i >= -radius && i <= radius) {
            float weight = gaussian(float(i), sigma);
            color += texture(u_texture, v_texCoord + vec2(float(i) * texelOffset, 0.0)) * weight;
            total += weight;
        }
    }
    
    FragColor = color / total;
}