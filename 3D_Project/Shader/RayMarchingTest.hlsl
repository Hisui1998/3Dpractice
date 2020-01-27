//テクスチャ
Texture2D<float4> tex : register(t0);

// サンプラ
SamplerState smp : register(s0);

struct Out
{
    float4 pos : POSITION;
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};

//頂点シェーダ
Out pera2VS(
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

// BOX系関数群-------------------------------------------------
// ボックスの距離関数
float Box(float3 pos, float3 size)
{
    return length(max(abs(pos) - size, 0.0));
}


// Boxを並べるたり動かす関数
float3 MoveBox(float3 pos)
{
    float interval = 1;
	//pos.xz = divSpace2d(pos.zx, interval);
    //pos.yz = divSpace2d(pos.zy, interval);
    //pos.xy = divSpace2d(pos.yx, interval);
    //pos.xz = rotate2d(pos.zx, pos.y);
    //pos.yz = rotate2d(pos.zy, _Time.y);
    //pos.xy = rotate2d(pos.yx, _Time.y);
    return pos;
}

// Boxのサイズを変更する関数
float3 ChangeSizeBox(float3 size)
{
    //size.z = sin(_Time.y) + 1;
    //size.y = cos(_Time.y) + 1;
    return size;
}

// ボックスまでの距離を取得する関数
float BoxDistance(float3 pos, float3 size)
{
	// ボックスの座標とサイズの更新
    pos = MoveBox(pos);
    size = ChangeSizeBox(size);

    return Box(pos, size.x);
}
// Box系ここまで-------------------------------------------------


// Sphere系関数群------------------------------------------------
// 球体の距離関数
float Sphere(float3 pos, float size)
{
    return length(pos) - size;
}

// 球体を並べるたり動かす関数
float3 MoveSphere(float3 pos)
{
    float interval = 5;
	//pos.xz = divSpace2d(pos.zx, interval);
    //pos.yz = divSpace2d(pos.zy, interval);
	//pos.xy = divSpace2d(pos.yx, interval);
    return pos;
}

// 球体のサイズを変更する関数
float3 ChangeSizeSphere(float3 size)
{
    //size.z = sin(_Time.y) + 1;
    //size.y = cos(_Time.y) + 1;
    return size;
}

// 球体までの距離を取得する関数
float SphereDistance(float3 pos, float size)
{
	// ボックスの座標とサイズの更新
    pos = MoveSphere(pos);
    size = ChangeSizeSphere(size);

    return Sphere(pos, size);
}
// Sphere系ここまで-------------------------------------------------



// ボックスの法線ベクトルを返す関数
float3 GetNormal(float3 pos, float3 size)
{
    float d = 0.001; // 法線を取得する範囲
    return normalize(
	float3(
		BoxDistance(pos + float3(d, 0, 0), size) - BoxDistance(pos + float3(-d, 0, 0), size),
		BoxDistance(pos + float3(0, d, 0), size) - BoxDistance(pos + float3(0, -d, 0), size),
		BoxDistance(pos + float3(0, 0, d), size) - BoxDistance(pos + float3(0, 0, -d), size)
		)
    );
}


// 色を取得する
float4 GetColor(float3 pos, float3 size, float3 rayVec, float totalDistance)
{
    half3 lightVec = pos.xyz; // ライトベクトル
    half3 normal = GetNormal(pos, size); // 法線ベクトル
    half3 reflectVec = reflect(-lightVec, normal); // ライトの反射ベクトル
    half3 halfVec = normalize(lightVec + -rayVec); // ライトベクトルが当たった場所からの

    half NdotL = saturate(dot(normal, lightVec)); // 法線とライトベクトルの内積
    half NdotH = saturate(dot(normal, halfVec)); // 法線と反射ベクトルの内積

    //float3 ambient = UNITY_LIGHTMODEL_AMBIENT.rgb * _DiffuseColor.rgb;
    //float3 diffuse = _LightColor0.rgb * _DiffuseColor * NdotL;
    //float3 specular = _LightColor0.rgb * _SpecularColor;

    float4 color;// = float4(ambient + diffuse + specular, 1);

    float fogFactor = saturate(totalDistance / 100);
    return color;
}

//ピクセルシェーダ
float4 pera2PS(Out o) : SV_Target
{
    float3 pos = o.pos.xyz; // 座標
    float3 rayVec = normalize(pos.xyz - float3(0,0,0)); // カメラからのレイベクトル
    float3 size = float3(0.1,0.1,0.1);
    
    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    pos = pos * float3(w / h, 1, 1);
    
    
    const int marchCnt = 0xff; // レイを行進させる回数
    const float EPSILON = 0.1; // 表面の厚み

    float4 color = 0;
    float totalDistance = 0; // レイの進んだトータルの距離
    float maxRayDistance = 0xff; // レイの最長距離

    // レイを行進させる
    for (int i = 0; i < marchCnt; ++i)
    {
		// レイが進んだ距離
        float progress = BoxDistance(pos, size.x);

        float farFactor = (totalDistance / 150);
        if (maxRayDistance > progress + farFactor)
        {
            maxRayDistance = progress + farFactor;
        }
        if (progress > EPSILON)
        {
            pos.xyz += progress * rayVec.xyz;
            totalDistance += progress;
            continue;
        }
        color += float4(1, 0.5, 0, 1); //GetColor(pos, size, rayVec, totalDistance);
    }
    
    return color;
}