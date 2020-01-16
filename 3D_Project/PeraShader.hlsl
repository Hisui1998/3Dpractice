//テクスチャ
Texture2D<float4> tex : register(t0);
//深度テクスチャ
Texture2D<float4> depth : register(t1);
//深度テクスチャライト
Texture2D<float4> lightdepth : register(t2);
//法線
Texture2D<float4> normal : register(t3);
//高輝度
Texture2D<float4> bloom : register(t4);
//ブルーム等の縮小バッファ
Texture2D<float4> shrink : register(t5);
//輪郭線抽出用影絵
Texture2D<float4> outline : register(t6);
//被写界深度用縮小バッファ
Texture2D<float4> scene : register(t7);
//SSAo
Texture2D<float> ssaotex : register(t8);

// サンプラ
SamplerState smp : register(s0);

// 色情報
cbuffer colors : register(b0)
{
    float4 bloomCol;
};
// huragu
cbuffer flags : register(b1)
{
    int GBuffer;
    int CenterLine;
    float _Time;
};

// ガウシアンぼかしの式
float4 GaussianFilteredColor5x5(Texture2D<float4> intex,SamplerState smpst,float2 inuv,float dx, float dy)
{
    float4 ret = intex.Sample(smpst, inuv)*36;
    
    ret += intex.Sample(smp, inuv + float2(-2 * dx,  2 * dy)) * 1;
    ret += intex.Sample(smp, inuv + float2(-2 * dx, -2 * dy)) * 1;
    ret += intex.Sample(smp, inuv + float2( 2 * dx, -2 * dy)) * 1;
    ret += intex.Sample(smp, inuv + float2( 2 * dx,  2 * dy)) * 1;
    
    ret += intex.Sample(smp, inuv + float2(-1 * dx,  2 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2( 1 * dx,  2 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2(-2 * dx,  1 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2( 2 * dx,  1 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2( 2 * dx, -1 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2(-1 * dx, -2 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2( 1 * dx, -2 * dy)) * 4;
    ret += intex.Sample(smp, inuv + float2(-2 * dx, -1 * dy)) * 4;
    
    ret += intex.Sample(smp, inuv + float2( 0 * dx,  2 * dy)) * 6;
    ret += intex.Sample(smp, inuv + float2(-2 * dx,  0 * dy)) * 6;
    ret += intex.Sample(smp, inuv + float2( 0 * dx, -2 * dy)) * 6;
    ret += intex.Sample(smp, inuv + float2( 2 * dx,  0 * dy)) * 6;
    
    ret += intex.Sample(smp, inuv + float2(-1 * dx,  1 * dy)) * 16;
    ret += intex.Sample(smp, inuv + float2( 1 * dx,  1 * dy)) * 16;
    ret += intex.Sample(smp, inuv + float2(-1 * dx, -1 * dy)) * 16;
    ret += intex.Sample(smp, inuv + float2( 1 * dx, -1 * dy)) * 16;
    
    ret += intex.Sample(smp, inuv + float2( 0 * dx,  1 * dy)) * 24;
    ret += intex.Sample(smp, inuv + float2(-1 * dx,  0 * dy)) * 24;    
    ret += intex.Sample(smp, inuv + float2( 1 * dx,  0 * dy)) * 24;
    ret += intex.Sample(smp, inuv + float2( 0 * dx, -1 * dy)) * 24;
    
    ret /= 256;
    return ret;
}

struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

//頂点シェーダ
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

// 係数を出す関数
float2 mod2(float2 a, float2 b)
{
    return a - b * floor(a / b);
}

// 座標をずらす関数
float2 divSpace2d(float2 pos, float interval, float offset = 0)
{
    return mod2(pos + offset, interval) - (0.5 * interval);
}

// 回転させる関数
float2 rotate2d(float2 pos, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return mul(float2x2(c, s, -s, c), pos);
}

// 球体の距離関数
float Sphere(float3 pos, float size)
{
    return length(pos) - size;
}

float3 MoveSphere(float3 pos)
{
    float interval = 5;
    pos.xz = float2(mod2(pos.zx, interval) - (0.5 * interval));
    pos.zy = float2(mod2(pos.yz, interval) - (0.5 * interval));
    return pos;
}

float GetSphereDist(float3 pos, float size)
{
    pos = MoveSphere(pos);
    
    return Sphere(pos,size);
}

// BOX系関数群-------------------------------------------------
// ボックスの距離関数
float Box(float3 pos, float3 size)
{
    return length(max(abs(pos) - size, 0.0));
}

// Boxを並べるたり動かす関数
float3 MoveBox(float3 pos)
{
    float interval = sin(_Time) + 5;
    pos.xz = divSpace2d(pos.zx, 5);
    pos.yz = divSpace2d(pos.zy, 5);
    //pos.xy = divSpace2d(pos.yx, interval);
    //pos.xz = rotate2d(pos.zx, _Time);
    //pos.yz = rotate2d(pos.zy, _Time);
    //pos.xy = rotate2d(pos.yx, _Time);
    
    float scale = 3;
    float3 offset = float3(1, 1, 1);
    for (int i = 0; i < 5; i++)
    {
        pos = abs(pos);
        if (pos.x < pos.y)
        {
            pos.xy = pos.yx;
        }
        if (pos.x < pos.z)
        {
            pos.xz = pos.zx;
        }
        if (pos.y < pos.z)
        {
            pos.yz = pos.zy;
        }
        pos *= scale;
        pos.xyz -= offset * (scale - 1.0);
        if (pos.z < -0.5 * offset.z * (scale - 1.0))
        {
            pos.z += offset.z * (scale - 1.0);
        }
    }
    return (length(max(abs(pos.xyz) - float3(1.0, 1.0, 1.0), 0.0)))/3;
    return pos;
}

// Boxのサイズを変更する関数
float3 ChangeSizeBox(float3 size)
{
    size.z = sin(_Time) + 1;
    size.y = cos(_Time) + 1;
    return size;
}

// ボックスまでの距離を取得する関数
float GetBoxDistance(float3 pos, float3 size)
{
	// ボックスの座標とサイズの更新
    pos = MoveBox(pos);
    //size = ChangeSizeBox(size);

    return Box(pos, size);
}
// Box系ここまで-------------------------------------------------

// ボックスの法線ベクトルを返す関数
float3 GetNormal(float3 pos, float3 size)
{
    float d = 0.001; // 法線を取得する範囲
    return normalize(
	float3(
		GetBoxDistance(pos + float3(d, 0, 0), size) - GetBoxDistance(pos + float3(-d, 0, 0), size),
		GetBoxDistance(pos + float3(0, d, 0), size) - GetBoxDistance(pos + float3(0, -d, 0), size),
		GetBoxDistance(pos + float3(0, 0, d), size) - GetBoxDistance(pos + float3(0, 0, -d), size)
		)
    );
}

//ピクセルシェーダ
float4 peraPS(Out o) : SV_Target
{
    float4 texColor = tex.Sample(smp, o.uv);
    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float dx = 1 / w;
    float dy = 1 / h;  
    
    float4 GaussTex = GaussianFilteredColor5x5(tex, smp, o.uv, dx, dy);
    float4 GaussScene = GaussianFilteredColor5x5(scene, smp, o.uv, dx, dy);
    
    // 輪郭線
    float edgesize = 2.f;
    float4 tc = outline.Sample(smp, o.uv);
    tc = tc * 4 -
    outline.Sample(smp, o.uv + float2(-dx * edgesize, 0)) -
    outline.Sample(smp, o.uv + float2(dx * edgesize, 0)) -
    outline.Sample(smp, o.uv + float2(0, dy * edgesize)) -
    outline.Sample(smp, o.uv + float2(0, -dy * edgesize));
    float edge = dot(float3(0.3f, 0.3f, 0.4f), 1 - tc.rgb);
    edge = pow(edge,10);
    if (CenterLine)
    {
        if ((o.uv.x <= 0.5005f) && (o.uv.x >= 0.4995f))
        {
            return 1-texColor;
        }
        else if ((o.uv.y <= 0.501f) && (o.uv.y >= 0.499f))
        {
            return 1 - texColor;
        }
    }
    if (GBuffer)
    {
        if ((o.uv.x <= 0.2) && (o.uv.y <= 0.2))
        {
        // カメラからの深度値
            float dep = depth.Sample(smp, o.uv * 5);
            dep = pow(dep, 100);
            return 1 - float4(dep, dep, dep, 1);
        }
        else if ((o.uv.x <= 0.2) && (o.uv.y <= 0.4))
        {
            float dep = lightdepth.Sample(smp, o.uv * 5);
            dep = pow(dep, 100);
            return 1 - float4(dep, dep, dep, 1);
        }
        else if ((o.uv.x <= 0.2) && (o.uv.y <= 0.6))
        {
            float4 dep = normal.Sample(smp, o.uv * 5);
            return dep;
        }
        else if ((o.uv.x <= 0.2) && (o.uv.y <= 0.8))
        {
            float4 dep = bloom.Sample(smp, o.uv * 5);
            return dep;
        }
        else if ((o.uv.x <= 0.2) && (o.uv.y <= 1.0))
        {
            float4 dep = shrink.Sample(smp, o.uv * 5);
            return dep;
        }
        else if ((o.uv.x <= 0.4) && (o.uv.y <= 0.2))
        {
            float4 dep = GaussianFilteredColor5x5(scene, smp, o.uv * 5, dx, dy);
            return dep;
        }
        else if ((o.uv.x <= 0.4) && (o.uv.y <= 0.4))
        {
            float4 dep = ssaotex.Sample(smp, o.uv * 5);
            return dep;
        }
    }
         
    
    if (texColor.a <= 0.2f)
    {
        float3 pos = o.pos.rgb;
        float3 start = float3(0, 0, -2.5);
        float m = min(w, h);
        float3 tpos = float3(pos.xy * float2(w / m, h / m), 0);
        float3 ray = normalize(tpos - start);
        float sphsize = 1.0f;
        float3 boxsize = float3(1, 1, 1);
        int marcingCnt = 0xff;
        
        for (int cnt = 0; cnt < marcingCnt; ++cnt)
        {            
            float len = GetBoxDistance(start, boxsize);
            start += ray * len;
            if (len < 0.001f)
            {
                return float4((float) (marcingCnt - cnt) / marcingCnt, (float) (marcingCnt - cnt) / marcingCnt, (float) (marcingCnt - cnt) / marcingCnt, 1) * bloomCol;
            }
        }
    }
    
    float4 oneBloom = GaussianFilteredColor5x5(bloom, smp, o.uv, dx, dy);
    float4 bloomSum = float4(0, 0, 0, 0);
    float2 uvSize = float2(1, 0.5);
    float2 uvOffset = float2(0, 0);
    for (int i = 0; i < 8; ++i)
    {
        bloomSum += GaussianFilteredColor5x5(shrink, smp, o.uv * uvSize + uvOffset, dx, dy);
        uvOffset.y += uvSize.y;
        uvSize *= 0.5f;
    }
    
    //画面真ん中からの深度の差を測る
    float depthDiff = abs(depth.Sample(smp, float2(0.5, 0.5)) - depth.Sample(smp, o.uv));
    depthDiff = pow(depthDiff, 0.5f);
    uvSize = float2(1, 0.5);
    uvOffset = float2(0, 0);
    float t = depthDiff * 8;
    float no;
    t = modf(t, no);// noには整数部が入って、返り値は余りの小数部が帰ってくる
    float4 retColor[2];
    
    retColor[0] = tex.Sample(smp, o.uv); //通常テクスチャ
    if (no == 0.0f)
    {
        retColor[1] = GaussianFilteredColor5x5(scene, smp, o.uv * uvSize + uvOffset, dx, dy);
    }
    else
    {
        for (int i = 1; i <= 8; ++i)
        {
            if (i - no < 0)
            {
                continue;
            }
            retColor[i - no] = GaussianFilteredColor5x5(scene, smp, o.uv * uvSize + uvOffset, dx, dy);
            uvOffset.y += uvSize.y;
            uvSize *= 0.5f;
            if (i - no > 1)
            {
                break;
            }
        }
    }
    
    return float4(1, edge, edge, edge)*lerp(retColor[0], retColor[1], t) + bloomCol * bloomSum;
    
    return float4(1, edge, edge, edge)*texColor + saturate((GaussianFilteredColor5x5(bloom, smp, o.uv, dx, dy) + bloomCol) * bloomSum);
    
    return GaussTex; // ガウスぼかし
    return float4(1, edge, edge, 1); // 輪郭線のみ
    return oneBloom;// 等倍ブルーム
    return float4(1, edge, edge, 1) * texColor; // 輪郭線+テクスチャカラー
}