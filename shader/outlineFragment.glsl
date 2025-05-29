

precision highp  float;

// 来自顶点着色器的纹理坐标
in vec2 v_texCoord;
// 输出到帧缓冲的颜色
out vec4 FragColor;

// 原纹理，Alpha 通道需要包含文字形状
uniform sampler2D u_texture;
uniform vec2 u_texResolution;  
uniform int u_steps;
uniform vec4 u_outlineColor;

uniform vec4 u_uvTransform;
// uniform vec2 u_outlineOffset;

// 采样方向（48个方向，间隔7.5°）
const int NUM_DIRECTIONS = 48;
const vec2 directions[NUM_DIRECTIONS] = vec2[](
    vec2( 1.0000,  0.0000),  //   0°
    vec2( 0.9914,  0.1305),  //   7.5°
    vec2( 0.9659,  0.2588),  //  15°
    vec2( 0.9239,  0.3827),  //  22.5°
    vec2( 0.8660,  0.5000),  //  30°
    vec2( 0.7880,  0.6157),  //  37.5°
    vec2( 0.7071,  0.7071),  //  45°
    vec2( 0.6157,  0.7880),  //  52.5°
    vec2( 0.5000,  0.8660),  //  60°
    vec2( 0.3827,  0.9239),  //  67.5°
    vec2( 0.2588,  0.9659),  //  75°
    vec2( 0.1305,  0.9914),  //  82.5°
    vec2( 0.0000,  1.0000),  //  90°
    vec2(-0.1305,  0.9914),  //  97.5°
    vec2(-0.2588,  0.9659),  // 105°
    vec2(-0.3827,  0.9239),  // 112.5°
    vec2(-0.5000,  0.8660),  // 120°
    vec2(-0.6157,  0.7880),  // 127.5°
    vec2(-0.7071,  0.7071),  // 135°
    vec2(-0.7880,  0.6157),  // 142.5°
    vec2(-0.8660,  0.5000),  // 150°
    vec2(-0.9239,  0.3827),  // 157.5°
    vec2(-0.9659,  0.2588),  // 165°
    vec2(-0.9914,  0.1305),  // 172.5°
    vec2(-1.0000,  0.0000),  // 180°
    vec2(-0.9914, -0.1305),  // 187.5°
    vec2(-0.9659, -0.2588),  // 195°
    vec2(-0.9239, -0.3827),  // 202.5°
    vec2(-0.8660, -0.5000),  // 210°
    vec2(-0.7880, -0.6157),  // 217.5°
    vec2(-0.7071, -0.7071),  // 225°
    vec2(-0.6157, -0.7880),  // 232.5°
    vec2(-0.5000, -0.8660),  // 240°
    vec2(-0.3827, -0.9239),  // 247.5°
    vec2(-0.2588, -0.9659),  // 255°
    vec2(-0.1305, -0.9914),  // 262.5°
    vec2( 0.0000, -1.0000),  // 270°
    vec2( 0.1305, -0.9914),  // 277.5°
    vec2( 0.2588, -0.9659),  // 285°
    vec2( 0.3827, -0.9239),  // 292.5°
    vec2( 0.5000, -0.8660),  // 300°
    vec2( 0.6157, -0.7880),  // 307.5°
    vec2( 0.7071, -0.7071),  // 315°
    vec2( 0.7880, -0.6157),  // 322.5°
    vec2( 0.8660, -0.5000),  // 330°
    vec2( 0.9239, -0.3827),  // 337.5°
    vec2( 0.9659, -0.2588),  // 345°
    vec2( 0.9914, -0.1305)   // 352.5°
);


void main() 
{
    

    vec2 transformedTexCoord = (v_texCoord) * u_uvTransform.zw + u_uvTransform.xy;


    vec4 srcColor;
    if (transformedTexCoord.x < 0.0 || transformedTexCoord.y < 0.0 || transformedTexCoord.x > 1.0 || transformedTexCoord.y > 1.0)
    {
        srcColor = vec4(0.0, 0.0, 0.0, 0.0);
        // FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        // return;
    }
    else 
    {
        srcColor = texture(u_texture, transformedTexCoord);

        // FragColor = vec4(transformedTexCoord, 0.0, 1.0);
        // return;

    }

    // 当前像素的 Alpha
    float alphaCenter = srcColor.a;

    if (alphaCenter > 0.01)
    {
        FragColor = u_outlineColor;
    }
    else 
    {
        float minDist = 999999.0; // 用来记录搜索到的最短距离

        float distAdded = 0.0; // 用来记录当前像素的轮廓宽度
        float addCount = 0.0;

        // 在多个方向上搜索轮廓
        for(int i = 0; i < NUM_DIRECTIONS; i++)
        {
            vec2 dir = normalize(directions[i]);
            // 每次向外“走” stepLen 像素，最多走 u_steps 步
            // 注意：这里 stepLen 先设为1像素对应的 UV 长度
            float stepLen = 1.0;

            // 逐步走
            float dist = 0.0; 
            float prevAlpha = alphaCenter;
            for(int s = 0; s < u_steps; s++)
            {
                dist += stepLen;
                // 换算成UV偏移
                vec2 offsetUV = dir * dist / u_texResolution; 

                // // 边界检查，防止越过纹理范围
                // if(sampleUV.x<0.0 || sampleUV.x>1.0 ||
                //    sampleUV.y<0.0 || sampleUV.y>1.0)
                // {
                //     // 超出纹理边界就停止
                //     break;
                // }

                vec2 sampleUV = transformedTexCoord + offsetUV;
                if(sampleUV.x<0.0 || sampleUV.x>1.0 ||
                sampleUV.y<0.0 || sampleUV.y>1.0)
                {
                    // 超出纹理边界就停止
                    // break;
                    continue;
                }

                float a = texture(u_texture, sampleUV).a;

                // 一旦检测到 alpha 与 prevAlpha “跨过阈值”(从<0.5到>0.5或者相反)
                // 就认为碰到边缘
                // 这里简单判断：只要 a 和 prevAlpha 处于不同侧，就算入边缘
                bool crossed = (a>0.5 && prevAlpha<0.5) || (a<0.5 && prevAlpha>0.5);
                if(crossed)
                {
                    // 记录最小距离
                    if(dist < minDist)
                    {
                        minDist = dist;
                    }
                    break;
                }
                prevAlpha = a;
            }

            if(dist < (float(u_steps) - 0.1))
            {

                distAdded += dist;
                addCount += 1.0;
            }
        }
        if(minDist > 999998.0) // 说明没有撞到任何边缘
        {
            FragColor = srcColor;
        }
        else {

            float alpha = minDist >= float(u_steps - 3) ? (1.0 - (minDist - float(u_steps -3))/2.0) : 1.0;
            FragColor = vec4(u_outlineColor.rgb, alpha);
        }
    }
}



