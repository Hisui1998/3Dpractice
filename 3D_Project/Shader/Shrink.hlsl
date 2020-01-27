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
Out shrinkVS(
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
float4 shrinkPS(Out o) : SV_Target
{
    return tex.Sample(smp, o.uv);
}