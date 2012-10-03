#line 2 "blended.fx"

shared float4x4 ModelViewProjection;
shared float3   ModelEyePos;

float3 ModelLightPos;

int VertexStreams;
float4 VertexWeights1;
float4 VertexWeights2;

bool  AlphaBlendEnable = true;
int AlphaRef;
int CullMode = 2;

texture DiffMap0;
texture BumpMap0;

#include "shaders:../lib/lib.fx"

//------------------------------------------------------------------------------
/**
    shader input/output structure definitions
*/
struct vsInputBlendedDepth
{
    float4 position0 : POSITION0;
    float4 position1 : POSITION1;
    float4 position2 : POSITION2;
    float4 position3 : POSITION3;
    float4 position4 : POSITION4;
    float4 position5 : POSITION5;
    float4 position6 : POSITION6;
    float4 position7 : POSITION7;
};

struct vsOutputStaticDepth
{
    float4 position : POSITION;
    float  depth : TEXCOORD0;
};

struct vsInputBlendedColor
{
    float4 position0 : POSITION0;
    float4 position1 : POSITION1;
    float4 position2 : POSITION2;
    float4 position3 : POSITION3;
    float4 position4 : POSITION4;
    float4 position5 : POSITION5;
    float4 position6 : POSITION6;
    float4 position7 : POSITION7;
    float3 normal0   : NORMAL0;
    float3 normal1   : NORMAL1;
    float3 normal2   : NORMAL2;
    float3 normal3   : NORMAL3;
    float3 normal4   : NORMAL4;
    float3 normal5   : NORMAL5;
    float3 normal6   : NORMAL6;
    float3 normal7   : NORMAL7;
    float2 uv0       : TEXCOORD0;
    float3 tangent   : TANGENT;
};

struct vsOutputStaticColor
{
    float4 position      : POSITION;
    float2 uv0           : TEXCOORD0;
    float3 lightVec      : TEXCOORD1;
    float3 modelLightVec : TEXCOORD2;
    float3 halfVec       : TEXCOORD3;
    float3 eyePos        : TEXCOORD4;
    DECLARE_SCREENPOS(TEXCOORD5)
};

struct vsOutputUvDepth
{
    float4 position : POSITION;
    float3 uv0depth : TEXCOORD0;
};

//------------------------------------------------------------------------------
/**
    sampler definitions
*/
sampler DiffSampler = sampler_state
{
    Texture = <DiffMap0>;
    AddressU  = Wrap;
    AddressV  = Wrap;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    MipMapLodBias = -0.75;
};

sampler BumpSampler = sampler_state
{
    Texture   = <BumpMap0>;
    AddressU  = Wrap;
    AddressV  = Wrap;
    MinFilter = Point;
    MagFilter = Linear;
    MipFilter = Linear;
    MipMapLodBias = -0.75;
};

//------------------------------------------------------------------------------
/**
    Vertex shader for blending (depth values).
*/
vsOutputStaticDepth vsBlendedDepth(const vsInputBlendedDepth vsIn)
{
    vsOutputStaticDepth vsOut;
    float4 pos = 0;
    if (VertexStreams > 0)
    {
        pos = vsIn.position0;
        if (VertexStreams > 1)
        {
            pos += (vsIn.position1-vsIn.position0) * VertexWeights1.y;
            if (VertexStreams > 2)
            {
                pos += (vsIn.position2-vsIn.position0) * VertexWeights1.z;
                if (VertexStreams > 3)
                {
                    pos += (vsIn.position3-vsIn.position0) * VertexWeights1.w;
                    if (VertexStreams > 4)
                    {
                        pos += (vsIn.position4-vsIn.position0) * VertexWeights2.x;
                        if (VertexStreams > 5)
                        {
                            pos += (vsIn.position5-vsIn.position0) * VertexWeights2.y;
                            if (VertexStreams > 6)
                            {
                                pos += (vsIn.position6-vsIn.position0) * VertexWeights2.z;
                            }
                        }
                    }
                }
            }
        }
    }
    vsOut.position = mul(pos, ModelViewProjection);
    vsOut.depth = vsOut.position.z;
    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Vertex shader for blending (color values).
*/
vsOutputStaticColor vsBlendedColor(const vsInputBlendedColor vsIn)
{
    vsOutputStaticColor vsOut;
    float4 pos = 0;
    float3 normal = 0;
    float3 tangent = 0;
    if (VertexStreams > 0)
    {
        pos = vsIn.position0;
        normal = vsIn.normal0;
        if (VertexStreams > 1)
        {
            pos += (vsIn.position1 - vsIn.position0) * VertexWeights1.y;
            normal += (vsIn.normal1 - vsIn.normal0) * VertexWeights1.y;
            if (VertexStreams > 2)
            {
                pos += (vsIn.position2 - vsIn.position0) * VertexWeights1.z;
                normal += (vsIn.normal2 - vsIn.normal0) * VertexWeights1.z;
                if (VertexStreams > 3)
                {
                    pos += (vsIn.position3 - vsIn.position0) * VertexWeights1.w;
                    normal += (vsIn.normal3 - vsIn.normal0) * VertexWeights1.w;
                    if (VertexStreams > 4)
                    {
                        pos += (vsIn.position4 - vsIn.position0) * VertexWeights2.x;
                        normal += (vsIn.normal4 - vsIn.normal0) * VertexWeights2.x;
                        if (VertexStreams > 5)
                        {
                            pos += (vsIn.position5 - vsIn.position0) * VertexWeights2.y;
                            normal += (vsIn.normal5 - vsIn.normal0) * VertexWeights2.y;
                            if (VertexStreams > 6)
                            {
                                pos += (vsIn.position6 - vsIn.position0) * VertexWeights2.z;
                                normal += (vsIn.normal6 - vsIn.normal0) * VertexWeights2.z;
                            }
                        }
                    }
                }
            }
        }
    }
    vsOut.position = mul(pos, ModelViewProjection);
    VS_SETSCREENPOS(vsOut.position);
    vsOut.uv0 = vsIn.uv0;
    vsLight(pos, normal, vsIn.tangent, cross(normal,vsIn.tangent), ModelEyePos, ModelLightPos, vsOut.lightVec, vsOut.modelLightVec, vsOut.halfVec, vsOut.eyePos);
    return vsOut;
}

//------------------------------------------------------------------------------
/**
*/
color4 psStaticDepth(const vsOutputStaticDepth psIn) : COLOR
{
    return float4(psIn.depth, 0.0f, 0.0f, 1.0f);
}

//------------------------------------------------------------------------------
/**
    Pixel shader: generate color values for static geometry.
*/
color4 psStaticColor(const vsOutputStaticColor psIn, uniform bool hdr, uniform bool shadow) : COLOR
{
#if DEBUG_LIGHTCOMPLEXITY
    return float4(0.05, 0.0f, 0.0f, 1.0f);
#else
    half shadowIntensity = shadow ? psShadow(PS_SCREENPOS) : 1.0;
    half2 uvOffset = half2(0.0f, 0.0f);
    if (BumpScale != 0.0f)
    {
        uvOffset = ParallaxUv(psIn.uv0, BumpSampler, psIn.eyePos);
    }
    color4 diffColor = tex2D(DiffSampler, psIn.uv0 + uvOffset);
    float3 tangentSurfaceNormal = (tex2D(BumpSampler, psIn.uv0 + uvOffset).rgb * 2.0f) - 1.0f;

    color4 baseColor = psLight(diffColor, tangentSurfaceNormal, psIn.lightVec, psIn.modelLightVec, psIn.halfVec, shadowIntensity);
    if (hdr)
    {
        return EncodeHDR(baseColor);
    }
    else
    {
        return baseColor;
    }
#endif
};

//------------------------------------------------------------------------------
/**
    Techniques for shader "blended"
*/
technique tBlendedDepth
{
    pass p0
    {
        CullMode = <CullMode>;
        VertexShader = compile VS_PROFILE vsBlendedDepth();
        PixelShader  = compile PS_PROFILE psStaticDepth();
    }
}

technique tBlendedColor
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsBlendedColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, false);
    }
}

technique tBlendedColorHDR
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsBlendedColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, false);
    }
}

technique tBlendedColorShadow
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsBlendedColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, true);
    }
}

technique tBlendedColorHDRShadow
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsBlendedColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, true);
    }
}