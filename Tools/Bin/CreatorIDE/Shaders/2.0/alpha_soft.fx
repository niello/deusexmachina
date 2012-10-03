#line 2 "alpha_soft.fx"

shared float4x4 ModelViewProjection;
shared float3   ModelEyePos;

float3 ModelLightPos;

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
struct vsInputStaticColor
{
    float4 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv0      : TEXCOORD0;
    float3 tangent  : TANGENT;
    float3 binormal : BINORMAL;
};

struct vsOutputSoftSilhouette
{
    float4 position      : POSITION;
    float3 uv0silhouette : TEXCOORD0;
    float3 lightVec      : TEXCOORD1;
    float3 modelLightVec : TEXCOORD2;
    float3 halfVec       : TEXCOORD3;
    float3 eyePos        : TEXCOORD4;
    DECLARE_SCREENPOS(TEXCOORD5)
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
    Vertex shader: generate vertices for soft silhouettes.
*/
vsOutputSoftSilhouette vsSoftSilhouette(const vsInputStaticColor vsIn)
{
    vsOutputSoftSilhouette vsOut;
    vsOut.position = mul(vsIn.position, ModelViewProjection);
    VS_SETSCREENPOS(vsOut.position);
    vsOut.uv0silhouette.xy = vsIn.uv0;
    vsLight(vsIn.position, vsIn.normal, vsIn.tangent,vsIn.binormal, ModelEyePos, ModelLightPos, vsOut.lightVec, vsOut.modelLightVec, vsOut.halfVec, vsOut.eyePos);

    // compute silhouette modulation color
    float3 eVec = normalize(ModelEyePos - vsIn.position);
    vsOut.uv0silhouette.z = abs(dot(vsIn.normal, eVec));
    vsOut.uv0silhouette.z *= vsOut.uv0silhouette.z;

    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Pixel shader: generate color values for soft silhouette geometry
*/
color4 psSoftSilhouette(const vsOutputSoftSilhouette psIn, uniform bool hdr, uniform bool shadow) : COLOR
{
#if DEBUG_LIGHTCOMPLEXITY
    return float4(0.05, 0.0f, 0.0f, 1.0f);
#else
    half shadowIntensity = shadow ? psShadow(PS_SCREENPOS) : 1.0;
    half2 uvOffset = half2(0.0f, 0.0f);
    if (BumpScale != 0.0f)
    {
        uvOffset = ParallaxUv(psIn.uv0silhouette.xy, BumpSampler, psIn.eyePos);
    }
    color4 diffColor = tex2D(DiffSampler, psIn.uv0silhouette.xy + uvOffset);
    float3 tangentSurfaceNormal = (tex2D(BumpSampler, psIn.uv0silhouette.xy + uvOffset).rgb * 2.0f) - 1.0f;

    color4 baseColor = psLight(diffColor, tangentSurfaceNormal, psIn.lightVec, psIn.modelLightVec, psIn.halfVec, shadowIntensity);
    baseColor *= psIn.uv0silhouette.z;
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
    Techniques for shader "alpha_soft"
*/
technique tSoftAlphaColor
{
    pass p0
    {
        CullMode     = <CullMode>;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsSoftSilhouette();
        PixelShader  = compile PS_PROFILE psSoftSilhouette(false, false);
    }
}

technique tSoftAlphaColorHDR
{
    pass p0
    {
        CullMode     = <CullMode>;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsSoftSilhouette();
        PixelShader  = compile PS_PROFILE psSoftSilhouette(true, false);
    }
}

technique tSoftAlphaColorShadow
{
    pass p0
    {
        CullMode     = <CullMode>;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsSoftSilhouette();
        PixelShader  = compile PS_PROFILE psSoftSilhouette(false, true);
    }
}

technique tSoftAlphaColorHDRShadow
{
    pass p0
    {
        CullMode     = <CullMode>;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        VertexShader = compile VS_PROFILE vsSoftSilhouette();
        PixelShader  = compile PS_PROFILE psSoftSilhouette(true, true);
    }
}