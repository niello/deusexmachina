//------------------------------------------------------------------------------
//  hdr.fx
//
//  Contains various techniques for HDR rendering effects.
//
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------

/*
    NOTE: VS_PROFILE and PS_PROFILE macros are usually provided by the
    application and contain the highest supported shader models.

#define VS_PROFILE vs_2_0
#define PS_PROFILE ps_2_0
*/
#include "shaders:../lib/randtable.fx"

static const int MaxSamples = 16;

texture DiffMap0;
texture DiffMap1;
texture DiffMap2;
texture DiffMap3;
texture SpecMap0;
float3 SampleOffsetsWeights[MaxSamples];
shared float4 DisplayResolution;
float Intensity0;
float Intensity1;
float Intensity2;
float4 MatDiffuse;
float4 MatAmbient;
float4 FogDistances;
float4 FogColor;
float4 CameraFocus;
float Noise;
float Scale;
float Frequency;
shared float Time;

//------------------------------------------------------------------------------
//  Declarations.
//------------------------------------------------------------------------------
struct vsInput
{
    float4 position : POSITION;
    float2 uv0      : TEXCOORD0;
};

struct vsOutput
{
    float4 position  : POSITION;
    float2 uv0 : TEXCOORD0;
};

sampler SourceSampler = sampler_state
{
    Texture = <DiffMap0>;
    AddressU = Clamp;
    AddressV = Clamp;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = None;
};

sampler BloomSampler = sampler_state
{
    Texture = <DiffMap1>;
    AddressU = Clamp;
    AddressV = Clamp;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = None;
};

sampler Lum1x1Sampler = sampler_state
{
    Texture = <DiffMap1>;
    AddressU = Clamp;
    AddressV = Clamp;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = None;
};

sampler BloomSampler2 = sampler_state
{
    Texture = <DiffMap2>;
    AddressU = Clamp;
    AddressV = Clamp;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = None;
};

sampler DepthSampler = sampler_state
{
    Texture = <DiffMap3>;
    AddressU = Clamp;
    AddressV = Clamp;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = None;
};

sampler BlurSampler = sampler_state
{
    Texture = <DiffMap1>;
    AddressU = Clamp;
    AddressV = Clamp;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = None;
};

sampler NoiseSampler = sampler_state
{
    Texture = <SpecMap0>;
    AddressU = Wrap;
    AddressV = Wrap;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = None;
};

//------------------------------------------------------------------------------
//  Helper Functions.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    RGBE8 decoding for HDR rendering through conventional RGBA8 render targets.
    This is taken from the DX sample "HDRFormats"
*/
float3 DecodeRGBE8(in float4 rgbe)
{
    float3 decoded;
    float exponent = (rgbe.a * 255.0) - 128.0;
    decoded = rgbe.rgb * exp2(exponent);
    return decoded;
}

//------------------------------------------------------------------------------
/**
    Scale up pseudo-HDR-value encoded by EncodeHDR() in shaders.fx.
*/
float4 DecodeHDR(in float4 rgba)
{
    return rgba * float4(2.0f, 2.0f, 2.0f, 1.0f);
}

/*
float4 DecodeHDR(in float4 rgba)
{
    const float colorSpace = 0.8;
    const float maxHDR = 8;
    const float hdrSpace = 1 - colorSpace;
    //const float hdrPow = log(maxHDR)/log(hdrSpace);
    const float hdrPow = 10;
    //const float hdrRt = 1/hdrPow;
    const float hdrRt = 0.1;

    float3 col = clamp(rgba.rgb,0,colorSpace) * (1/colorSpace);
    float3 hdr = pow(clamp(rgba.rgb - colorSpace, 0, hdrSpace)+1,hdrPow)-1;
    float4 result;
    result.rgb = col + hdr;
    result.a = rgba.a;
    return result;
}
*/

//------------------------------------------------------------------------------
/**
*/
float
GaussianDistribution(in const float x, in const float y, in const float rho)
{
    const float pi = 3.1415927f;
    float g = 1.0f / sqrt(2.0f * pi * rho * rho);
    g *= exp(-(x * x + y * y) / (2 * rho * rho));
    return g;
}

//------------------------------------------------------------------------------
/**
    UpdateSamplesDownscale4x4

    Create filter kernel for a 4x4 downscale operation.
    This is normally executed pre-shader.
*/
void
UpdateSamplesDownscale4x4(in int texWidth, in int texHeight, out float3 sampleOffsetsWeights[MaxSamples])
{
    float tu = 1.0f / texWidth;
    float tv = 1.0f / texHeight;

    int index = 0;
    int y;
    for (y = 0; y < 4; y++)
    {
        int x;
        for (x = 0; x < 4; x++)
        {
            sampleOffsetsWeights[index++] = float3((x - 1.5f) * tu, (y - 1.5f) * tv, 0.0f);
        }
    }
}

//------------------------------------------------------------------------------
/**
    UpdateSamplesFilter2x2

    Create a filter kernel for a 2x2 linear filter.
*/
void
UpdateSamplesFilter2x2(in int texWidth, in int texHeight)
{
    float tu = 1.0f / texWidth;
    float tv = 1.0f / texHeight;

    SampleOffsetsWeights[0] = float3(-tu, -tv, 0.0f);
    SampleOffsetsWeights[1] = float3(+tu, -tv, 0.0f);
    SampleOffsetsWeights[2] = float3(+tu, +tv, 0.0f);
    SampleOffsetsWeights[3] = float3(-tu, +tv, 0.0f);
}

//------------------------------------------------------------------------------
/**
    UpdateSamplesLuminance

    Create a filter kernel for a 3x3 luminance filter.
    This is normally executed pre-shader.
*/
void
UpdateSamplesLuminance(in int texWidth, in int texHeight, out float3 sampleOffsetsWeights[MaxSamples])
{
    float tu = 1.0f / (3.0f * texWidth);
    float tv = 1.0f / (3.0f * texHeight);

    int index = 0;
    int x;
    for (x = -1; x <= 1; x++)
    {
        int y;
        for (y = -1; y <= 1; y++)
        {
            sampleOffsetsWeights[index++] = float3(x * tu, y * tv, 0.0f);
        }
    }
    for (index; index < MaxSamples; index++)
    {
        sampleOffsetsWeights[index] = float3(0.0f, 0.0f, 0.0f);
    }
}

//------------------------------------------------------------------------------
/**
    UpdateSamplesGaussBlur5x5

    Update the sample offsets and weights for a horizontal or vertical blur.
    This is normally executed in the pre-shader.
*/
void
UpdateSamplesGaussBlur5x5(in int texWidth, in int texHeight, float multiplier, out float3 sampleOffsetsWeights[MaxSamples])
{
    float tu = 1.0f / (float) texWidth;
    float tv = 1.0f / (float) texHeight;

    float totalWeight = 0.0f;
    int index = 0;
    int x;
    for (x = -2; x <= 2; x++)
    {
        int y;
        for (y = -2; y <= 2; y++)
        {
            // exclude pixels with block distance > 2, to reduce 5x5 filter
            // to 13 sample points instead of 25 (we only do 16 lookups)
            if ((abs(x) + abs(y)) <= 2)
            {
                // get unscaled Gaussian intensity
                sampleOffsetsWeights[index].xy = float2(x * tu, y * tv);
                float curWeight = GaussianDistribution(x, y, 1.0f);
                sampleOffsetsWeights[index].z = curWeight;
                totalWeight += curWeight;

                index++;
            }
        }
    }

    // normalize weights
    int i;
    float invTotalWeightMultiplier = (1.0f / totalWeight) * multiplier;
    for (i = 0; i < index; i++)
    {
        sampleOffsetsWeights[i].z *= invTotalWeightMultiplier;
    }

    // make sure the extra samples dont influence the result
    for (; index < MaxSamples; index++)
    {
        sampleOffsetsWeights[index] = float3(0.0f, 0.0f, 0.0f);
    }
}

//------------------------------------------------------------------------------
/**
    UpdateSamplesBloom

    Get sample offsets and weights for a horizontal or vertical bloom filter.
    This is normally executed in the pre-shader.
*/
void
UpdateSamplesBloom(in bool horizontal, in int texSize, in float deviation, in float multiplier, out float3 sampleOffsetsWeights[MaxSamples])
{
    float tu = 1.0f / (float) texSize;

    // fill center texel
    float weight = multiplier * GaussianDistribution(0.0f, 0.0f, deviation);
    sampleOffsetsWeights[0]  = float3(0.0f, 0.0f, weight);
    sampleOffsetsWeights[15] = float3(0.0f, 0.0f, 0.0f);

    // fill first half
    int i;
    for (i = 1; i < 8; i++)
    {
        if (horizontal)
        {
            sampleOffsetsWeights[i].xy = float2(i * tu, 0.0f);
        }
        else
        {
            sampleOffsetsWeights[i].xy = float2(0.0f, i * tu);
        }
        weight = multiplier * GaussianDistribution((float)i, 0, deviation);
        sampleOffsetsWeights[i].z = weight;
    }

    // mirror second half
    for (i = 8; i < 15; i++)
    {
        sampleOffsetsWeights[i] = sampleOffsetsWeights[i - 7] * float3(-1.0f, -1.0f, 1.0f);
    }
}

//------------------------------------------------------------------------------
//  Vertex shader for rendering a fullscreen quad with correction
//  for D3D's texel center.
//------------------------------------------------------------------------------
vsOutput vsQuad(const vsInput vsIn)
{
    vsOutput vsOut;
    vsOut.position = vsIn.position;
    vsOut.uv0 = vsIn.uv0;
    return vsOut;
}

//------------------------------------------------------------------------------
//  A 4x4 filtered downscale pixel shader.
//------------------------------------------------------------------------------
float4 psDownscale4x4(const vsOutput psIn) : COLOR
{
    UpdateSamplesDownscale4x4(DisplayResolution.x, DisplayResolution.y, SampleOffsetsWeights);
    float4 sample = 0.0f;
    int i;
    for (i = 0; i < 16; i++)
    {
        sample += tex2D(SourceSampler, psIn.uv0 + SampleOffsetsWeights[i].xy) * 1.1f;
    }
    return (sample / 16);
}

//------------------------------------------------------------------------------
//  Implements a bright pass filter. Uses the measured 1x1 luminance texture
//  as indication what areas of the picture are classified as "bright".
//------------------------------------------------------------------------------
float4 psBrightPassFilter(const vsOutput psIn) : COLOR
{
    float4 sample = DecodeHDR(tex2D(SourceSampler, psIn.uv0));
    float brightPassThreshold = Intensity1;
    float brightPassOffset    = Intensity2;

    // subtract out dark pixels
    sample.rgb -= brightPassThreshold;

    // clamp to 0
    sample.rgb = max(sample.rgb, 0.0f);

    // map value to 0..1
    sample.rgb /= brightPassOffset + sample.rgb;

    return sample;
}

//------------------------------------------------------------------------------
//  Implements a 5x5 Gaussian blur filter.
//------------------------------------------------------------------------------
float4 psGaussBlur5x5(const vsOutput psIn) : COLOR
{
    // preshader
    UpdateSamplesGaussBlur5x5(DisplayResolution.x, DisplayResolution.y, 1.5f, SampleOffsetsWeights);

    // shader
    float4 sample = 0.0f;
    int i;
    for (i = 0; i < 12; i++)
    {
        sample += SampleOffsetsWeights[i].z * tex2D(SourceSampler, psIn.uv0 + SampleOffsetsWeights[i].xy);
    }
    return sample;
}

//------------------------------------------------------------------------------
//  Pixel shader which performs a horizontal bloom effect.
//------------------------------------------------------------------------------
float4 psBloomHori(const vsOutput psIn) : COLOR
{
    // preshader...
    UpdateSamplesBloom(true, DisplayResolution.x, 3.0f, 2.0f, SampleOffsetsWeights);

    // shader...
    int i;
    float4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
    for (i = 0; i < MaxSamples; i++)
    {
        color += SampleOffsetsWeights[i].z * tex2D(SourceSampler, psIn.uv0 + SampleOffsetsWeights[i].xy);
    }
    return color;
}

//------------------------------------------------------------------------------
//  Pixel shader which performs a vertical bloom effect.
//------------------------------------------------------------------------------
float4 psBloomVert(const vsOutput psIn) : COLOR
{
    // preshader...
    UpdateSamplesBloom(false, DisplayResolution.y, 3.0f, 2.0f, SampleOffsetsWeights);

    // shader...
    int i;
    float4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
    for (i = 0; i < MaxSamples; i++)
    {
        color += SampleOffsetsWeights[i].z * tex2D(SourceSampler, psIn.uv0 + SampleOffsetsWeights[i].xy);
    }
    return color;
}

//------------------------------------------------------------------------------
//  Pixel shader which combines the source buffers into the final
//  result for the HDR adaptive eye model render path.
//------------------------------------------------------------------------------
float4 psFinalScene(const vsOutput psIn) : COLOR
{
    float4 sample = tex2D(SourceSampler, psIn.uv0);
    float4 bloom  = tex2D(BloomSampler2, psIn.uv0);
    //float adaptedLum = tex2D(Lum1x1Sampler, float2(0.5f, 0.5f));
    // float middleGray = Intensity0;
    float bloomScale = Intensity1;

    // FIXME: handle blue shift

    // add bloom effect
    sample += bloomScale * bloom;

    return sample;
}

//------------------------------------------------------------------------------
//  Compute fogging given a depth value and input color.
//------------------------------------------------------------------------------
float4 psFog(float depth, float4 color)
{
    float start = FogDistances.x;
    float end   = FogDistances.y;
    float l = saturate((end - depth) / (end - start));
    return lerp(FogColor, color, l);
}

//------------------------------------------------------------------------------
//  Get depth blurred sample at position.
//------------------------------------------------------------------------------
float4 psDepthBlur(sampler focusTexture, sampler blurTexture, float2 uv)
{
    // get sampled depth around current pixel
    float d = tex2D(DepthSampler, uv).r;
    float focusDist = CameraFocus.x;
    float focusLength = CameraFocus.y;
    float focus = saturate(abs(d - focusDist) / focusLength);
    return lerp(tex2D(focusTexture, uv), tex2D(blurTexture, uv), focus);
}

//------------------------------------------------------------------------------
//  Add film grain to color.
//------------------------------------------------------------------------------
float4 psFilmGrain(float4 color, float2 uv)
{
    float4 noiseColor = tex2D(NoiseSampler, (uv + RandArray[fmod(Time * Frequency, 16)].xy) * Scale);
    return lerp(color, noiseColor, Noise);
}

//------------------------------------------------------------------------------
//  Pixel shader for final scene composition.
//------------------------------------------------------------------------------
float4 psHdrCompose(const vsOutput psIn) : COLOR
{
    // get depth-blurred sample
    float4 sample = DecodeHDR(psDepthBlur(SourceSampler, BlurSampler, psIn.uv0));

    // add fog
    sample = psFog(tex2D(DepthSampler, psIn.uv0).r, sample);
    float4 bloom  = tex2D(BloomSampler2, psIn.uv0);
    float saturation = Intensity0;
    float bloomScale = Intensity1;
    float4 lumiVec = MatAmbient;
    float4 balance = MatDiffuse;
    sample += bloomScale * bloom;
    float luminance = dot(sample.xyz, lumiVec.xyz);
    float4 color = balance * lerp(float4(luminance, luminance, luminance, luminance), sample, saturation);

    // add noise
    color = psFilmGrain(color, psIn.uv0);
    return color;
}

//------------------------------------------------------------------------------
//  Pixel shader for final scene composition.
//------------------------------------------------------------------------------
float4 psBlurAndFog(const vsOutput psIn, uniform bool hdr) : COLOR
{
    // get depth-blurred sample
    float4 sample;
    if (hdr)
    {
        sample = DecodeHDR(psDepthBlur(SourceSampler, BlurSampler, psIn.uv0));
    }
    else
    {
        sample = psDepthBlur(SourceSampler, BlurSampler, psIn.uv0);
    }

    // add fog
    sample = psFog(tex2D(DepthSampler, psIn.uv0).r, sample);

    float saturation = Intensity0;
    float4 lumiVec = MatAmbient;
    float4 balance = MatDiffuse;
    float luminance = dot(sample.xyz, lumiVec.xyz);
    float4 color = balance * lerp(float4(luminance, luminance, luminance, luminance), sample, saturation);
    return color;
}

//------------------------------------------------------------------------------
//  A simple copy pixel shader
//------------------------------------------------------------------------------
float4 psCopy(const vsOutput psIn) : COLOR
{
    return DecodeHDR(tex2D(SourceSampler, psIn.uv0));
}

//------------------------------------------------------------------------------
//  Pixel shader which combines a source texture with a bloom texture.
//------------------------------------------------------------------------------
float4 psCombine(const vsOutput psIn) : COLOR
{
    return tex2D(SourceSampler, psIn.uv0) + Intensity0 * tex2D(BloomSampler, psIn.uv0);
}

//------------------------------------------------------------------------------
//  Technique to perform a 4x4 filtered downscale
//------------------------------------------------------------------------------
technique tDownscale4x4
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
        VertexShader     = compile VS_PROFILE vsQuad();
        PixelShader      = compile PS_PROFILE psDownscale4x4();
    }
}

//------------------------------------------------------------------------------
//  Implement the adaptive eye model bright pass filter.
//------------------------------------------------------------------------------
technique tBrightPassFilter
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
        VertexShader     = compile VS_PROFILE vsQuad();
        PixelShader      = compile PS_PROFILE psBrightPassFilter();
    }
}

//------------------------------------------------------------------------------
//  Implements a Gaussian blur filter.
//------------------------------------------------------------------------------
technique tGaussBlur5x5
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
        VertexShader     = compile VS_PROFILE vsQuad();
        PixelShader      = compile PS_PROFILE psGaussBlur5x5();
    }
}

//------------------------------------------------------------------------------
//  Technique to perform the horizontal bloom effect.
//------------------------------------------------------------------------------
technique tBloomHori
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
        VertexShader     = compile VS_PROFILE vsQuad();
        PixelShader      = compile PS_PROFILE psBloomHori();
    }
}

//------------------------------------------------------------------------------
//  Technique to perform the horizontal bloom effect.
//------------------------------------------------------------------------------
technique tBloomVert
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
        VertexShader     = compile VS_PROFILE vsQuad();
        PixelShader      = compile PS_PROFILE psBloomVert();
    }
}

//------------------------------------------------------------------------------
//  Final scene composition (pre-alpha) for adaptive eye model
//  renderer.
//------------------------------------------------------------------------------
technique tFinalScene
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
        VertexShader     = compile VS_PROFILE vsQuad();
        PixelShader      = compile PS_PROFILE psFinalScene();
    }
}

//------------------------------------------------------------------------------
//  Technique to perform a horizontal blur
//------------------------------------------------------------------------------
technique tHoriBlur
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
        VertexShader     = compile VS_PROFILE vsQuad();
        PixelShader      = compile PS_PROFILE psBloomHori();
    }
}

//------------------------------------------------------------------------------
//  Technique to perform a vertical blur
//------------------------------------------------------------------------------
technique tVertBlur
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
        VertexShader     = compile VS_PROFILE vsQuad();
        PixelShader      = compile PS_PROFILE psBloomVert();
    }
}

//------------------------------------------------------------------------------
//  Technique to combine source texture with bloom texture.
//------------------------------------------------------------------------------
technique tCombine
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

        VertexShader = compile VS_PROFILE vsQuad();
        PixelShader = compile PS_PROFILE psCombine();
    }
}

//------------------------------------------------------------------------------
//  Technique to copy a source texture to the render target.
//------------------------------------------------------------------------------
technique tCopy
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

        VertexShader = compile VS_PROFILE vsQuad();
        PixelShader = compile PS_PROFILE psCopy();
    }
}

//------------------------------------------------------------------------------
//  Technique to for final scene composition.
//------------------------------------------------------------------------------
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

        VertexShader = compile VS_PROFILE vsQuad();
        PixelShader = compile PS_PROFILE psHdrCompose();
    }
}


//------------------------------------------------------------------------------
//  Technique to for final scene composition.
//------------------------------------------------------------------------------
technique tBlurAndFog
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

        VertexShader = compile VS_PROFILE vsQuad();
        PixelShader = compile PS_PROFILE psBlurAndFog(false);
    }
}

technique tBlurAndFogHDR
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

        VertexShader = compile VS_PROFILE vsQuad();
        PixelShader = compile PS_PROFILE psBlurAndFog(true);
    }
}