//テクスチャ
Texture2D<float4> tex : register(t0);

//深度テクスチャ
Texture2D<float4> depth : register(t1);

//深度テクスチャライト
Texture2D<float4> lightdepth : register(t2);

//法線
Texture2D<float4> normal : register(t3);

//ブルーム
Texture2D<float4> bloom : register(t4);

//明るさ
Texture2D<float4> shrink : register(t5);

// サンプラ
SamplerState smp : register(s0);

// 色情報
cbuffer colors : register(b0)
{
    float4 bloomCol;
};

// 色情報
cbuffer flags : register(b1)
{
    bool GBuffer;
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
    float4 ret = texColor;
    ret = ret * 4 -
    tex.Sample(smp, o.uv + float2(-dx, 0)) -
    tex.Sample(smp, o.uv + float2(dx, 0)) -
    tex.Sample(smp, o.uv + float2(0, dy)) -
    tex.Sample(smp, o.uv + float2(0, -dy));
    float b = dot(float3(0.3f, 0.3f, 0.4f), 1 - ret.rgb);
    b = pow(b, 4);

    float ret2 = depth.Sample(smp, o.uv);
    ret2 = ret * 4 -
    depth.Sample(smp, o.uv + float2(-dx*4, 0)) -
    depth.Sample(smp, o.uv + float2(dx*4, 0)) -
    depth.Sample(smp, o.uv + float2(0, dy*4)) -
    depth.Sample(smp, o.uv + float2(0, -dy*4));
    
    ret2 = dot(0.4f, 1 - ret2);
    ret2 = pow(ret2,1);
    //return float4(ret2, ret2, ret2, 1) * texColor;
    
   
    //return ret3 * texColor;
    if (GBuffer)
    {
        if ((o.uv.x <= 0.2) && (o.uv.y <= 0.2))
        {
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
    //return bloomCol;
    return texColor + saturate((GaussianFilteredColor5x5(bloom, smp, o.uv, dx, dy) + bloomCol) * bloomSum);
    
    //return float4(aa, texColor.a);
    //return oneBloom;
    return float4(1, b, b, 1) * texColor;
}