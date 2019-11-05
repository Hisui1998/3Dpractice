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

	float2 adduv1 : ADDUV0;
	float2 adduv2 : ADDUV1;
	float2 adduv3 : ADDUV2;
	float2 adduv4 : ADDUV3;

    min16uint weighttype : WEIGHTTYPE;

	min16uint4 boneno : BONENO0;

	min16float weight1 : WEIGHT0;
	min16float weight2 : WEIGHT1;
	min16float weight3 : WEIGHT2;
	min16float weight4 : WEIGHT3;
};

// 頂点シェーダ
Out vs(
	float3 pos : POSITION,
    float3 normal : NORMAL,
    float2 uv  : TEXCOORD,

	float2 adduv1 : ADDUV0,
	float2 adduv2 : ADDUV1,
	float2 adduv3 : ADDUV2,
	float2 adduv4 : ADDUV3,

	min16uint weighttype : WEIGHTTYPE,

	min16uint4 boneno: BONENO,

	min16float weight1 : WEIGHT0,
	min16float weight2 : WEIGHT1,
	min16float weight3 : WEIGHT2,
	min16float weight4 : WEIGHT3
)
/*頂点シェーダ*/
{
    Out o;
	matrix m = boneMats[boneno.x] * 1.0f;// ウェイト1.0の固定値
	
    if (weighttype == 1)
    {
		m = boneMats[boneno.x] * weight1 + boneMats[boneno.y] * (1.0f - weight1);
    }
    else if (weighttype == 2)
    {
        m = boneMats[boneno.x] * weight1 + boneMats[boneno.y] * weight2 + boneMats[boneno.z] * weight3 + boneMats[boneno.w] * weight4;
    }
	else if (weighttype == 3)
	{
		m = boneMats[boneno.x] * weight1 + boneMats[boneno.y] * (1.0f - weight1);
	}


	pos = mul(m, float4(pos, 1));
    o.pos = mul(world, float4(pos, 1));
    o.svpos = mul(wvp, float4(pos, 1));
    o.uv = uv;
    o.normal = mul(world, float4(normal, 1));
    o.vnormal = mul(view, float4(o.normal, 1));

	o.weighttype = weighttype;

    o.boneno = boneno;

    o.weight1 = weight1;
    o.weight2 = weight2;
    o.weight3 = weight3;
    o.weight4 = weight4;

	return o;
}

// ピクセルシェーダ
float4 ps(Out o):SV_TARGET
{
	//return float4(o.weight1,o.weight2,0.0,1);
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
                    + saturate(specularB*specular);

    return saturate(toonDif * diffuse * texColor * sph.Sample(smp, sphereMapUV))
            + spa.Sample(smp, sphereMapUV) * texColor
            + saturate(specularB * specular)
            + float4(texColor.rgb * ambient*0.2, texColor.a);
}