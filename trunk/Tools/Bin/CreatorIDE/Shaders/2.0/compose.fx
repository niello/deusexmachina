#line 2 "compose.fx"

float4 MatAmbient;
float4 MatDiffuse;

float Intensity0;

static const float MaxWaterDist = 4.0;

texture DiffMap0;
texture DiffMap1;

#include "shaders:../lib/std.fx"

//------------------------------------------------------------------------------
/**
    shader input/output structure definitions
*/
struct vsInputCompose
{
    float4 position : POSITION;
    float2 uv0 : TEXCOORD0;
};

struct vsOutputCompose
{
    float4 position : POSITION;
    float2 uv0 : TEXCOORD0;
};

//------------------------------------------------------------------------------
/**
    sampler definitions
*/
sampler ComposeSourceBuffer = sampler_state
{
    Texture = <DiffMap0>;
    AddressU = Clamp;
    AddressV = Clamp;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;
};

sampler ComposeSourceBuffer1 = sampler_state
{
    Texture = <DiffMap1>;
    AddressU = Clamp;
    AddressV = Clamp;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;
};

//------------------------------------------------------------------------------
/**
    Vertex shader function for final image composing.
*/
vsOutputCompose vsCompose(const vsInputCompose vsIn)
{
    vsOutputCompose vsOut;
    vsOut.position = vsIn.position;
    vsOut.uv0 = vsIn.uv0;
    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Pixel shader function for final image composing.
*/
color4 psCompose(const vsOutputCompose psIn) : COLOR
{
    color4 imageColor = tex2D(ComposeSourceBuffer, psIn.uv0);
    half luminance = dot(imageColor.xyz, MatAmbient.xyz);
    return MatDiffuse * lerp(color4(luminance, luminance, luminance, luminance), imageColor, Intensity0);
}

//------------------------------------------------------------------------------
//  Pixel shader for final reflection/refraction water composition with depth in alphachannel.
//------------------------------------------------------------------------------
float4 psWaterDepthCompose(const vsOutputCompose psIn) : COLOR
{
    float4 scene = tex2D(ComposeSourceBuffer, psIn.uv0);
    float4 depth = tex2D(ComposeSourceBuffer1, psIn.uv0);
    float clampedDepth = clamp(depth.r / MaxWaterDist, 0, 1);

    return float4(scene.r, scene.g, scene.b, clampedDepth);
}

//------------------------------------------------------------------------------
/**
    Techniques for final frame compose.
*/
technique tCompose
{
    pass p0
    {
        ZWriteEnable     = False;
        ZEnable          = False;
        ColorWriteEnable = RED|GREEN|BLUE|ALPHA;
        AlphaBlendEnable = False;
        AlphaTestEnable  = False;
        CullMode         = None;
        StencilEnable    = False;
        VertexShader     = compile VS_PROFILE vsCompose();
        PixelShader      = compile PS_PROFILE psCompose();
    }
}

//------------------------------------------------------------------------------
/**
*/
technique tWaterDepthCompose
{
    pass p0
    {
        ZWriteEnable     = False;
        ZEnable          = False;
        ColorWriteEnable = RED|GREEN|BLUE|ALPHA;
        AlphaBlendEnable = False;
        AlphaTestEnable  = False;
        CullMode         = None;
        StencilEnable    = False;

        VertexShader = compile vs_2_0 vsCompose();
        PixelShader = compile ps_2_0 psWaterDepthCompose();
    }
}