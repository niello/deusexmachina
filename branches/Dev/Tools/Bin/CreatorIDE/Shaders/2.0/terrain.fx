#line 1 "terrain.fx"

typedef half4 color4;

int CullMode = 2;

shared float4x4 ModelViewProjection;
shared float3   ModelEyePos;

float4 TexGenS;                     // texgen parameters for u
float4 TexGenT;                     // texgen parameters for v

texture AmbientMap0;                // material weight texture
texture DiffMap0;                   // grass tile texture
texture DiffMap1;                   // rock tile texture
texture DiffMap2;                   // ground tile texture
texture DiffMap3;                   // snow tile texture

static const float DetailEnd = 150.0f;
static const float TexScale = 0.008f;
static const float DetailTexScale = 0.5f;

//------------------------------------------------------------------------------
//  shader input/output declarations
//------------------------------------------------------------------------------
struct vsInputTerrainColor
{
    float4 position : POSITION;
};

struct vsOutputTerrainColor
{
    float4 position : POSITION;         // position in projection space
    float2 uv0 : TEXCOORD0;             // generated material weight texture coordinates
    float2 uv1 : TEXCOORD1;             // generated tile texture coordinates
    float2 uv2 : TEXCOORD2;             // detail texture coordinates
    float1 fog : TEXCOORD3;             // x: reldist
};

//------------------------------------------------------------------------------
//  Texture samplers
//------------------------------------------------------------------------------
sampler WeightSampler = sampler_state
{
    Texture = <AmbientMap0>;
    AddressU  = Clamp;
    AddressV  = Clamp;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Point;
};

sampler GrassSampler = sampler_state
{
    Texture = <DiffMap0>;
    AddressU  = Wrap;
    AddressV  = Wrap;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Point;
};

sampler RockSampler = sampler_state
{
    Texture = <DiffMap1>;
    AddressU  = Wrap;
    AddressV  = Wrap;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Point;
};

sampler GroundSampler = sampler_state
{
    Texture = <DiffMap2>;
    AddressU  = Wrap;
    AddressV  = Wrap;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Point;
};

sampler SnowSampler = sampler_state
{
    Texture = <DiffMap3>;
    AddressU  = Wrap;
    AddressV  = Wrap;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Point;
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
    Vertex shader for terrain color.
*/
vsOutputTerrainColor vsTerrainColor(const vsInputTerrainColor vsIn)
{
    vsOutputTerrainColor vsOut;
    vsOut.position = mul(vsIn.position, ModelViewProjection);
    // generate texture coordinates after OpenGL rules
    vsOut.uv0.x = dot(vsIn.position, TexGenS);
    vsOut.uv0.y = dot(vsIn.position, TexGenT);
    vsOut.uv1.xy = vsIn.position.xz * TexScale;
    vsOut.uv2.xy = vsIn.position.xz * DetailTexScale;
    float eyeDist = distance(ModelEyePos, vsIn.position);
    vsOut.fog.x = 1.0f - saturate(eyeDist / DetailEnd);
    return vsOut;
}

//------------------------------------------------------------------------------
/**
    Pixel shader for terrain color.
*/
color4 psTerrainColor(const vsOutputTerrainColor psIn, uniform bool hdr) : COLOR
{
#if DEBUG_LIGHTCOMPLEXITY
    return float4(0.05, 0.0f, 0.0f, 1.0f);
#else
    // sample material weight texture
    color4 matWeights = tex2D(WeightSampler, psIn.uv0);

    // sample tile textures
    color4 baseColor = matWeights.x * lerp(tex2D(GrassSampler, psIn.uv1), tex2D(GrassSampler, psIn.uv2), psIn.fog.x);
    //baseColor += matWeights.y * lerp(tex2D(RockSampler, psIn.uv1), tex2D(RockSampler, psIn.uv2), psIn.fog.x);
    baseColor += matWeights.y * tex2D(RockSampler, psIn.uv2);
    baseColor += matWeights.z * lerp(tex2D(GroundSampler, psIn.uv1), tex2D(GroundSampler, psIn.uv2), psIn.fog.x);
    baseColor += matWeights.w * lerp(tex2D(SnowSampler, psIn.uv1), tex2D(SnowSampler, psIn.uv2), psIn.fog.x);

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
//  The technique.
//------------------------------------------------------------------------------
technique tTerrainColor
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaRef = 100;
        ZWriteEnable = true;
        
        VertexShader = compile VS_PROFILE vsTerrainColor();
        PixelShader  = compile PS_PROFILE psTerrainColor(false);
    }
}

technique tTerrainColorHDR
{
    pass p0
    {
        CullMode = <CullMode>;
		AlphaRef = 100;
        ZWriteEnable = true;
        
        VertexShader = compile VS_PROFILE vsTerrainColor();
        PixelShader  = compile PS_PROFILE psTerrainColor(true);
    }
}