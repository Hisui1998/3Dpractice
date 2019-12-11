#define FXAA_GRAY_AS_LUMA 1
#define FXAA_PC 1
#define FXAA_HLSL_5 1
#include"FXAA.hlsl"

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
    float3 lightPos;
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
// シャドウ
Texture2D<float> shadowMap : register(t4); //4 番スロットに設定されたテクスチャ(shadow)
struct Out
{
	float4 pos              : POSITION;
    float4 svpos            : SV_POSITION;
    float2 uv               : TEXCOORD0;

    float3 normal           : NORMAL0;
    float3 vnormal          : NORMAL1;

	float4 adduv1           : ADDUV0;
	float4 adduv2           : ADDUV1;
	float4 adduv3           : ADDUV2;
	float4 adduv4           : ADDUV3;

    min16uint weighttype    : WEIGHTTYPE;

    int4 boneno : BONENO0;

    float4 weight : WEIGHT0;
    uint instNo : INSTID;
};

struct PixelOut
{
    float4 zero : SV_TARGET0;
    float4 one : SV_TARGET1;
    float4 two : SV_TARGET2;
};

// 頂点シェーダ
Out vs(
	float3 pos              : POSITION,
    float3 normal           : NORMAL,
    float2 uv               : TEXCOORD,

	float4x4 adduv : ADDUV,

	min16uint weighttype : WEIGHTTYPE,

	int4 boneno : BONENO,

	float4 weight : WEIGHT,
    uint instNo : SV_InstanceID
)
/*頂点シェーダ*/
{
    Out o;
    matrix m = boneMats[boneno.x]; // ウェイト1.0の固定値
    if (weighttype == 1)
    {
        m =
        boneMats[boneno.x] * float(weight.x) +
        boneMats[boneno.y] * (1.0f - float(weight.x));
    }
    else if (weighttype == 2)
    {
        m = 
        boneMats[boneno.x] * float(weight.x) + 
        boneMats[boneno.y] * float(weight.y) + 
        boneMats[boneno.z] * float(weight.z) + 
        boneMats[boneno.w] * float(weight.w);
    }
	else if (weighttype == 3)
    {
        m =
        boneMats[boneno.x] * float(weight.x) +
        boneMats[boneno.y] * (1.0f - float(weight.x));
    }
    
    m._m03 += 10 * (instNo / 5) + 5 * ((instNo % 5) % 2);//x
    m._m23 += 10 * (instNo % 5) + 5 * ((instNo / 5) % 2);//z
    
    float4 movepos = mul(m, float4(pos, 1));
    
    o.pos = mul(world,movepos);    
    
    o.svpos = mul(wvp, movepos);
    
    o.uv = uv;
    
    o.normal = mul(world, float4(normal, 1));
    o.vnormal = mul(view, float4(o.normal, 1));

	o.weighttype = weighttype;

    o.boneno = boneno;

    o.weight = weight;
    o.instNo = instNo;
    
	return o;
}


float4 FXAAtoTEX(Texture2D<float4> intex, SamplerState insmp,float2 inuv,float dx,float dy)
{
    FxaaTex InputFXAATex = { insmp, intex };
    float4 fxaaShrink = FxaaPixelShader(
		inuv, // FxaaFloat2 pos,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f), // FxaaFloat4 fxaaConsolePosPos,
		InputFXAATex, // FxaaTex tex,
		InputFXAATex, // FxaaTex fxaaConsole360TexExpBiasNegOne,
		InputFXAATex, // FxaaTex fxaaConsole360TexExpBiasNegTwo,
		float2(dx, dy), // FxaaFloat2 fxaaQualityRcpFrame,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f), // FxaaFloat4 fxaaConsoleRcpFrameOpt,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f), // FxaaFloat4 fxaaConsoleRcpFrameOpt2,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f), // FxaaFloat4 fxaaConsole360RcpFrameOpt2,
		0.75f, // FxaaFloat fxaaQualitySubpix,
		0.166f, // FxaaFloat fxaaQualityEdgeThreshold,
		0.0833f, // FxaaFloat fxaaQualityEdgeThresholdMin,
		0.0f, // FxaaFloat fxaaConsoleEdgeSharpness,
		0.0f, // FxaaFloat fxaaConsoleEdgeThreshold,
		0.0f, // FxaaFloat fxaaConsoleEdgeThresholdMin,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f) // FxaaFloat fxaaConsole360ConstDir,
	    );
    return fxaaShrink;
}

// ピクセルシェーダ
PixelOut ps(Out o)
{
    PixelOut po;
    // 視線ベクトル
    float3 eye = float3(0, 10, -25);
    float3 ray = o.pos.xyz - eye;
    
    float3 light = normalize(lightPos); //光の向かうベクトル(平行光線)

    //ディフューズ計算
    float diffuseB = saturate(dot(-light, o.normal));// 光の反射ベクトル
    float4 toonDif = toon.Sample(smpToon, float2(0, 0));

    //光の反射ベクトル
    float3 refLight = normalize(reflect(light, o.normal.xyz));
    float specularB = pow(saturate(dot(refLight, -ray)), specular.a);

    //スフィアマップ用UV(中心原点)
    float2 sphereMapUV = o.vnormal.xy;
    sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

    float4 texColor = tex.Sample(smp, o.uv);   
    
    // シャドウマップ用UVの作成
    float4 shadowpos = mul(lvp, o.pos);
    float2 shadowMapUV = shadowpos.xy;
    shadowMapUV = (shadowMapUV + float2(1, -1)) * float2(0.5, -0.5);

    // 深度値の取得
    float depth = shadowMap.Sample(smp, shadowMapUV);
    
    // 深度値の比較
    float dbright = 1;// 影の強さ
    if (shadowpos.z >= depth + 0.00005f)
    {
        //texColor.rgb *= 0.5;
        toonDif = toon.Sample(smpToon, float2(0, 1));
    }
    po.zero = saturate(texColor * diffuse * toonDif);
    po.one = float4(o.normal, 1);
        
    float y = dot(float3(0.3f, 0.6f, 0.1f), texColor.rgb);
    po.two = y > 0.99f ? texColor : 0.0;
    
    return po;
    
    //return saturate(toonDif * diffuse * texColor * sph.Sample(smp, sphereMapUV))
    //        + spa.Sample(smp, sphereMapUV) * texColor
    //        + saturate(specularB * specular)
    //        + float4(texColor.rgb * ambient, texColor.a);
}