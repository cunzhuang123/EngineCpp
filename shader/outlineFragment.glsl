

precision mediump float;

// 来自顶点着色器的纹理坐标
in vec2 v_texCoord;

// 输出到帧缓冲的颜色
out vec4 FragColor;

// 原纹理，Alpha 通道需要包含文字形状
uniform sampler2D u_texture;
uniform vec2 u_texResolution;  


// 采样方向（这里用12个方向，分布在 360° 上）
// const int NUM_DIRECTIONS = 12;
// const vec2 directions[NUM_DIRECTIONS] = vec2[](
//     vec2( 1.0,  0.0),
//     vec2( 0.866,  0.5),
//     vec2( 0.5,   0.866),
//     vec2( 0.0,   1.0),
//     vec2(-0.5,   0.866),
//     vec2(-0.866, 0.5),
//     vec2(-1.0,   0.0),
//     vec2(-0.866, -0.5),
//     vec2(-0.5,  -0.866),
//     vec2( 0.0,  -1.0),
//     vec2( 0.5,  -0.866),
//     vec2( 0.866, -0.5)
// );

// const int NUM_DIRECTIONS = 24;
// const vec2 directions[NUM_DIRECTIONS] = vec2[](
//     vec2( 1.0000,  0.0000),  //   0°
//     vec2( 0.9659,  0.2588),  //  15°
//     vec2( 0.8660,  0.5000),  //  30°
//     vec2( 0.7071,  0.7071),  //  45°
//     vec2( 0.5000,  0.8660),  //  60°
//     vec2( 0.2588,  0.9659),  //  75°
//     vec2( 0.0000,  1.0000),  //  90°
//     vec2(-0.2588,  0.9659),  // 105°
//     vec2(-0.5000,  0.8660),  // 120°
//     vec2(-0.7071,  0.7071),  // 135°
//     vec2(-0.8660,  0.5000),  // 150°
//     vec2(-0.9659,  0.2588),  // 165°
//     vec2(-1.0000,  0.0000),  // 180°
//     vec2(-0.9659, -0.2588),  // 195°
//     vec2(-0.8660, -0.5000),  // 210°
//     vec2(-0.7071, -0.7071),  // 225°
//     vec2(-0.5000, -0.8660),  // 240°
//     vec2(-0.2588, -0.9659),  // 255°
//     vec2( 0.0000, -1.0000),  // 270°
//     vec2( 0.2588, -0.9659),  // 285°
//     vec2( 0.5000, -0.8660),  // 300°
//     vec2( 0.7071, -0.7071),  // 315°
//     vec2( 0.8660, -0.5000),  // 330°
//     vec2( 0.9659, -0.2588)   // 345°
// );


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


// 为了近似计算“对于当前像素与边缘的最近距离”，
// 我们在每个方向按一定步进搜索。STEPS 越大精度越高，但开销也更大。
// const int STEPS = 16;

void main() 
{
        // 描边颜色
    vec4 u_outlineColor = vec4(1.0, 1.0, 1.0, 1.0);
    // 描边宽度（单位：像素）
    // float u_outlineWidth = 30.0;  
    // 纹理分辨率，用于将“像素”转换成UV步长，比如 (1.0/width, 1.0/height)
    int STEPS = 30;
    float u_outlineWidth = float(STEPS);
    // 读取原纹理颜色
    vec4 srcColor = texture(u_texture, v_texCoord);

    // 当前像素的 Alpha
    float alphaCenter = srcColor.a;

    if (alphaCenter > 0.999)
    {
        FragColor = srcColor;
        return;
    }

    // 如果它本身在文字内部 (alpha ~ 1.0) 或外部 (alpha ~ 0.0)，
    // 我们需要找它“到文字边缘”的距离。 
    // 如果 alpha 在 0~1 之间，本身就是半透明边缘，也参与同样的逻辑。

    float minDist = 999999.0; // 用来记录搜索到的最短距离

    // 在多个方向上搜索轮廓
    for(int i = 0; i < NUM_DIRECTIONS; i++)
    {
        vec2 dir = normalize(directions[i]);
        // 每次向外“走” stepLen 像素，最多走 STEPS 步
        // 注意：这里 stepLen 先设为1像素对应的 UV 长度
        float stepLen = 1.0;

        // 逐步走
        float dist = 0.0; 
        float prevAlpha = alphaCenter;
        for(int s = 0; s < STEPS; s++)
        {
            dist += stepLen;
            // 换算成UV偏移
            vec2 offsetUV = dir * dist / u_texResolution; 
            vec2 sampleUV = v_texCoord + offsetUV;

            // 边界检查，防止越过纹理范围
            if(sampleUV.x<0.0 || sampleUV.x>1.0 ||
               sampleUV.y<0.0 || sampleUV.y>1.0)
            {
                // 超出纹理边界就停止
                break;
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
    }

    // 如果找不到任何边缘，minDist 还是巨大值，说明在文本很里面或很外面
    // 这时可视具体需求处理：
    //  - 在文字最内部: 距离可以近似为0
    //  - 在文字最外部: 距离可能很大
    // 这里粗略处理：若 alphaCenter>0.5 就视为距边缘=0，若 alphaCenter<0.5 就视为∞
    // 但为了更自然，你也可以把 minDist 设置为 0（表示已经是文字内）或一个大值
    if(minDist > 999998.0) // 说明没有撞到任何边缘
    {
        if(alphaCenter > 0.5)
        {
            minDist = 0.0; // 在字形内部
            // FragColor = srcColor;
        }
        else
        {
            minDist = 999999.0; // 在字形外部
            // FragColor = u_outlineColor;
        }
        FragColor = srcColor;
    }
    else {
        FragColor = u_outlineColor;
    }



    // if(minDist > 999998.0) // 说明没有撞到任何边缘
    // {
    //     if(alphaCenter > 0.5)
    //         minDist = 0.0; // 在字形内部
    //     else
    //         minDist = 999999.0; // 在字形外部

    //     FragColor = srcColor;
    //     return;
    // }

    // // 比对描边宽度
    // // 如果距离 < u_outlineWidth，则表示在“外描边”范围内
    // // 用一个smoothstep让边缘更平滑
    // float halfSmooth = 2.0;//u_outlineWidth/10.0; // 让描边边缘有1像素的过渡
    // float outlineFactor = 1.0 - smoothstep(u_outlineWidth - halfSmooth,
    //                                       u_outlineWidth + halfSmooth,
    //                                       minDist);

    // // 我们希望只在“文字的外部区域”显示描边
    // // 也就是 当原像素 alpha < 0.5 且 outlineFactor>0 时，可以显示描边
    // // 请按实际需求调整，如果要“文字内部描边”，逻辑会不同
    // float isOutside = step(alphaCenter, 0.5);  
    // float finalOutline = outlineFactor * isOutside;

    // // 组合一下：如果 finalOutline>0，用描边颜色；否则用原像素颜色
    // float mixFactor = clamp(finalOutline, 0.0, 1.0);
    // vec3 finalRGB   = mix(srcColor.rgb, u_outlineColor.rgb, mixFactor);
    // float finalAlpha= max(srcColor.a, mixFactor * u_outlineColor.a);

    // if (alphaCenter > 0.0)
    // {
    //     FragColor = srcColor;
    // }
    // else{
    //     FragColor = vec4(finalRGB, finalAlpha);
    // }
}
