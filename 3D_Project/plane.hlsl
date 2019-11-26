//�e�N�X�`��
Texture2D<float4> tex : register(t0);

// �T���v��
SamplerState smp : register(s0);

// �s��(�}�g���b�N�X)
cbuffer mat : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
    matrix shadow;
    matrix wvp;
    matrix lvp;
};

struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

//���_�V�F�[�_
Out PlaneVS(
    float4 pos : POSITION,
    float2 uv : TEXCOORD)
{
    Out o;
    o.pos = mul(lvp, pos);
    o.svpos = mul(wvp, pos);
    o.uv = uv;
    return o;
}

//�s�N�Z���V�F�[�_
float4 PlanePS(Out o) : SV_Target
{
    float4 color = float4(1, 0.5, 0.8, 1);

    float2 shadowMapUV = o.pos.xy;
    shadowMapUV = (shadowMapUV + float2(1, -1)) * float2(0.5, -0.5);

    float depth = pow(tex.Sample(smp, shadowMapUV).z, 100);
    float3 dbright = float3(1, 1, 1);
    if (o.pos.z > depth + 0.005f)
    {
        dbright *= 0.9f;
    }
    return color*float4(dbright, 1);
}