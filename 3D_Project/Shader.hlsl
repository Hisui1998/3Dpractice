// �e�N�X�`��
Texture2D<float4> tex : register(t0);

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
};

// �}�e���A��
cbuffer material : register(b1)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
}

// ��Z�X�t�B�A�}�b�v
Texture2D<float4> sph : register(t1); //1 �ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��

// ��Z�X�t�B�A�}�b�v
Texture2D<float4> spa : register(t2); //1 �ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��


struct Out
{
	float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL0;
    float3 vnormal : NORMAL1;
};

// ���_�V�F�[�_
Out vs( float3 pos : POSITION,
        float2 uv  : TEXCOORD,
        float3 normal : NORMAL)
{
	Out o;
    o.pos = mul(world, float4(pos, 1));
    o.svpos = mul(wvp, float4(pos, 1));
    o.uv = uv;
    o.normal = mul(world, float4(normal, 1));
    o.vnormal = mul(view, float4(o.normal, 1));
	return o;
}

// �s�N�Z���V�F�[�_
float4 ps(Out o):SV_TARGET
{    
    // ����
    float3 light = normalize(float3(1, -1, 1));
    light = normalize(light);
    float3 lightColor = float3(1, 1, 1);

    // ����
    float3 mirror = reflect(light,o.normal);
    
    // ���ˌ�
    float spec = saturate(dot(reflect(light, o.normal), float3(0,0,0)));
    spec = pow(spec, specular.a);

    // �e
    float brightness = saturate(dot(light, o.normal.xyz));

    // ���K��UV
    float2 normalUV = (o.normal.xy + float2(1, -1)) * float2(0.5, -0.5);

    // �X�t�B�A�}�b�v��UV
    float2 sphereMapUV = o.vnormal.xy;

    // �Ԃ茌
    return float4(saturate(brightness + specular.xyz * spec + ambient.xyz), 1)
                    * diffuse * tex.Sample(smp, o.uv) * sph.Sample(smp, sphereMapUV) + spa.Sample(smp, sphereMapUV);
}