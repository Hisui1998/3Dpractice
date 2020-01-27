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

// �W�����o���֐�
float2 mod2(float2 a, float2 b)
{
    return a - b * floor(a / b);
}

// ���W�����炷�֐�
float2 divSpace2d(float2 pos, float interval, float offset = 0)
{
    return mod2(pos + offset, interval) - (0.5 * interval);
}

// ��]������֐�
float2 rotate2d(float2 pos, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return mul(float2x2(c, s, -s, c), pos);
}

// BOX�n�֐��Q-------------------------------------------------
// �{�b�N�X�̋����֐�
float Box(float3 pos, float3 size)
{
    return length(max(abs(pos) - size, 0.0));
}


// Box����ׂ邽�蓮�����֐�
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

// Box�̃T�C�Y��ύX����֐�
float3 ChangeSizeBox(float3 size)
{
    //size.z = sin(_Time.y) + 1;
    //size.y = cos(_Time.y) + 1;
    return size;
}

// �{�b�N�X�܂ł̋������擾����֐�
float BoxDistance(float3 pos, float3 size)
{
	// �{�b�N�X�̍��W�ƃT�C�Y�̍X�V
    pos = MoveBox(pos);
    size = ChangeSizeBox(size);

    return Box(pos, size.x);
}
// Box�n�����܂�-------------------------------------------------


// Sphere�n�֐��Q------------------------------------------------
// ���̂̋����֐�
float Sphere(float3 pos, float size)
{
    return length(pos) - size;
}

// ���̂���ׂ邽�蓮�����֐�
float3 MoveSphere(float3 pos)
{
    float interval = 5;
	//pos.xz = divSpace2d(pos.zx, interval);
    //pos.yz = divSpace2d(pos.zy, interval);
	//pos.xy = divSpace2d(pos.yx, interval);
    return pos;
}

// ���̂̃T�C�Y��ύX����֐�
float3 ChangeSizeSphere(float3 size)
{
    //size.z = sin(_Time.y) + 1;
    //size.y = cos(_Time.y) + 1;
    return size;
}

// ���̂܂ł̋������擾����֐�
float SphereDistance(float3 pos, float size)
{
	// �{�b�N�X�̍��W�ƃT�C�Y�̍X�V
    pos = MoveSphere(pos);
    size = ChangeSizeSphere(size);

    return Sphere(pos, size);
}
// Sphere�n�����܂�-------------------------------------------------



// �{�b�N�X�̖@���x�N�g����Ԃ��֐�
float3 GetNormal(float3 pos, float3 size)
{
    float d = 0.001; // �@�����擾����͈�
    return normalize(
	float3(
		BoxDistance(pos + float3(d, 0, 0), size) - BoxDistance(pos + float3(-d, 0, 0), size),
		BoxDistance(pos + float3(0, d, 0), size) - BoxDistance(pos + float3(0, -d, 0), size),
		BoxDistance(pos + float3(0, 0, d), size) - BoxDistance(pos + float3(0, 0, -d), size)
		)
    );
}


// �F���擾����
float4 GetColor(float3 pos, float3 size, float3 rayVec, float totalDistance)
{
    half3 lightVec = pos.xyz; // ���C�g�x�N�g��
    half3 normal = GetNormal(pos, size); // �@���x�N�g��
    half3 reflectVec = reflect(-lightVec, normal); // ���C�g�̔��˃x�N�g��
    half3 halfVec = normalize(lightVec + -rayVec); // ���C�g�x�N�g�������������ꏊ�����

    half NdotL = saturate(dot(normal, lightVec)); // �@���ƃ��C�g�x�N�g���̓���
    half NdotH = saturate(dot(normal, halfVec)); // �@���Ɣ��˃x�N�g���̓���

    //float3 ambient = UNITY_LIGHTMODEL_AMBIENT.rgb * _DiffuseColor.rgb;
    //float3 diffuse = _LightColor0.rgb * _DiffuseColor * NdotL;
    //float3 specular = _LightColor0.rgb * _SpecularColor;

    float4 color;// = float4(ambient + diffuse + specular, 1);

    float fogFactor = saturate(totalDistance / 100);
    return color;
}

//�s�N�Z���V�F�[�_
float4 pera2PS(Out o) : SV_Target
{
    float3 pos = o.pos.xyz; // ���W
    float3 rayVec = normalize(pos.xyz - float3(0,0,0)); // �J��������̃��C�x�N�g��
    float3 size = float3(0.1,0.1,0.1);
    
    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    pos = pos * float3(w / h, 1, 1);
    
    
    const int marchCnt = 0xff; // ���C���s�i�������
    const float EPSILON = 0.1; // �\�ʂ̌���

    float4 color = 0;
    float totalDistance = 0; // ���C�̐i�񂾃g�[�^���̋���
    float maxRayDistance = 0xff; // ���C�̍Œ�����

    // ���C���s�i������
    for (int i = 0; i < marchCnt; ++i)
    {
		// ���C���i�񂾋���
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