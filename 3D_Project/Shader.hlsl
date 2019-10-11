Texture2D<float4> tex : register(t0);

SamplerState smp : register(s0);

cbuffer mat : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
    matrix wvp;
    matrix lvp;
};

cbuffer material : register(b1)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
}


struct Out
{
	float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
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
	return o;
}

// �s�N�Z���V�F�[�_
float4 ps(Out o):SV_TARGET
{
    float3 l = normalize(float3(1, -1, 1));
    float b = dot(-l, o.normal);
    return float4(b, b, b, 1) * diffuse * tex.Sample(smp, o.uv);

    /*
    // ����
    float3 light = float3(-1, 1, -1);
    light = normalize(light);

    // ���̖@���x�N�g��
    float3 mirror = reflect(light,o.normal);

    // ����
    float3 ray = float3(0,0,0);

    // ���ˌ�
    float spec = saturate(dot(reflect(light, o.normal),ray));
    spec = pow(spec, specular.a);

    // �e
    float brightness = saturate(dot(light, o.normal.xyz));

    // �Ԃ茌
    return float4(saturate(brightness + specular.xyz * spec + ambient.xyz), 1)                    * diffuse * tex.Sample(smp,o.uv);    */
    /*�@�ߋ��Ɏg�����Ԃ茌�����@*/
    //return float4(ambient, 1);
    //return float4(1, 1, 1, 1);
    //return float4(britness, britness, britness, 1);
    //return float4(diffuse, 1);
    //return float4(brightness, brightness, brightness, 1) * diffuse;
}