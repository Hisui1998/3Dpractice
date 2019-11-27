// サンプラ
SamplerState smp : register(s0);

// 行列(マトリックス)
cbuffer mat : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
    matrix shadow;
    matrix wvp;
    matrix lvp;
};

///ボーン行列
cbuffer bones : register(b1)
{
    matrix boneMats[512];
}

struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD0;

    float3 normal : NORMAL0;
    float3 vnormal : NORMAL1;

    float4 adduv1 : ADDUV0;
    float4 adduv2 : ADDUV1;
    float4 adduv3 : ADDUV2;
    float4 adduv4 : ADDUV3;

    min16uint weighttype : WEIGHTTYPE;

    int4 boneno : BONENO0;

    float4 weight : WEIGHT0;
    uint instNo : INSTID;
};

// 頂点シェーダ
Out shadowVS(
	float3 pos : POSITION,
    float3 normal : NORMAL,
    float2 uv : TEXCOORD,

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

    float4 movepos = mul(m, float4(pos, 1));
    movepos = mul(shadow, movepos);
    o.pos = mul(world, movepos);
    o.svpos = mul(lvp, movepos);

    return o;
}

// ピクセルシェーダ
float4 shadowPS() : SV_TARGET
{    
   return  float4(1, 1, 1, 1);
}