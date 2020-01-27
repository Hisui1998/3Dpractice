//�e�N�X�`��
Texture2D<float4> tex : register(t0);
//�[�x�e�N�X�`��
Texture2D<float4> depth : register(t1);
//�[�x�e�N�X�`�����C�g
Texture2D<float4> lightdepth : register(t2);
//�@��
Texture2D<float4> normal : register(t3);
//���P�x
Texture2D<float4> bloom : register(t4);
//�u���[�����̏k���o�b�t�@
Texture2D<float4> shrink : register(t5);
//�֊s�����o�p�e�G
Texture2D<float4> outline : register(t6);
//��ʊE�[�x�p�k���o�b�t�@
Texture2D<float4> scene : register(t7);
//SSAo
Texture2D<float> ssaotex : register(t8);

// �T���v��
SamplerState smp : register(s0);

// �F���
cbuffer colors : register(b0)
{
    float4 bloomCol;
    float4 marchCol;
};
// huragu
cbuffer flags : register(b1)
{
    int GBuffer;
    int CenterLine;
    float _Time;
    int MarchingCnt;
    int Sponge;
    int Modeloff;
    int BackOff;
};

// �K�E�V�A���ڂ����̎�
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

//���_�V�F�[�_
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

// �]��
float mod(float a, float b)
{
    return a - b * floor(a / b);
}
// �]��
float2 mod2(float2 a, float2 b)
{
    return a - b * floor(a / b);
}

float3 mod3(float3 a, float b)
{
    return a - b * floor(a / b);
}

// ���W�����炷�֐�
float2 divSpace2d(float2 pos, float interval, float offset = 0)
{
    return mod2(pos + offset, interval) - (0.5 * interval);
}

// ���W�����炷�֐�
float3 divSpace3d(float3 pos, float interval, float offset = 0)
{
    return mod3(pos + offset, interval) - (0.5 * interval);
}

// ��]������֐�
float2 rotate2d(float2 pos, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return mul(float2x2(c, -s, s, c), pos);
}

// ���̂̋����֐�
float Sphere(float3 pos, float size)
{
    return length(pos) - size;
}

float3 MoveSphere(float3 pos)
{
    float interval = 3;
    //pos = divSpace3d(pos, interval);
    pos.xz = divSpace2d(pos.zx, 3);
    pos.yz = divSpace2d(pos.zy, 3);
    return pos;
}

float GetSphereDist(float3 pos, float size)
{
    pos = MoveSphere(pos);
    
    return Sphere(pos,size);
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
    float interval = sin(_Time) + 5;
    //pos = divSpace3d(pos, 3);
    pos.xz = divSpace2d(pos.zx, 3);
    pos.yz = divSpace2d(pos.zy, 3);
    //pos.xy = divSpace2d(pos.yx, interval);
    pos.xz = rotate2d(pos.zx, _Time);
    pos.yz = rotate2d(pos.zy, _Time);
    pos.xy = rotate2d(pos.xy, _Time);  
    
    return pos;
}

// Box�̃T�C�Y��ύX����֐�
float3 ChangeSizeBox(float3 size)
{
    size.z = sin(_Time) + 1;
    size.y = cos(_Time) + 1;
    return size;
}

float sdCross(float3 p, float c)
{
    p = abs(p);
    float dxy = max(p.x, p.y);
    float dyz = max(p.y, p.z);
    float dxz = max(p.x, p.z);
    return min(dxy, min(dyz, dxz)) - c;
}

// ����������
float Hole(float3 p, float3 b)
{
    p = abs(p) - b;
    return length(max(p, 0.0)) + min(max(p.x, max(p.y, p.z)), 0.0);
}

// �{�b�N�X�ɑ΂��Č���������
float BoxToSponge(float3 pos, float3 size)
{
    float d = Hole(pos, size);
    float s = 1 / size;
    for (int i = 0; i < Sponge; i++)
    {
        float3 a = mod3(pos * s, 2.0) - 1.0;
        s *= 3.0;
        float3 r = 1.0 - 3.0 * abs(a);
        float c = sdCross(r, 1.0) / s;
        d = max(d, c);
    }
    return d;
}

// �{�b�N�X�܂ł̋������擾����֐�
float GetBoxDistance(float3 pos, float3 size)
{
	// �{�b�N�X�̍��W�ƃT�C�Y�̍X�V
    pos = MoveBox(pos);
    //size = ChangeSizeBox(size);
    
    if (Sponge)
    {
        // �X�|���W�ɂ���
        return BoxToSponge(pos, size);
    }
    
    return Box(pos, size);
}
// Box�n�����܂�-------------------------------------------------

// �{�b�N�X�̖@���x�N�g����Ԃ��֐�
float3 GetBoxNormal(float3 pos, float3 size)
{
    float d = 0.0001; // �@�����擾����͈�
    return normalize(
	float3(
		GetBoxDistance(pos + float3(d, 0, 0), size) - GetBoxDistance(pos + float3(-d, 0, 0), size),
		GetBoxDistance(pos + float3(0, d, 0), size) - GetBoxDistance(pos + float3(0, -d, 0), size),
		GetBoxDistance(pos + float3(0, 0, d), size) - GetBoxDistance(pos + float3(0, 0, -d), size)
		)
    );
}

// ���̖̂@���x�N�g����Ԃ��֐�
float3 GetSphNormal(float3 pos, float size)
{
    float d = 0.0001; // �@�����擾����͈�
    return normalize(
	float3(
		GetSphereDist(pos + float3(d, 0, 0), size) - GetSphereDist(pos + float3(-d, 0, 0), size),
		GetSphereDist(pos + float3(0, d, 0), size) - GetSphereDist(pos + float3(0, -d, 0), size),
		GetSphereDist(pos + float3(0, 0, d), size) - GetSphereDist(pos + float3(0, 0, -d), size)
		)
    );
}

//�s�N�Z���V�F�[�_
float4 peraPS(Out o) : SV_Target
{
    float4 texColor = tex.Sample(smp, o.uv);
    float w, h, level;
    tex.GetDimensions(0, w, h, level);
    float dx = 1 / w;
    float dy = 1 / h;  
    
    float4 GaussTex = GaussianFilteredColor5x5(tex, smp, o.uv, dx, dy);
    float4 GaussScene = GaussianFilteredColor5x5(scene, smp, o.uv, dx, dy);
    
    // �֊s��
    float edgesize = 1.5f;
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
        // �J��������̐[�x�l
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
    if (MarchingCnt && (BackOff==0))
    {
        if ((texColor.a <= 0.2f) || (Modeloff != 0))
        {
            float sphsize = 0.5; // ���̂̃T�C�Y
            float3 boxsize = float3(0.5, 0.5, 0.5); // ���̃T�C�Y
            
            float3 pos = o.pos.rgb; // ���W
            float3 start = float3(0, 0, -2.5); // ���C�̔�΂���[
            float m = min(w, h); // �A�X�y�N�g�䒲���p
            float3 tpos = float3(pos.xy * float2(w / m, h / m), 0); // �^�[�Q�b�g�̃|�W�V����
            float3 ray = normalize(tpos - start); // ���C�̃x�N�g�� 
            
            float3 light = normalize(float3(1, 1, -1));
            float4 col = float4(0,0,0,1);
            
            light.xz = rotate2d(light.xz, sin(_Time));
            light.yz = rotate2d(light.yz, cos(_Time));
            ray.xz = rotate2d(ray.xz, sin(_Time));
            ray.yz = rotate2d(ray.yz, cos(_Time));
            
            int marcingCnt = MarchingCnt;
        
            for (int cnt = 0; cnt < marcingCnt; ++cnt)
            {
                float len = GetBoxDistance(start, boxsize);
                float len2 = GetSphereDist(start, sphsize);
                
                len = min(len, len2);
                start += ray * len;
                if (len < 0.001f)
                {
                    float3 normal = GetSphNormal(start, sphsize);
                    if (len < len2)
                    {
                        normal = GetBoxNormal(start, boxsize);
                    }
                    float3 halfVec = normalize(-light - start);
                    float diffuse = dot(normal, light);
                    float specular = pow(clamp(dot(normal, halfVec), 0.0, 1.0), 50.0);
                    col = float4(abs(sin(_Time)), abs(sin(_Time) + cos(_Time) - 1), abs(cos(_Time)), 1) * diffuse * marchCol + specular;
                }
            }
            return col;
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
    
    //��ʐ^�񒆂���̐[�x�̍��𑪂�
    float depthDiff = abs(depth.Sample(smp, float2(0.5, 0.5)) - depth.Sample(smp, o.uv));
    depthDiff = pow(depthDiff, 0.5f);
    uvSize = float2(1, 0.5);
    uvOffset = float2(0, 0);
    float t = depthDiff * 8;
    float no;
    t = modf(t, no);// no�ɂ͐������������āA�Ԃ�l�͗]��̏��������A���Ă���
    float4 retColor[2];
    
    retColor[0] = tex.Sample(smp, o.uv); //�ʏ�e�N�X�`��
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
    
    return GaussTex; // �K�E�X�ڂ���
    return float4(1, edge, edge, 1); // �֊s���̂�
    return oneBloom;// ���{�u���[��
    return float4(1, edge, edge, 1) * texColor; // �֊s��+�e�N�X�`���J���[
}