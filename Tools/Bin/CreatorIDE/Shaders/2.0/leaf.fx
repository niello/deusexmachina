#line 2 "leaf.fx"

shared float4x4 InvModelView;
shared float4x4 ModelViewProjection;
shared float3   ModelEyePos;

shared float Time;

float3 ModelLightPos;

float SpriteSwingAngle;             // swing angle modulator
float SpriteSwingTime;              // swing period
float InnerLightIntensity;          // light intensity at center
float OuterLightIntensity;          // light intensity at periphery
float3 BoxCenter;                   // model space bounding box center
static const float4 GenAngle = { 3.54824948311, -11.6819286346, 10.4263944626, -2.29271507263 };

int AlphaRef;
bool  AlphaBlendEnable = true;

texture DiffMap0;
texture BumpMap0;

#include "shaders:../lib/lib.fx"

//------------------------------------------------------------------------------
/**
    shader input/output structure definitions
*/
struct vsOutputUvDepth
{
    float4 position : POSITION;
    float3 uv0depth : TEXCOORD0;
};

struct vsInputLeafDepth
{
    float4 position : POSITION;
    float3 extrude  : NORMAL;           // sprite extrusion vector
    float2 uv0 : TEXCOORD0;
};

struct vsInputLeafColor
{
    float4 position : POSITION;
    float3 extrude  : NORMAL;           // sprite extrusion vector
    float2 uv0 : TEXCOORD0;
};

struct vsOutputLeafColor
{
    float4 position      : POSITION;
    float3 uv0intensity  : TEXCOORD0;
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
    Helper function for leaf vertex shaders to compute rotated, extruded
    leaf corner vertex in model space.

    @param  inPosition  input model space position
    @param  extrude     the extrude vector
    @return             output model space position
*/
float4 ComputeRotatedExtrudedLeafVertex(const float4 inPosition, const float3 extrude)
{
    float4 position = inPosition;

    // compute rotation angle
    float t = frac((Time / SpriteSwingTime) + position.x);
    float t2 = t * t;
    float t3 = t2 * t;
    float ang = SpriteSwingAngle * t * (GenAngle.x * t3 + GenAngle.y * t2 + GenAngle.z * t + GenAngle.w);

    // build rotation matrix
    float sinAng, cosAng;
    sincos(ang, sinAng, cosAng);
    float3x3 rot = {
        cosAng, -sinAng, 0.0f,
        sinAng,  cosAng, 0.0f,
        0.0f,    0.0f,   1.0f,
    };

    // rotate extrude vector and transform to model space
    float3 extrudeVec = mul(rot, extrude);
    float3 modelExtrudeVec = mul(extrudeVec, (float3x3)InvModelView);

    // extrude to corner position
    position.xyz += modelExtrudeVec.xyz;

    return position;
}

//------------------------------------------------------------------------------
/**
    Vertex shader: generate depth values for leafs.
*/
vsOutputUvDepth vsLeafDepth(const vsInputLeafDepth vsIn)
{
    vsOutputUvDepth vsOut;

    // compute swayed position in model space
    float4 position = ComputeSwayedPosition(vsIn.position);
    position = ComputeRotatedExtrudedLeafVertex(position, vsIn.extrude);

    // transform to projection space
    vsOut.position = mul(position, ModelViewProjection);
    vsOut.uv0depth.xy = vsIn.uv0;
    vsOut.uv0depth.z  = vsOut.position.z;

    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Vertex shader: generate color values for leafs.
*/
vsOutputLeafColor vsLeafColor(const vsInputLeafColor vsIn)
{
    vsOutputLeafColor vsOut;

    // compute swayed position in model space
    float4 position = ComputeSwayedPosition(vsIn.position);
    position = ComputeRotatedExtrudedLeafVertex(position, vsIn.extrude);

    // transform to projection space
    vsOut.position = mul(position, ModelViewProjection);
    VS_SETSCREENPOS(vsOut.position);
    vsOut.uv0intensity.xy = vsIn.uv0;

    // compute light vec
    float3 lightVec = ModelLightPos - position;

    // compute a model space normal and tangent vector
    float3 posCenter = vsIn.position - BoxCenter;
    float3 normal   = normalize(posCenter);
    float3 tangent  = cross(normal, float3(0.0f, 1.0f, 0.0f));
    float3 binormal = cross(normal, tangent);

    // compute a selfshadowing value
    float relDistToCenter = dot(posCenter, posCenter) / dot(BoxCenter-BoxMaxPos*0.8f, BoxCenter-BoxMaxPos*0.8f);
    float selfShadow = lerp(InnerLightIntensity, OuterLightIntensity, relDistToCenter);
    vsOut.uv0intensity.z = selfShadow;

    vsLight(vsIn.position, normal, tangent, binormal, ModelEyePos, ModelLightPos, vsOut.lightVec, vsOut.modelLightVec, vsOut.halfVec, vsOut.eyePos);

    return vsOut;
}

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
    Color pixel shader for leafs.
*/
color4 psLeafColor(const vsOutputLeafColor psIn, uniform bool hdr, uniform bool shadow) : COLOR
{
#if DEBUG_LIGHTCOMPLEXITY
    return float4(0.05f, 0.0f, 0.0f, 1.0f);
#else

    half shadowIntensity = shadow ? psShadow(PS_SCREENPOS) : 1.0;
    half2 uvOffset = half2(0.0f, 0.0f);
    if (BumpScale != 0.0f)
    {
        uvOffset = ParallaxUv(psIn.uv0intensity.xy, BumpSampler, psIn.eyePos);
    }
    color4 diffColor = tex2D(DiffSampler, psIn.uv0intensity.xy + uvOffset);
    float3 tangentSurfaceNormal = (tex2D(BumpSampler, psIn.uv0intensity.xy + uvOffset).rgb * 2.0f) - 1.0f;

    color4 baseColor = psLight(diffColor, tangentSurfaceNormal, psIn.lightVec, psIn.modelLightVec, psIn.halfVec, shadowIntensity);
    baseColor *= psIn.uv0intensity.z;
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
    Techniques for leaf renderer.
*/
technique tLeafDepth
{
    pass p0
    {
        CullMode = None;
        AlphaRef     = <AlphaRef>;
        VertexShader = compile VS_PROFILE vsLeafDepth();
        PixelShader  = compile PS_PROFILE psStaticDepthATest();
    }
}

technique tLeafColor
{
    pass p0
    {
        CullMode = None;
        AlphaBlendEnable = <AlphaBlendEnable>;
        AlphaRef     = <AlphaRef>;
        VertexShader = compile VS_PROFILE vsLeafColor();
        PixelShader  = compile PS_PROFILE psLeafColor(false, true);
    }
}

technique tLeafColorHDR
{
    pass p0
    {
        CullMode = None;
        AlphaBlendEnable = <AlphaBlendEnable>;
        AlphaRef     = <AlphaRef>;
        VertexShader = compile VS_PROFILE vsLeafColor();
        PixelShader  = compile PS_PROFILE psLeafColor(true, true);
    }
}
