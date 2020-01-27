//法線
Texture2D<float> Depth : register(t1);
Texture2D<float4> Normal : register(t3);

// サンプラ
SamplerState smp : register(s0);

cbuffer sceneBuffer : register(b2)
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
    float dp = Depth.Sample(smp, input.uv);
    dp = pow(dp, 100);
    
    float w, h, miplevels;
    Depth.GetDimensions(0, w, h, miplevels);
    float dx = 1.0f / w;
    float dy = 1.0f / h;
    
    //SSAO
    float4 respos = mul(invproj, float4(input.uv * float2(2, -2) + float2(-1, 1), dp, 1));
    respos.xyz = respos.xyz / respos.w;
    float div = 0.0f;
    float ao = 0.0f;
    float3 norm = normalize((Normal.Sample(smp, input.uv).xyz * 2) - 1);
    
    const float radius = 0.5f;
    if (dp < 1.0f)
    {
        for (int i = 0; i < 256; ++i)
        {
            float rnd1 = Randam(float2(i * dx, i * dy)) * 2 - 1;
            float rnd2 = Randam(float2(rnd1, i * dy)) * 2 - 1;
            float rnd3 = Randam(float2(rnd2, rnd1)) * 2 - 1;
            float3 omega = normalize(float3(rnd1, rnd2, rnd3));
            omega = normalize(omega);
            
            float dt = dot(norm, omega);
            float sgn = sign(dt);
            omega *= sign(dt);
            
            float4 rpos = mul(projection, float4(respos.xyz + omega * radius, 1));
            rpos.xyz /= rpos.w;
            dt *= sgn; //正の値にしてcosθを取得する
            div += dt; //遮蔽を考えない結果を加算する
            ao += step(
                Depth.Sample(smp, (rpos.xy + float2(1, -1)) * float2(0.5f, -0.5f)),
                rpos.z
            ) * dt;
        }
    }
    return 1.0f - ao;
};