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
	float4 pos      : POSITION;
    float4 svpos    : SV_POSITION;
    float2 uv       : TEXCOORD0;

    float3 normal   : NORMAL0;
    float3 vnormal  : NORMAL1;

    float4 adduv1 : TEXCOORD1;
    float4 adduv2 : TEXCOORD2;
    float4 adduv3 : TEXCOORD3;
    float4 adduv4 : TEXCOORD4;

    uint weighttype : WEIGHTTYPE;

    int boneno1 : BONENO;
    int boneno2 : BONENO1;
    int boneno3 : BONENO2;
    int boneno4 : BONENO3;

    float weight1 : WEIGHT;
    float weight2 : WEIGHT1;
    float weight3 : WEIGHT2;
    float weight4 : WEIGHT3;
};

// 頂点シェーダ
Out vs(float3 pos : POSITION,
        float3 normal : NORMAL,
        float2 uv  : TEXCOORD,

        float4 adduv1 : TEXCOORD1,
        float4 adduv2 : TEXCOORD2,
        float4 adduv3 : TEXCOORD3,
        float4 adduv4 : TEXCOORD4,

        uint weighttype : WEIGHTTYPE,

        int boneno1 : BONENO,
        int boneno2 : BONENO1,
        int boneno3 : BONENO2,
        int boneno4 : BONENO3,

        float weight1 : WEIGHT,
        float weight2 : WEIGHT1,
        float weight3 : WEIGHT2,
        float weight4 : WEIGHT3)
{
    Out o;

    if (weighttype == 0)
    {
        pos = mul(boneMats[boneno1], float4(pos, 1));
    }
    else if (weighttype == 1)
    {
        matrix m = boneMats[boneno1] * weight1 + boneMats[boneno2] * (1.0f - weight1);
        pos = mul(m, float4(pos, 1));
    }
    else if (weighttype == 2)
    {
        matrix m = boneMats[boneno1] * weight1 + boneMats[boneno2] * weight2 + boneMats[boneno3] * weight3 + boneMats[boneno4] * weight4;
        pos = mul(m, float4(pos, 1));
    }
    else if (weighttype == 3)
    {
        matrix m = boneMats[boneno1] * weight1 + boneMats[boneno2] * (1.0f - weight1);
        pos = mul(m, float4(pos, 1));
    }


    o.pos = mul(world, float4(pos, 1));
    o.svpos = mul(wvp, float4(pos, 1));
    o.uv = uv;
    o.normal = mul(world, float4(normal, 1));
    o.vnormal = mul(view, float4(o.normal, 1));

    o.boneno1 = boneno1;
    o.boneno2 = boneno2;
    o.boneno3 = boneno3;
    o.boneno4 = boneno4;

    o.weight1 = weight1;
    o.weight2 = weight2;
    o.weight3 = weight3;
    o.weight4 = weight4;

    // ﾃﾞﾊﾞｯｸﾞ用
    // o.weight = weight;

	return o;
}

// ピクセルシェーダ
float4 ps(Out o):SV_TARGET
{
    // 視線ベクトル
    float3 eye = float3(0, 20, -20);
    float3 ray = o.pos.xyz - eye;

    float3 light = normalize(float3(-1, -1, 1)); //光の向かうベクトル(平行光線)

    //ディフューズ計算
    float diffuseB = saturate(dot(-light, o.normal));
    float4 toonDif = toon.Sample(smpToon, float2(0, 1 - diffuseB));

    //光の反射ベクトル
    float3 refLight = normalize(reflect(light, o.normal.xyz));
    float specularB = pow(saturate(dot(refLight, -ray)), specular.a);

    //スフィアマップ用UV
    float2 sphereMapUV = o.vnormal.xy;
    sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

    float4 texColor = tex.Sample(smp, o.uv); //テクスチャカラー

    return saturate(texColor * diffuse * toonDif);
                    +specularB;

    return saturate(toonDif * diffuse * texColor * sph.Sample(smp, sphereMapUV))
            + spa.Sample(smp, sphereMapUV) * texColor
            + saturate(specularB * specular)
            + float4(texColor.rgb * ambient*0.2, texColor.a);
}