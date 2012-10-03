#line 2 "static.fx"

shared float3   ModelEyePos;

float3 ModelLightPos;

shared float4x4 ModelViewProjection;

bool  AlphaBlendEnable = true;
int AlphaRef;
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
struct vsInputStaticDepth
{
    float4 position : POSITION;
};

struct vsOutputStaticDepth
{
    float4 position : POSITION;
    float  depth : TEXCOORD0;
};

struct vsInputStaticColor
{
    float4 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv0      : TEXCOORD0;
    float3 tangent  : TANGENT;
    float3 binormal : BINORMAL;
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

struct vsInputUvDepth
{
    float4 position : POSITION;
    float2 uv0 : TEXCOORD0;
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
    Vertex shader: generate depth values for static geometry.
*/
vsOutputStaticDepth vsStaticDepth(const vsInputStaticDepth vsIn)
{
    vsOutputStaticDepth vsOut;
    vsOut.position = mul(vsIn.position, ModelViewProjection);
    vsOut.depth = vsOut.position.z;
    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Vertex shader: generate color values for static geometry.
*/
vsOutputStaticColor vsStaticColor(const vsInputStaticColor vsIn)
{
    vsOutputStaticColor vsOut;
    vsOut.position = mul(vsIn.position, ModelViewProjection);
    VS_SETSCREENPOS(vsOut.position);
    vsOut.uv0 = vsIn.uv0;
    vsLight(vsIn.position, vsIn.normal, vsIn.tangent, vsIn.binormal, ModelEyePos, ModelLightPos, vsOut.lightVec, vsOut.modelLightVec, vsOut.halfVec, vsOut.eyePos);
    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Vertex shader: generate depth values for static geometry with uv
    coordinates.
*/
vsOutputUvDepth vsStaticUvDepth(const vsInputUvDepth vsIn)
{
    vsOutputUvDepth vsOut;
    vsOut.position = mul(vsIn.position, ModelViewProjection);
    vsOut.uv0depth.xy = vsIn.uv0;
    vsOut.uv0depth.z = vsOut.position.z;
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
    Pixel shader: generate depth values for static geometry with
    alpha test.
*/
color4 psStaticDepthATest(const vsOutputUvDepth psIn) : COLOR
{
    float alpha = tex2D(DiffSampler, psIn.uv0depth.xy).a;
    clip(alpha - (AlphaRef / 256.0f));
    return float4(psIn.uv0depth.z, 0.0f, 0.0f, alpha);
}

//------------------------------------------------------------------------------
/**
    Techniques for shader "static"
*/
technique tStaticDepth
{
    pass p0
    {
        CullMode     = <CullMode>;
        VertexShader = compile VS_PROFILE vsStaticDepth();
        PixelShader  = compile PS_PROFILE psStaticDepth();
    }
}

technique tStaticColor
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsStaticColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, false);
    }
}

technique tStaticColorHDR
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsStaticColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, false);
    }
}

technique tStaticColorShadow
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsStaticColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, true);
    }
}

technique tStaticColorHDRShadow
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsStaticColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, true);
    }
}

//------------------------------------------------------------------------------
/**
    Techniques for shader "static_atest".
*/
technique tStaticDepthATest
{
    pass p0
    {
        CullMode     = <CullMode>;
        AlphaRef     = <AlphaRef>;
        VertexShader = compile VS_PROFILE vsStaticUvDepth();
        PixelShader  = compile PS_PROFILE psStaticDepthATest();
    }
}

technique tStaticColorATest
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        AlphaRef     = <AlphaRef>;
        VertexShader = compile VS_PROFILE vsStaticColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, false);
    }
}

technique tStaticColorATestHDR
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        AlphaRef     = <AlphaRef>;
        VertexShader = compile VS_PROFILE vsStaticColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, false);
    }
}

technique tStaticColorATestShadow
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        AlphaRef     = <AlphaRef>;
        VertexShader = compile VS_PROFILE vsStaticColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, true);
    }
}

technique tStaticColorATestHDRShadow
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        AlphaRef     = <AlphaRef>;
        VertexShader = compile VS_PROFILE vsStaticColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, true);
    }
}

//------------------------------------------------------------------------------
/**
    Techniques for shader "alpha"
*/
technique tStaticAlphaColor
{
    pass p0
    {
        CullMode     = <CullMode>;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsStaticColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, false);
    }
}

technique tStaticAlphaColorHDR
{
    pass p0
    {
        CullMode     = <CullMode>;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsStaticColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, false);
    }
}

technique tStaticAlphaColorShadow
{
    pass p0
    {
        CullMode     = <CullMode>;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsStaticColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, true);
    }
}

technique tStaticAlphaColorHDRShadow
{
    pass p0
    {
        CullMode     = <CullMode>;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsStaticColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, true);
    }
}
