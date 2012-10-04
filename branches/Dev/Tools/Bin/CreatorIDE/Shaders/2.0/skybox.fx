#line 1 "skybox.fx"

typedef half4 color4;

shared float4x4 ModelViewProjection;
shared float4x4 Model;

float4 MatDiffuse;

float Intensity0;

texture DiffMap0;
texture CubeMap0;

//------------------------------------------------------------------------------
//  shader input/output declarations
//------------------------------------------------------------------------------
struct vsInputCubeSkyboxColor
{
    float4 position : POSITION;
};

struct vsOutputCubeSkyboxColor
{
    float4 position : POSITION;
    float3 uv0 : TEXCOORD0;
};

struct vsInputSkyboxColor
{
    float4 position : POSITION;
    float2 uv0      : TEXCOORD0;
};

struct vsOutputSkyboxColor
{
    float4 position : POSITION;
    float2 uv0      : TEXCOORD0;
};

//------------------------------------------------------------------------------
//  Texture samplers
//------------------------------------------------------------------------------
sampler EnvironmentSampler = sampler_state
{
    Texture = <CubeMap0>;
    AddressU  = Wrap;
    AddressV  = Wrap;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
};

sampler SkySampler = sampler_state
{
    Texture = <DiffMap0>;
    AddressU  = Wrap;
    AddressV  = Wrap;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = None;
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
    Vertex shader for cubemapped sky box.
*/
vsOutputCubeSkyboxColor vsCubeSkyboxColor(vsInputCubeSkyboxColor vsIn)
{
    vsOutputCubeSkyboxColor vsOut;
    vsOut.position = mul(vsIn.position, ModelViewProjection);
    vsOut.uv0      = mul(vsIn.position, (float3x3)Model);
    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Pixel shader for cubemapped sky box.
*/
color4 psCubeSkyboxColor(vsOutputCubeSkyboxColor psIn, uniform bool hdr) : COLOR
{
    color4 baseColor = texCUBE(EnvironmentSampler, psIn.uv0) * MatDiffuse;
    if (hdr)
    {
        return EncodeHDR(baseColor);
    }
    else
    {
        return baseColor;
    }
}

//------------------------------------------------------------------------------
/**
    Vertex shader for sky box.
*/
vsOutputSkyboxColor vsSkyboxColor(vsInputSkyboxColor vsIn)
{
    vsOutputSkyboxColor vsOut;
    vsOut.position = mul(vsIn.position, ModelViewProjection);
    vsOut.uv0 = vsIn.uv0;
    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Pixel shader for sky box.
*/
color4 psSkyboxColor(vsOutputSkyboxColor psIn, uniform bool hdr) : COLOR
{
    color4 color = tex2D(SkySampler, psIn.uv0) * MatDiffuse * Intensity0;
    if (hdr)
    {
        return EncodeHDR(color);
    }
    else
    {
        return color;
    }
}

//------------------------------------------------------------------------------
/**
    Techniques for cubemapped sky box.
*/
technique tCubeSkyboxColor
{
    pass p0
    {
        CullMode = None;
        VertexShader = compile VS_PROFILE vsCubeSkyboxColor();
        PixelShader  = compile PS_PROFILE psCubeSkyboxColor(false);
    }
}

technique tCubeSkyboxColorHDR
{
    pass p0
    {
        CullMode = None;
        VertexShader = compile VS_PROFILE vsCubeSkyboxColor();
        PixelShader  = compile PS_PROFILE psCubeSkyboxColor(true);
    }
}

//------------------------------------------------------------------------------
/**
    Techniques for cubemapped sky box.
*/
technique tSkyboxColor
{
    pass p0
    {
        CullMode = None;
        VertexShader = compile VS_PROFILE vsSkyboxColor();
        PixelShader  = compile PS_PROFILE psSkyboxColor(false);
    }
}

technique tSkyboxColorHDR
{
    pass p0
    {
        CullMode = None;
        VertexShader = compile VS_PROFILE vsSkyboxColor();
        PixelShader  = compile PS_PROFILE psSkyboxColor(true);
    }
}
