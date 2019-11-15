//�e�N�X�`��
Texture2D<float4> tex : register(t0);

// �T���v��
SamplerState smp : register(s0);

struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

//���_�V�F�[�_
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

//�s�N�Z���V�F�[�_
float4 peraPS(Out o) : SV_Target
{
    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float dx = 0.5f / w;
    float dy = 0.5f / h;
    float4 ret = tex.Sample(smp, o.uv);
    ret = ret * 4 -
    tex.Sample(smp, o.uv + float2(-dx, 0)) -
    tex.Sample(smp, o.uv + float2(dx, 0)) -
    tex.Sample(smp, o.uv + float2(0, dy)) -
    tex.Sample(smp, o.uv + float2(0, -dy));
    float b = dot(float3(0.3f, 0.3f, 0.4f), 1 - ret.rgb);
    b = pow(b, 4);

    //return float4(
    //tex.Sample(smp, o.uv + float2(dx * -5, 0)).r,
    //tex.Sample(smp, o.uv + float2(dx * 5, 0)).g,
    //tex.Sample(smp, o.uv).ba
    //);

    return float4(float4(b, b, b, 1) * tex.Sample(smp, o.uv));
}