// �e�N�X�`��
Texture2D<float4> tex : register(t0);

// �T���v��
SamplerState smp : register(s0);

SamplerState smpToon : register(s1); //1 �ԃX���b�g�ɐݒ肳�ꂽ�T���v��(�g�D�[���p) 

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

///�{�[���s��
cbuffer bones : register(b2)
{
    matrix boneMats[512];
}

// ��Z�X�t�B�A�}�b�v
Texture2D<float4> sph : register(t1); //1 �ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��

// ��Z�X�t�B�A�}�b�v
Texture2D<float4> spa : register(t2); //2 �ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��

// �g�D�[��
Texture2D<float4> toon : register(t3); //3 �ԃX���b�g�ɐݒ肳�ꂽ�e�N�X�`��(�g�D�[��)
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

    int weight : WEIGHTTYPE;

    float2 boneno1 : BONENO1;
    float2 boneno2 : BONENO2;
    float2 boneno3 : BONENO3;
    float2 boneno4 : BONENO4;

    int weight1 : WEIGHT1;
    int weight2 : WEIGHT2;
    int weight3 : WEIGHT3;
    int weight4 : WEIGHT4;
};

// ���_�V�F�[�_
Out vs( float3 pos : POSITION,
        float2 uv  : TEXCOORD,
        float3 normal : NORMAL,
        float2 boneno : BONENO,
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

    // ���ޯ�ޗp
    o.weight = weight;

	return o;
}

// �s�N�Z���V�F�[�_
float4 ps(Out o):SV_TARGET
{
    // �����x�N�g��
    float3 eye = float3(0, 20, -20);
    float3 ray = o.pos.xyz - eye;

    float3 light = normalize(float3(1, -1, 1)); //���̌������x�N�g��(���s����)

    //�f�B�t���[�Y�v�Z
    float diffuseB = saturate(dot(-light, o.normal));
    float4 toonDif = toon.Sample(smpToon, float2(0, 1 - diffuseB));

    //���̔��˃x�N�g��
    float3 refLight = normalize(reflect(light, o.normal.xyz));
    float specularB = pow(saturate(dot(refLight, -ray)), specular.a);

    //�X�t�B�A�}�b�v�pUV
    float2 sphereMapUV = o.vnormal.xy;
    sphereMapUV = (sphereMapUV + float2(1, -1)) * float2(0.5, -0.5);

    float4 texColor = tex.Sample(smp, o.uv); //�e�N�X�`���J���[

    return saturate(texColor * diffuse * toonDif);
                + specularB;

    return saturate(toonDif * diffuse * texColor * sph.Sample(smp, sphereMapUV))
            + spa.Sample(smp, sphereMapUV) * texColor
            + saturate(specularB * specular)
            + float4(texColor.rgb * ambient*0.2, texColor.a);
}