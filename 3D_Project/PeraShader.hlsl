//テクスチャ
Texture2D<float4> tex : register(t0);

//深度テクスチャ
Texture2D<float4> depth : register(t1);

//深度テクスチャライト
Texture2D<float4> lightdepth : register(t2);

//法線
Texture2D<float4> normal : register(t3);

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
    ret2 = pow(ret2,4);
    //return float4(ret2, ret2, ret2, 1) * texColor;
    
    if ((o.uv.x <= 0.2) && (o.uv.y <= 0.2))
    {
        float dep = depth.Sample(smp, o.uv * 5);
        dep = pow(dep, 100);
        return 1-float4(dep, dep, dep, 1);
    }
    else if ((o.uv.x <= 0.2) && (o.uv.y <= 0.4))
    {
        float dep = lightdepth.Sample(smp, o.uv * 5);
        dep = pow(dep, 100);
        return 1-float4(dep, dep, dep, 1);
    }
    else if ((o.uv.x <= 0.2) && (o.uv.y <= 0.6))
    {
        float4 dep = normal.Sample(smp, o.uv * 5);
        //dep = pow(dep, 100);
        return dep;
    }
    
    return tex.Sample(smp, o.uv);
    return float4(1, b, b, 1) * texColor;
}