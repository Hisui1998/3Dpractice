//�e�N�X�`��
Texture2D<float> shadowMap : register(t0);

// �T���v��
SamplerState smp : register(s0);

// �s��(�}�g���b�N�X)
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

//���_�V�F�[�_
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

//�s�N�Z���V�F�[�_
float4 PlanePS(Out o) : SV_Target
{
    float4 color = float4(1, 1, 1, 1);// ���̐F
    
    if (((int) (o.uv.x * 100) % 2)&&((int) (o.uv.y * 100) % 2))
    {
        color *= 0.6f;
    }
    
    // �V���h�E�}�b�v�pUV�̍쐬
        float2 shadowMapUV = mul(lvp, o.pos).xy;
    shadowMapUV = (shadowMapUV + float2(1, -1)) * float2(0.5, -0.5);

    // �[�x�l�̎擾
    float depth = pow(shadowMap.Sample(smp, shadowMapUV), 10);
    
    // �[�x�l�̔�r
    float dbright = 1;// �e�̋���
    if (mul(lvp, o.pos).z > depth)
    {
        dbright *= 0.3f;
    }
    
    return float4(color.rgb * dbright, color.a);
}