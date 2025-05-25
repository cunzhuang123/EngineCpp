precision mediump float;

out vec4 FragColor;

uniform float angleDeg;   // 0–360 旋转角（循环）
uniform vec2  size;       // 画布像素尺寸

const vec3 BG_COLOR = vec3(0.1412, 0.2, 0.251);
const float BORDER = 10.0;  // 边框宽度
const vec3 COLOR_A  = vec3(0.20, 0.60, 0.95);
const vec3 COLOR_B  = vec3(0.5804, 0.9843, 0.9373);
const float PI      = 3.14159265359;

void main()
{
    /* 1. 把像素映射到以屏幕中心为原点、短边为 1 的坐标系 */
    vec2 frag = gl_FragCoord.xy;
    if (frag.x < BORDER || frag.y < BORDER || frag.x > (size.x - BORDER) || frag.y > (size.y - BORDER))
    {
        FragColor = vec4(0.3686, 0.3216, 0.1922, 1.0);
        return;
    }

    float s = min(size.x, size.y);
    vec2 uv = (frag - 0.5 * size) / s;   // 大约 [-0.5,0.5]

    /* 2. 环形遮罩（6 像素厚） */
    float r      = length(uv);
    float ring   = step(0.28, r) - step(0.34, r);  // 0.28~0.34 之间为 1，其余 0

    /* 3. 角度判断（带环绕） */
    float theta  = atan(uv.y, uv.x);                // [-π,π]
    theta = (theta < 0.0) ? theta + 2.0*PI : theta;

    float start  = radians(mod(angleDeg, 360.0));
    float arcLen = PI * 0.8;                        // 144°

    /* 把 (theta - start) 放到 [0,2π) 再比较 */
    float diff   = mod(theta - start + 2.0*PI, 2.0*PI);
    float inArc  = step(diff, arcLen);              // 在扫角内为 1

    /* 4. 颜色 */
    float t      = clamp(diff / arcLen, 0.0, 1.0);  // 0→1 渐变
    vec3 ringClr = mix(COLOR_A, COLOR_B, t);

    vec3 color = BG_COLOR;
    color = mix(color, ringClr, ring * inArc);

    FragColor = vec4(color, 1.0);
}