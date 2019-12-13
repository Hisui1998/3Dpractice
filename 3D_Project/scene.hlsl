//テクスチャ
Texture2D<float4> tex : register(t0);

// サンプラ
SamplerState smp : register(s0);

struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

//頂点シェーダ
Out sceneVS(
    float4 pos : POSITION,
    float2 uv : TEXCOORD)
{
    Out o;
    o.pos = pos;
    o.svpos = pos;
    o.uv = uv;
    return o;
}
// ガウシアンぼかしの式
float4 GaussianFilteredColor5x5(Texture2D<float4> intex, SamplerState smpst, float2 inuv, float dx, float dy)
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

//ピクセルシェーダ
float4 scenePS(Out o) : SV_Target
{
    return tex.Sample(smp, o.uv);
}