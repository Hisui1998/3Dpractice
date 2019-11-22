//テクスチャ
Texture2D<float4> tex : register(t0);

//深度テクスチャ
Texture2D<float4> depth : register(t1);

//深度テクスチャライト
Texture2D<float4> lightdepth : register(t2);

// サンプラ
SamplerState smp : register(s0);

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
    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float dx = 0.75 / w;
    float dy = 0.75 / h;
    float4 ret = tex.Sample(smp, o.uv);
    ret = ret * 4 -
    tex.Sample(smp, o.uv + float2(-dx, 0)) -
    tex.Sample(smp, o.uv + float2(dx, 0)) -
    tex.Sample(smp, o.uv + float2(0, dy)) -
    tex.Sample(smp, o.uv + float2(0, -dy));
    float b = dot(float3(0.3f, 0.3f, 0.4f), 1 - ret.rgb);
    b = pow(b, 4);

    if ((o.uv.x <= 0.2) && (o.uv.y <= 0.2))
    {
        float dep = depth.Sample(smp, o.uv * 5);
        dep = pow(dep, 100);
        return 1-float4(dep, dep, dep, 1);
    }
    else if ((o.uv.x <= 0.2) && (o.uv.y <= 0.4))
    {
        float dep = lightdepth.Sample(smp, o.uv * 5);
        dep = pow(dep, 10);
        return 1-float4(dep, dep, dep, 1);
    }

    float dep = lightdepth.Sample(smp, o.uv);

    return tex.Sample(smp, o.uv);
}