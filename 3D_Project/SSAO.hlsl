//ñ@ê¸
Texture2D<float> Depth : register(t1);
Texture2D<float4> Normal : register(t3);

// ÉTÉìÉvÉâ
SamplerState smp : register(s0);

cbuffer sceneBuffer : register(b3)
{    
    matrix world;
    matrix view;
    matrix projection;
    matrix invproj;
    matrix wvp;
    matrix lvp;
    float3 lightPos;
}

float Randam(float2 inuv)
{
    return frac(sin(dot(inuv, float2(12.9898, 78.233))) * 43758.5453);
}

struct Input
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float SSAOPS(Input input):SV_Target
{
    float i = Normal.Sample(smp, input.uv).r + Normal.Sample(smp, input.uv).g + Normal.Sample(smp, input.uv).b;
    return i/3;
    return Randam(input.uv);
};