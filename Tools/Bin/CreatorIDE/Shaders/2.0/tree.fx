#line 2 "tree.fx"

shared float4x4 ModelViewProjection;
shared float3   ModelEyePos;

float3 ModelLightPos;

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

//------------------------------------------------------------------------------
/**
    Vertex shader for tree pixel depth.
*/
vsOutputUvDepth vsTreeDepth(const vsInputUvDepth vsIn)
{
    vsOutputUvDepth vsOut;

    float4 position = ComputeSwayedPosition(vsIn.position);
    vsOut.position = mul(position, ModelViewProjection);
    vsOut.uv0depth.xy = vsIn.uv0;
    vsOut.uv0depth.z  = vsOut.position.z;
    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Vertex shader for tree pixel color.
*/
vsOutputStaticColor vsTreeColor(const vsInputStaticColor vsIn)
{
    vsOutputStaticColor vsOut;

    // compute lerp factor based on height above min y
    float ipol = (vsIn.position.y - BoxMinPos.y) / (BoxMaxPos.y - BoxMinPos.y);

    // compute rotated vertex position, normal and tangent in model space
    float4 rotPosition  = float4(mul(Swing, vsIn.position), 1.0f);
    float3 rotNormal    = mul(Swing, vsIn.normal);
    float3 rotTangent   = mul(Swing, vsIn.tangent);

    // lerp between original and rotated pos
    float4 lerpPosition = lerp(vsIn.position, rotPosition, ipol);
    float3 lerpNormal   = lerp(vsIn.normal, rotNormal, ipol);
    float3 lerpTangent  = lerp(vsIn.tangent, rotTangent, ipol);

    // transform vertex position
    vsOut.position = transformStatic(lerpPosition, ModelViewProjection);
    VS_SETSCREENPOS(vsOut.position);
    vsOut.uv0 = vsIn.uv0;
    vsLight(vsIn.position, vsIn.normal, vsIn.tangent, vsIn.binormal, ModelEyePos, ModelLightPos, vsOut.lightVec, vsOut.modelLightVec, vsOut.halfVec, vsOut.eyePos);

    return vsOut;
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
    Techniques for tree renderer.
*/
technique tTreeDepth
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaRef     = <AlphaRef>;
        VertexShader = compile VS_PROFILE vsTreeDepth();
        PixelShader  = compile PS_PROFILE psStaticDepthATest();
    }
}

technique tTreeColor
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        AlphaRef     = <AlphaRef>;
        VertexShader = compile VS_PROFILE vsTreeColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, false);
    }
}

technique tTreeColorHDR
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        AlphaRef     = <AlphaRef>;
        VertexShader = compile VS_PROFILE vsTreeColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, false);
    }
}

technique tTreeColorShadow
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        AlphaRef     = <AlphaRef>;
        VertexShader = compile VS_PROFILE vsTreeColor();
        PixelShader  = compile PS_PROFILE psStaticColor(false, true);
    }
}

technique tTreeColorHDRShadow
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        AlphaRef     = <AlphaRef>;
        VertexShader = compile VS_PROFILE vsTreeColor();
        PixelShader  = compile PS_PROFILE psStaticColor(true, true);
    }
}
