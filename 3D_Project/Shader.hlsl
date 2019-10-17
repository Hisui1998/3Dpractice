// テクスチャ
Texture2D<float4> tex : register(t0);

// サンプラ
SamplerState smp : register(s0);

SamplerState smpToon : register(s1); //1 番スロットに設定されたサンプラ(トゥーン用) 

// 行列(マトリックス)
cbuffer mat : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
    matrix wvp;
    matrix lvp;
};

// マテリアル
cbuffer material : register(b1)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
}

///ボーン行列
cbuffer bones : register(b2)
{
    matrix boneMats[512];
}

// 乗算スフィアマップ
Texture2D<float4> sph : register(t1); //1 番スロットに設定されたテクスチャ

// 乗算スフィアマップ
Texture2D<float4> spa : register(t2); //2 番スロットに設定されたテクスチャ

// トゥーン
Texture2D<float4> toon : register(t3); //3 番スロットに設定されたテクスチャ(トゥーン)
struct Out
{
	float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL0;
    float3 vnormal : NORMAL1;
    min16uint2 boneno : BONENO;
    min16uint weight : WEIGHT;
};

// 頂点シェーダ
Out vs( float3 pos : POSITION,
        float2 uv  : TEXCOORD,
        float3 normal : NORMAL,
        min16uint2 boneno : BONENO,
        min16uint weight : WEIGHT)
{
    Out o;
    float w = weight / 100.f;
    matrix m = boneMats[boneno.x] * w + boneMats[boneno.y] * (1 - w);
    pos = mul(m, float4(pos,1));

    o.pos = mul(world, float4(pos, 1));
    o.svpos = mul(wvp, float4(pos, 1));
    o.uv = uv;
    o.normal = mul(world, float4(normal, 1));
    o.vnormal = mul(view, float4(o.normal, 1));

	return o;
}

// ピクセルシェーダ
float4 ps(Out o):SV_TARGET
{
    float3 light = normalize(float3(1, -1, 1)); //光の向かうベクトル(平行光線)
    float3 lightColor = float3(1, 1, 1); //ライトのカラー(1,1,1で真っ白)

    //ディフューズ計算
    float diffuseB = saturate(dot(-light, o.normal));
    float4 toonDif = toon.Sample(smpToon, float2(0, 1.0 - diffuseB));

    //光の反射ベクトル
    float3 refLight = normalize(reflect(light, o.normal.xyz));
    float specularB = pow(saturate(dot(refLight, float3(0,0,0))), specular.a);

    //スフィアマップ用UV
    float2 sphereMapUV = o.vnormal.xy;
    sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

    float4 texColor = tex.Sample(smp, o.uv); //テクスチャカラー

    return saturate(toonDif * diffuse * texColor * sph.Sample(smp, sphereMapUV))
            + spa.Sample(smp, sphereMapUV) * texColor 
            + float4(specularB * specular.rgb, 1) 
            + float4(texColor.xyz * ambient * 0.5, 1); 
}