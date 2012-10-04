#line 1 "particles.fx"

#define DEBUG_LIGHTCOMPLEXITY 0

typedef half4 color4;

shared float4x4 InvModelView;
shared float4x4 ModelViewProjection;

int AlphaSrcBlend = 5;
int AlphaDstBlend = 6;

texture DiffMap0;

//------------------------------------------------------------------------------
//  shader input/output declarations
//------------------------------------------------------------------------------
struct vsInputParticleColor
{
    float4 position  : POSITION;  // the particle position in world space
    float2 uv0       : TEXCOORD0; // the particle texture coordinates
    float2 extrude   : TEXCOORD1; // the particle corner offset
    float2 transform : TEXCOORD2; // the particle rotation and scale
    float4 color     : COLOR0;    // the particle color
};

struct vsOutputParticleColor
{
    float4 position : POSITION;
    float2 uv0      : TEXCOORD0;
    float4 diffuse  : COLOR0;
};

struct vsInputParticle2Color
{
    float4 position  : POSITION;    // the particle position in world space
    float3 velocity  : NORMAL;      // the particle coded uv and corners,rotation and scale
    float2 uv        : TEXCOORD0;   // the particle coded uv and corners,rotation and scale
    float4 data      : COLOR0;      // the particle coded uv and corners,rotation and scale
};

struct vsOutputParticle2Color
{
    float4 position : POSITION;
    float2 uv0      : TEXCOORD0;
    float4 diffuse  : COLOR0;
};

//------------------------------------------------------------------------------
//  Texture samplers
//------------------------------------------------------------------------------
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
    Scale down pseudo-HDR-value into RGB8.
*/
color4 EncodeHDR(in color4 rgba)
{
    return rgba * color4(0.5, 0.5, 0.5, 1.0);
}

//------------------------------------------------------------------------------
/**
    Vertex shader for particles.
*/
vsOutputParticleColor vsParticleColor(const vsInputParticleColor vsIn)
{
    float rotation = vsIn.transform[0];
    float size     = vsIn.transform[1];

    // build rotation matrix
    float sinAng, cosAng;
    sincos(rotation, sinAng, cosAng);
    float3x3 rot = {
        cosAng, -sinAng, 0.0f,
        sinAng,  cosAng, 0.0f,
        0.0f,    0.0f,   1.0f,
    };

    float4 position =  vsIn.position;
    float3 extrude  =  float3(vsIn.extrude, 0.0f);
    extrude *= size;
    extrude =  mul(rot, extrude);
    extrude =  mul(extrude, (float3x3) InvModelView);
    position.xyz += extrude.xyz;

    vsOutputParticleColor vsOut;
    vsOut.position = mul(position, ModelViewProjection);
    vsOut.uv0      = vsIn.uv0;
    vsOut.diffuse  = vsIn.color;

    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Vertex shader for particle2's.
*/
vsOutputParticle2Color vsParticle2Color(const vsInputParticle2Color vsIn)
{
    float code     = vsIn.data[0];
    float rotation = vsIn.data[1];
    float size     = vsIn.data[2];
    float colorCode  = vsIn.data[3];

    // build rotation matrix
    float sinAng, cosAng;
    sincos(rotation, sinAng, cosAng);
    float3x3 rot = {
        cosAng, -sinAng, 0.0f,
        sinAng,  cosAng, 0.0f,
        0.0f,    0.0f,   1.0f,
    };

    // decode color data
    float4  rgba;
    rgba.z = modf(colorCode/256.0f,colorCode);
    rgba.y = modf(colorCode/256.0f,colorCode);
    rgba.x = modf(colorCode/256.0f,colorCode);
    rgba.w = modf(code/256.0f,code);
    rgba *= 256.0f/255.0f;

    float4 position =  vsIn.position;


    // the corner offset gets calculated from the velocity

    float3 extrude = mul(InvModelView,vsIn.velocity);
    if(code != 0.0f)
    {
        extrude = normalize(extrude);
        float vis = abs(extrude.z);
        size *= cos(vis*3.14159f*0.5f);
        rgba.w *= cos(vis*3.14159f*0.5f);
    };
    extrude.z = 0.0f;
    extrude = normalize(extrude);

    extrude *= size;
    extrude =  mul(rot, extrude);
    extrude =  mul(extrude, (float3x3) InvModelView);
    position.xyz += extrude.xyz;

    vsOutputParticle2Color vsOut;
    vsOut.position = mul(position, ModelViewProjection);
    vsOut.uv0      = vsIn.uv;
    vsOut.diffuse  = rgba;

    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Pixel shader for particles.
*/
color4 psParticleColor(const vsOutputParticleColor psIn, uniform bool hdr) : COLOR
{
#if DEBUG_LIGHTCOMPLEXITY
    return float4(0.05f, 0.0f, 0.0f, 1.0f);
#else
    color4 finalColor = tex2D(DiffSampler, psIn.uv0) * psIn.diffuse;
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
    Techniques for particles.
*/
technique tParticleColor
{
    pass p0
    {
        CullMode     = None;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        Sampler[0]   = <DiffSampler>;
        VertexShader = compile VS_PROFILE vsParticleColor();
        PixelShader  = compile PS_PROFILE psParticleColor(false);
    }
}

technique tParticleColorHDR
{
    pass p0
    {
        CullMode     = None;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        Sampler[0]   = <DiffSampler>;
        VertexShader = compile VS_PROFILE vsParticleColor();
        PixelShader  = compile PS_PROFILE psParticleColor(true);
    }
}

//------------------------------------------------------------------------------
/**
    Techniques for particle2's.
*/
technique tParticle2Color
{
    pass p0
    {
        CullMode     = None;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        Sampler[0]   = <DiffSampler>;
        VertexShader = compile VS_PROFILE vsParticle2Color();
        PixelShader  = compile PS_PROFILE psParticleColor(false);
    }
}

technique tParticle2ColorHDR
{
    pass p0
    {
        CullMode     = None;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        Sampler[0]   = <DiffSampler>;
        VertexShader = compile VS_PROFILE vsParticle2Color();
        PixelShader  = compile PS_PROFILE psParticleColor(true);
    }
}
