#line 1 "environment.fx"

shared float4x4 ModelViewProjection;
shared float4x4 Model;
shared float3   ModelEyePos;

float3 ModelLightPos;

int CullMode = 2;
int AlphaSrcBlend = 5;
int AlphaDstBlend = 6;
bool  AlphaBlendEnable = true;

texture DiffMap0;
texture BumpMap0;
texture CubeMap0;

#include "shaders:../lib/lib.fx"

//------------------------------------------------------------------------------
/**
    shader input/output structure definitions
*/
struct vsOutputEnvironmentColor
{
    float4 position      : POSITION;
    float2 uv0           : TEXCOORD0;
    float3 lightVec      : TEXCOORD1;
    float3 modelLightVec : TEXCOORD2;
    float3 halfVec       : TEXCOORD3;
    float3 worldReflect  : TEXCOORD4;
    float3 eyePos        : TEXCOORD5;
    DECLARE_SCREENPOS(TEXCOORD6)
};

struct vsInputStaticColor
{
    float4 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv0      : TEXCOORD0;
    float3 tangent  : TANGENT;
    float3 binormal : BINORMAL;
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

sampler EnvironmentSampler = sampler_state
{
    Texture = <CubeMap0>;
    AddressU  = Wrap;
    AddressV  = Wrap;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
};

//------------------------------------------------------------------------------
/**
    Vertex shader for environment mapping effect.
*/
vsOutputEnvironmentColor vsEnvironmentColor(const vsInputStaticColor vsIn)
{
    vsOutputEnvironmentColor vsOut;
    vsOut.position  = mul(vsIn.position, ModelViewProjection);
    VS_SETSCREENPOS(vsOut.position);
    vsOut.uv0 = vsIn.uv0;
    vsLight(vsIn.position, vsIn.normal, vsIn.tangent, vsIn.binormal, ModelEyePos, ModelLightPos, vsOut.lightVec, vsOut.modelLightVec, vsOut.halfVec, vsOut.eyePos);

    // compute a model space reflection vector
    float3 modelEyeVec = normalize(vsIn.position.xyz - ModelEyePos);
    float3 modelReflect = reflect(modelEyeVec, vsIn.normal);

    // transform model space reflection vector to view space
    vsOut.worldReflect = mul(modelReflect, (float3x3)Model);

    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Pixel shader for environment mapping effect.
*/
color4 psEnvironmentColor(const vsOutputEnvironmentColor psIn, uniform bool hdr, uniform bool shadow) : COLOR
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
    color4 reflectColor = texCUBE(EnvironmentSampler, psIn.worldReflect);
    color4 color = lerp(diffColor, reflectColor, diffColor.a);
    color4 finalColor = psLight(color, tangentSurfaceNormal, psIn.lightVec, psIn.modelLightVec, psIn.halfVec, shadowIntensity);
    if (hdr)
    {
        return EncodeHDR(finalColor);
    }
    else
    {
        return finalColor;
    }
#endif
}

//------------------------------------------------------------------------------
/**
    Pixel shader for alpha blended environment mapped geometry.
*/
color4 psEnvironmentAlphaColor(const vsOutputEnvironmentColor psIn, uniform bool hdr, uniform bool shadow) : COLOR
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
    color4 reflectColor = texCUBE(EnvironmentSampler, psIn.worldReflect);

    // FIXME: this throws a compiler error that the target and source
    // of a lerp() cannot be the same...
    //color4 color = lerp(diffColor, reflectColor, diffColor.a);
    color4 color = diffColor;

    color.a = diffColor.a;
    color4 baseColor = psLight(color, tangentSurfaceNormal, psIn.lightVec, psIn.modelLightVec, psIn.halfVec, shadowIntensity);
    if (hdr)
    {
        return EncodeHDR(baseColor);
    }
    else
    {
        return baseColor;
    }
#endif
}

//------------------------------------------------------------------------------
/**
    Techniques for shader "environment"
*/
technique tEnvironmentColor
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsEnvironmentColor();
        PixelShader  = compile PS_PROFILE psEnvironmentColor(false, false);
    }
}

technique tEnvironmentColorHDR
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsEnvironmentColor();
        PixelShader  = compile PS_PROFILE psEnvironmentColor(true, false);
    }
}

technique tEnvironmentColorShadow
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsEnvironmentColor();
        PixelShader  = compile PS_PROFILE psEnvironmentColor(false, true);
    }
}

technique tEnvironmentColorHDRShadow
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile VS_PROFILE vsEnvironmentColor();
        PixelShader  = compile PS_PROFILE psEnvironmentColor(true, true);
    }
}

//------------------------------------------------------------------------------
/**
    Techniques for shader "environment_alpha"
*/
technique tEnvironmentAlphaColor
{
    pass p0
    {
        CullMode  = <CullMode>;
        SrcBlend  = <AlphaSrcBlend>;
        DestBlend = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsEnvironmentColor();
        PixelShader  = compile PS_PROFILE psEnvironmentAlphaColor(false, false);
    }
}

technique tEnvironmentAlphaColorHDR
{
    pass p0
    {
        CullMode  = <CullMode>;
        SrcBlend  = <AlphaSrcBlend>;
        DestBlend = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsEnvironmentColor();
        PixelShader  = compile PS_PROFILE psEnvironmentAlphaColor(true, false);
    }
}

technique tEnvironmentAlphaColorShadow
{
    pass p0
    {
        CullMode  = <CullMode>;
        SrcBlend  = <AlphaSrcBlend>;
        DestBlend = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsEnvironmentColor();
        PixelShader  = compile PS_PROFILE psEnvironmentAlphaColor(false, true);
    }
}

technique tEnvironmentAlphaColorHDRShadow
{
    pass p0
    {
        CullMode  = <CullMode>;
        SrcBlend  = <AlphaSrcBlend>;
        DestBlend = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsEnvironmentColor();
        PixelShader  = compile PS_PROFILE psEnvironmentAlphaColor(true, true);
    }
}