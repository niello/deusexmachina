#line 2 "skinned.fx"

shared float4x4 ModelViewProjection;
shared float3   ModelEyePos;

float3 ModelLightPos;

bool  AlphaBlendEnable = true;

matrix<float,4,3> JointPalette[72];
int CullMode = 2;
int AlphaSrcBlend = 5;
int AlphaDstBlend = 6;

texture DiffMap0;
texture BumpMap0;

#include "shaders:../lib/lib.fx"

//------------------------------------------------------------------------------
/**
    shader input/output structure definitions
*/
struct vsInputSkinnedDepth
{
    float4 position : POSITION;
    float4 weights : BLENDWEIGHT;
    float4 indices : BLENDINDICES;
};

struct vsOutputStaticDepth
{
    float4 position : POSITION;
    float  depth : TEXCOORD0;
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

struct vsInputSkinnedColor
{
    float4 position : POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float3 binormal : BINORMAL;
    float2 uv0      : TEXCOORD0;
    float4 weights  : BLENDWEIGHT;
    float4 indices  : BLENDINDICES;
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
    Vertex shader for skinning (write depth values).
*/
vsOutputStaticDepth vsSkinnedDepth(const vsInputSkinnedDepth vsIn)
{
    vsOutputStaticDepth vsOut;
    float4 skinPos = skinnedPosition(vsIn.position, vsIn.weights, vsIn.indices, JointPalette);
    vsOut.position = mul(skinPos, ModelViewProjection);
    vsOut.depth = vsOut.position.z;
    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Vertex shader for skinning (write color values).
*/
vsOutputStaticColor vsSkinnedColor(const vsInputSkinnedColor vsIn)
{
    vsOutputStaticColor vsOut;

    // get skinned position, normal and tangent
    float4 skinPos     = skinnedPosition(vsIn.position, vsIn.weights, vsIn.indices, JointPalette);
    vsOut.position     = mul(skinPos, ModelViewProjection);
    VS_SETSCREENPOS(vsOut.position);
    vsOut.uv0          = vsIn.uv0;
    float3 skinNormal  = skinnedNormal(vsIn.normal, vsIn.weights, vsIn.indices, JointPalette);
    float3 skinTangent = skinnedNormal(vsIn.tangent, vsIn.weights, vsIn.indices, JointPalette);
    float3 skinBinormal = skinnedNormal(vsIn.binormal, vsIn.weights, vsIn.indices, JointPalette);
    vsLight(skinPos, skinNormal, skinTangent, skinBinormal, ModelEyePos, ModelLightPos, vsOut.lightVec, vsOut.modelLightVec, vsOut.halfVec, vsOut.eyePos);
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
    Techniques for shader "skinned"
*/
technique tSkinnedDepth
{
    pass p0
    {
        CullMode = <CullMode>;
        VertexShader = compile VS_PROFILE vsSkinnedDepth();
        PixelShader  = compile PS_PROFILE psStaticDepth();
    }
}

technique tSkinnedColor
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsSkinnedColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, false);
    }
}

technique tSkinnedColorHDR
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsSkinnedColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, false);
    }
}

technique tSkinnedColorShadow
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsSkinnedColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, true);
    }
}

technique tSkinnedColorHDRShadow
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsSkinnedColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, true);
    }
}

//------------------------------------------------------------------------------
/**
    Techniques for shader "skinned alpha"
*/
technique tSkinnedAlphaColor
{
    pass p0
    {
        CullMode     = <CullMode>;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsSkinnedColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, false);
    }
}

technique tSkinnedAlphaColorHDR
{
    pass p0
    {
        CullMode     = <CullMode>;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsSkinnedColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, false);
    }
}

technique tSkinnedAlphaColorShadow
{
    pass p0
    {
        CullMode     = <CullMode>;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsSkinnedColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, true);
    }
}

technique tSkinnedAlphaColorHDRShadow
{
    pass p0
    {
        CullMode     = <CullMode>;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsSkinnedColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, true);
    }
}