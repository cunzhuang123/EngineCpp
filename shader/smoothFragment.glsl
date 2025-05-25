

// precision mediump float;

// in vec2 v_texCoord;
// out vec4 FragColor;

// uniform sampler2D u_texture;   // 上一帧渲染好的 Alpha 纹理
// uniform int u_op;  // 0: 膨胀, 1: 腐蚀

// // 3x3 采样偏移
// vec2 offsets[9] = vec2[](
//     vec2(-1.0, -1.0),
//     vec2( 0.0, -1.0),
//     vec2( 1.0, -1.0),
//     vec2(-1.0,  0.0),
//     vec2( 0.0,  0.0),
//     vec2( 1.0,  0.0),
//     vec2(-1.0,  1.0),
//     vec2( 0.0,  1.0),
//     vec2( 1.0,  1.0)
// );

// void main()
// {
//     vec2 u_texSize = vec2(1.0/1080.0, 1.0/540.0);

//     vec4 col = texture(u_texture, v_texCoord);
//     float centerAlpha = col.a;
//     // int u_op = 1;  // 0: 膨胀, 1: 腐蚀

//     // 根据 op 判断是膨胀还是腐蚀
//     float result = (u_op == 0) ? 0.0 : 1.0;  // 膨胀初始化=0, 腐蚀初始化=1
    
//     for(int i = 0; i < 9; i++)
//     {
//         vec2 sampleUV = v_texCoord + offsets[i] * u_texSize;
//         float sampleA  = texture(u_texture, sampleUV).a;

//         if (u_op == 0) {
//             // 膨胀：取邻域最大值
//             result = max(result, sampleA);
//         } else {
//             // 腐蚀：取邻域最小值
//             result = min(result, sampleA);
//         }
//     }

//     if (centerAlpha > 0.0 && result == 0.0) {
//         FragColor = col;// vec4(0.0, 0.0, 0.0, 0.0);
//     }
//     else if (centerAlpha == 0.0 && result > 0.0) {
//         FragColor = vec4(1.0, 1.0, 1.0, result);
//         // FragColor = col;
//     }
//     else{
//         FragColor = col;
//     }
// }


precision mediump float;

in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D u_texture;         // 输入的Alpha纹理

void main()
{
    vec2 u_texelSize = vec2(1.0 / 1920.0, 1.0 / 1080.0);
    float u_cornerThreshold = 0.01;// 判断凹凸角的阈值
    vec4 u_fillColor = vec4(0.1, 0.0, 1.0, 1.0);     // 当凹角时要填充的颜色

    // 读取中心像素（Alpha通道）
    vec4 centerColor = texture(u_texture, v_texCoord);
    float pCenter = centerColor.a;

    // 定义8个邻域的UV偏移
    //  [-1,-1], [ 0,-1], [+1,-1]
    //  [-1, 0],           [+1, 0]
    //  [-1,+1], [ 0,+1], [+1,+1]
    float pUpLeft     = texture(u_texture, v_texCoord + vec2(-u_texelSize.x, -u_texelSize.y)).a;
    float pUp         = texture(u_texture, v_texCoord + vec2( 0.0,             -u_texelSize.y)).a;
    float pUpRight    = texture(u_texture, v_texCoord + vec2( u_texelSize.x,  -u_texelSize.y)).a;
    float pLeft       = texture(u_texture, v_texCoord + vec2(-u_texelSize.x,   0.0           )).a;
    float pRight      = texture(u_texture, v_texCoord + vec2( u_texelSize.x,   0.0           )).a;
    float pDownLeft   = texture(u_texture, v_texCoord + vec2(-u_texelSize.x,   u_texelSize.y)).a;
    float pDown       = texture(u_texture, v_texCoord + vec2( 0.0,              u_texelSize.y)).a;
    float pDownRight  = texture(u_texture, v_texCoord + vec2( u_texelSize.x,   u_texelSize.y)).a;

    // 8邻域拉普拉斯
    float laplacian = (pUpLeft + pUp + pUpRight + pLeft + pRight + pDownLeft + pDown + pDownRight) 
                      - 8.0 * pCenter;

    // float laplacian = (clamp(pUpLeft, 0.0, 1.0) + clamp(pUp, 0.0, 1.0) + clamp(pUpRight, 0.0, 1.0) + clamp(pLeft, 0.0, 1.0)
    //  + clamp(pRight, 0.0, 1.0) + clamp(pDownLeft, 0.0, 1.0) + clamp(pDown, 0.0, 1.0) + clamp(pDownRight, 0.0, 1.0)) 
    //                 - 4.0;
    // u_cornerThreshold = 3.9;

    // 判断是否为“凹陷尖角”或“凸起尖角”
    if (laplacian > u_cornerThreshold)
    {
        // 凹陷尖角 - 用指定颜色填充
        FragColor = u_fillColor;
    }
    else if (laplacian < -u_cornerThreshold)
    {
        // 凸起尖角 - 输出透明
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        // FragColor = centerColor;
    }
    else
    {
        // 其他情况 - 保持原Alpha，不改变像素颜色(可根据需求改写)
        // 这里示例设为白色 * 原Alpha
        FragColor = centerColor;//vec4(1.0, 1.0, 1.0, pCenter);
    }
}
