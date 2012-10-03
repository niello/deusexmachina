//------------------------------------------------------------------------------
//  shaders.fx
//
//  Nebula2 standard shaders for shader model 2.0
//
//  (C) 2005 Radon Labs GmbH
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    shader parameters

    NOTE: VS_PROFILE and PS_PROFILE macros are usually provided by the
    application and contain the highest supported shader models.
*/
/*
#define VS_PROFILE vs_2_0
#define PS_PROFILE ps_2_0
*/
#line 2 "shaders.fx"

shared float4x4 ModelViewProjection;
shared float3   ModelEyePos;

shared float4x4 TextureTransform0;

float4x4 MLPUVStretch = {40.0f,  0.0f,  0.0f,  0.0f,
                         0.0f, 40.0f,  0.0f,  0.0f,
                         0.0f,  0.0f, 40.0f,  0.0f,
                         0.0f,  0.0f,  0.0f, 40.0f };

// detail texture scale for the layered shader, zoom by 40 as default
static const float4x4 DetailTexture = {40.0f,  0.0f,  0.0f,  0.0f,
                                        0.0f, 40.0f,  0.0f,  0.0f,
                                        0.0f,  0.0f, 40.0f,  0.0f,
                                        0.0f,  0.0f,  0.0f, 40.0f };

//static float w = 4.0f;      // width  for orthogonal projection
//static float h = 3.0f;      // height for orthogonal projection
//static float zn = -1.0f;    // z near for orthogonal projection
//static float zf = 1.0f;     // z far  for orthogonal projection
// orthogonal RH projection matrix, used by the 3d gui shader
/*
static const float4x4 OrthoProjection = {2/w,   0,      0,          0,
                                         0,     2/h,    0,          0,
                                         0,     0,      1/(zn-zf),  0,
                                         0,     0,      zn/(zn-zf), 1};
*/
static const float4x4 OrthoProjection = {0.5,   0,      0,      0,
                                         0,     0.6667, 0,      0,
                                         0,     0,      -0.5,   0,
                                         0,     0,       0.5,   1};

shared float Time;
shared float4 DisplayResolution;

float3 ModelLightPos;

shared float3   LightPos;

float CubeLightScale;

float4 Color0;


///??? NEEDED? (water shader used)
float4 TexGenS;                     // texgen parameters for u
float4 TexGenT;                     // texgen parameters for v

float4 Wind;                        // wind direction, strength in .w
float MinSpriteSize = 0.1f;
float MaxSpriteSize = 1.0f;
float3 RandPosScale = { 1234.5f, 3021.7f, 2032.1f };
float MinDist = 90.0f;
float MaxDist = 110.0f;

static const float MaxAtmoDist = 1000.0;

#include "shaders:../lib/lib.fx"
#include "shaders:../lib/randtable.fx"

//------------------------------------------------------------------------------
/**
    shader input/output structure definitions
*/

struct vsOutputRPLStaticColor
{
    float4 position         : POSITION;
    float2 uv0              : TEXCOORD0;
    float3 lightVec         : TEXCOORD1;
    float3 modelLightVec    : TEXCOORD2;
    float3 halfVec          : TEXCOORD3;
    float3 eyePos           : TEXCOORD4;
    DECLARE_SCREENPOS(TEXCOORD5)
};

//------------------------------------------------------------------------------
/**
    Vertex shader: generate color values for static geometry.
    With UV animation.
*/
vsOutputStaticColor vsUVAnimColor(const vsInputStaticColor vsIn)
{
    vsOutputStaticColor vsOut;
    vsOut.position = mul(vsIn.position, ModelViewProjection);
    VS_SETSCREENPOS(vsOut.position);

    // animate uv
    /*
    vsOut.uv0 = vsIn.uv0 + (Velocity.xy * Time);

    */
    vsOut.uv0 = mul(float3(vsIn.uv0, 1), TextureTransform0).xy;

    vsLight(vsIn.position, vsIn.normal, vsIn.tangent, vsIn.binormal, ModelEyePos, ModelLightPos, vsOut.lightVec, vsOut.modelLightVec, vsOut.halfVec, vsOut.eyePos);
    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Vertex shader: generate color values for static geometry.
    Modified for Radiosity Parallax Mapping.
    Add the tangentEyePos.
*/
vsOutputRPLStaticColor vsRPLStaticColor(const vsInputStaticColor vsIn)
{
     vsOutputRPLStaticColor vsOut;
     vsOut.position = mul(vsIn.position, ModelViewProjection);

    // The x axis of the UV-coordinates needs to be switched for parallaxmapping

    float2 tmpUv = vsIn.uv0;
    tmpUv.x = 1- tmpUv.x;
    vsOut.uv0 = tmpUv;

    //vsOut.uv0 = vsIn.uv0;

    // Compute the binormal and the tangentMatrix
    float3 binormal = cross(vsIn.normal, vsIn.tangent);
    float3x3 mytangentMatrix = float3x3(vsIn.tangent, binormal, vsIn.normal);

    // The ViewVector is needed for Parallaxmapping
    float3 ePos = ModelEyePos - vsIn.position;
    // Compute the Eyeposition in tangentspace
    VS_SETSCREENPOS(vsOut.position);
    vsLight(vsIn.position, vsIn.normal, vsIn.tangent,vsIn.binormal, ModelEyePos, ModelLightPos, vsOut.lightVec, vsOut.modelLightVec, vsOut.halfVec, vsOut.eyePos);
    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Pixel shader: generate color values for static geometry.
    Modified for Radiosity Parallax Lightmapping
*/
color4 psRPLStaticColor(const vsOutputRPLStaticColor psIn, uniform bool hdr, uniform bool shadow) : COLOR
{
#if DEBUG_LIGHTCOMPLEXITY
    return float4(0.05, 0.0f, 0.0f, 1.0f);
#else
    half shadowIntensity = shadow ? psShadow(PS_SCREENPOS) : 1.0;
    half2 uvOffset = float2(0.0f, 0.0f);
    if (BumpScale != 0.0f)
    {
        uvOffset = ParallaxUv(psIn.uv0, BumpSampler, psIn.eyePos);
    }
    color4 diffColor = tex2D(DiffSampler, psIn.uv0 + uvOffset);
    float3 tangentSurfaceNormal = (tex2D(BumpSampler, psIn.uv0 + uvOffset).rgb * 2.0f) - 1.0f;
    color4 baseColor = psLight(diffColor, tangentSurfaceNormal, psIn.lightVec, psIn.halfVec, psIn.modelLightVec, shadowIntensity);
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
