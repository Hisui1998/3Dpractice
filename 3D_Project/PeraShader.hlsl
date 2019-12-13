//テクスチャ
Texture2D<float4> tex : register(t0);
//深度テクスチャ
Texture2D<float4> depth : register(t1);
//深度テクスチャライト
Texture2D<float4> lightdepth : register(t2);
//法線
Texture2D<float4> normal : register(t3);
//高輝度
Texture2D<float4> bloom : register(t4);
//ブルーム等の縮小バッファ
Texture2D<float4> shrink : register(t5);
//輪郭線抽出用影絵
Texture2D<float4> outline : register(t6);
//被写界深度用縮小バッファ
Texture2D<float4> scene : register(t7);
//SSAo
Texture2D<float> ssaotex : register(t8);

// サンプラ
SamplerState smp : register(s0);

// 色情報
cbuffer colors : register(b0)
{
    float4 bloomCol;
};
// huragu
cbuffer flags : register(b1)
{
    int GBuffer;
    int CenterLine;
};

// ガウシアンぼかしの式
float4 GaussianFilteredColor5x5(Texture2D<float4> intex,SamplerState smpst,float2 inuv,float dx, float dy)
{
    float4 ret = intex.Sample(smpst, inuv);
    ret += intex.Sample(smp, inuv + float2(-2 * dx, 2 * dy)) * 1;
    ret += intex.Sample(smp, inuv + float2(-1 * dx, 2 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2(0 * dx, 2 * dy)) * 6;
    ret += intex.Sample(smp, inuv + float2(1 * dx, 2 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2(2 * dx, 2 * dy)) * 1;
    ret += intex.Sample(smp, inuv + float2(-2 * dx, 1 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2(-1 * dx, 1 * dy)) * 16;
    ret += intex.Sample(smp, inuv + float2(0 * dx, 1 * dy)) * 24;
    ret += intex.Sample(smp, inuv + float2(1 * dx, 1 * dy)) * 16;
    ret += intex.Sample(smp, inuv + float2(2 * dx, 1 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2(-2 * dx, 0 * dy)) * 6;
    ret += intex.Sample(smp, inuv + float2(-1 * dx, 0 * dy)) * 24;
    
    ret += intex.Sample(smp, inuv + float2(1 * dx, 0 * dy)) * 24;
    ret += intex.Sample(smp, inuv + float2(2 * dx, 0 * dy)) * 6;
    ret += intex.Sample(smp, inuv + float2(-2 * dx, -1 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2(-1 * dx, -1 * dy)) * 16;
    ret += intex.Sample(smp, inuv + float2(0 * dx, -1 * dy)) * 24;
    ret += intex.Sample(smp, inuv + float2(1 * dx, -1 * dy)) * 16;
    ret += intex.Sample(smp, inuv + float2(2 * dx, -1 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2(-2 * dx, -2 * dy)) * 1;
    ret += intex.Sample(smp, inuv + float2(-1 * dx, -2 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2(0 * dx, -2 * dy)) * 6;
    ret += intex.Sample(smp, inuv + float2(1 * dx, -2 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2(2 * dx, -2 * dy)) * 1;
    ret /= 256;
    return ret;
}

struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

//頂点シェーダ
Out peraVS(
    float4 pos : POSITION,
    float2 uv : TEXCOORD)
{
    Out o;
    o.pos = pos;
    o.svpos = pos;
    o.uv = uv;
    return o;
}

//ピクセルシェーダ
float4 peraPS(Out o) : SV_TarGet
{
    float4 texColor = tex.Sample(smp, o.uv);
    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float dx = 1 / w;
    float dy = 1 / h;
    
    float4 GaussTex = GaussianFilteredColor5x5(tex, smp, o.uv, dx, dy);
    float4 GaussScene = GaussianFilteredColor5x5(scene, smp, o.uv, dx, dy);
    
    float edgesize = 2.f;
    float4 tc = outline.Sample(smp, o.uv);
    tc = tc * 4 -
    outline.Sample(smp, o.uv + float2(-dx * edgesize, 0)) -
    outline.Sample(smp, o.uv + float2(dx * edgesize, 0)) -
    outline.Sample(smp, o.uv + float2(0, dy * edgesize)) -
    outline.Sample(smp, o.uv + float2(0, -dy * edgesize));
    float edge = dot(float3(0.3f, 0.3f, 0.4f), 1 - tc.rgb);
    edge = pow(edge,10);
    if (CenterLine.r)
    {
        if ((o.uv.x <= 0.5005f) && (o.uv.x >= 0.4995f))
        {
            return 1-texColor;
        }
        else if ((o.uv.y <= 0.501f) && (o.uv.y >= 0.499f))
        {
            return 1 - texColor;
        }
    }
    if (GBuffer.r)
    {
        if ((o.uv.x <= 0.2) && (o.uv.y <= 0.2))
        {
        // カメラからの深度値
            float dep = depth.Sample(smp, o.uv * 5);
            dep = pow(dep, 100);
            return 1 - float4(dep, dep, dep, 1);
        }
        else if ((o.uv.x <= 0.2) && (o.uv.y <= 0.4))
        {
            float dep = lightdepth.Sample(smp, o.uv * 5);
            dep = pow(dep, 100);
            return 1 - float4(dep, dep, dep, 1);
        }
        else if ((o.uv.x <= 0.2) && (o.uv.y <= 0.6))
        {
            float4 dep = normal.Sample(smp, o.uv * 5);
            return dep;
        }
        else if ((o.uv.x <= 0.2) && (o.uv.y <= 0.8))
        {
            float4 dep = bloom.Sample(smp, o.uv * 5);
            return dep;
        }
        else if ((o.uv.x <= 0.2) && (o.uv.y <= 1.0))
        {
            float4 dep = shrink.Sample(smp, o.uv * 5);
            return dep;
        }
        else if ((o.uv.x <= 0.4) && (o.uv.y <= 0.2))
        {
            float4 dep = GaussianFilteredColor5x5(scene, smp, o.uv * 5, dx, dy);
            return dep;
        }
        else if ((o.uv.x <= 0.4) && (o.uv.y <= 0.4))
        {
            float4 dep = ssaotex.Sample(smp, o.uv * 5);
            return dep;
        }
    }
        
    float4 oneBloom = GaussianFilteredColor5x5(bloom, smp, o.uv, dx, dy);
    float4 bloomSum = float4(0, 0, 0, 0);
    float2 uvSize = float2(1, 0.5);
    float2 uvOffset = float2(0, 0);
    for (int i = 0; i < 8; ++i)
    {
        bloomSum += GaussianFilteredColor5x5(shrink, smp, o.uv * uvSize + uvOffset, dx, dy);
        uvOffset.y += uvSize.y;
        uvSize *= 0.5f;
    }
    
    //画面真ん中からの深度の差を測る
    float depthDiff = abs(depth.Sample(smp, float2(0.5, 0.5)) - depth.Sample(smp, o.uv));
    depthDiff = pow(depthDiff, 0.5f);
    uvSize = float2(1, 0.5);
    uvOffset = float2(0, 0);
    float t = depthDiff * 8;
    float no;
    t = modf(t, no);// noには整数部が入って、返り値は余りの小数部が帰ってくる
    float4 retColor[2];
    retColor[0] = tex.Sample(smp, o.uv); //通常テクスチャ
    if (no == 0.0f)
    {
        retColor[1] = GaussianFilteredColor5x5(scene, smp, o.uv * uvSize + uvOffset, dx*2, dy*2);
    }
    else
    {
        for (int i = 1; i <= 8; ++i)
        {
            if (i - no < 0)
                continue;
            retColor[i - no] = GaussianFilteredColor5x5(scene, smp, o.uv * uvSize + uvOffset, dx*2, dy*2);
            uvOffset.y += uvSize.y;
            uvSize *= 0.5f;
            if (i - no > 1)
            {
                break;
            }
        }
    }
    
    return lerp(retColor[0], retColor[1], t) + bloomCol * bloomSum;
    
    return float4(1, edge, edge, edge)*texColor + saturate((GaussianFilteredColor5x5(bloom, smp, o.uv, dx, dy) + bloomCol) * bloomSum);
    
    return GaussTex; // ガウスぼかし
    return float4(1, edge, edge, 1); // 輪郭線のみ
    return oneBloom;// 等倍ブルーム
    return float4(1, edge, edge, 1) * texColor; // 輪郭線+テクスチャカラー
}