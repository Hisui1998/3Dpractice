//テクスチャ
Texture2D<float> shadowMap : register(t0);

// サンプラ
SamplerState smp : register(s0);

// 行列(マトリックス)
cbuffer mat : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
    matrix wvp;
    matrix lvp;
    float3 lightPos;
};

struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

//頂点シェーダ
Out PlaneVS(
    float4 pos : POSITION,
    float2 uv : TEXCOORD)
{
    Out o;
    o.pos = mul(world, pos);
    o.svpos = mul(wvp, pos);
    o.uv = uv;
    return o;
}

//ピクセルシェーダ
float4 PlanePS(Out o) : SV_Target
{
    float4 color = float4(1, 1, 1, 1);// 床の色
    
    if (((int) (o.uv.x * 100) % 2)&&((int) (o.uv.y * 100) % 2))
    {
        color *= 0.6f;
    }
    
    // シャドウマップ用UVの作成
        float2 shadowMapUV = mul(lvp, o.pos).xy;
    shadowMapUV = (shadowMapUV + float2(1, -1)) * float2(0.5, -0.5);

    // 深度値の取得
    float depth = pow(shadowMap.Sample(smp, shadowMapUV), 10);
    
    // 深度値の比較
    float dbright = 1;// 影の強さ
    if (mul(lvp, o.pos).z > depth)
    {
        dbright *= 0.3f;
    }
    
    return float4(color.rgb * dbright, color.a);
}