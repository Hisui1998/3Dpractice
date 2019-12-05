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
Out pera2VS(
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
float4 pera2PS(Out o) : SV_Target
{
    return tex.Sample(smp,o.uv);
    
    float4 texCol = tex.Sample(smp, o.uv);
    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float dx = 1 / w;
    float dy = 1 / h;
    float mag = 1;
    
    float4 rtnCol = tex.Sample(smp, o.uv);
    rtnCol = rtnCol * 4 -
    tex.Sample(smp, o.uv + float2(dx * mag, 0)) -
    tex.Sample(smp, o.uv + float2(-dx * mag, 0))-
    tex.Sample(smp, o.uv + float2(0, dy * mag)) -
    tex.Sample(smp, o.uv + float2(0, -dy * mag));
    
    rtnCol = pow(rtnCol, 4);
    
    return rtnCol;
}