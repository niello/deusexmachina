//------------------------------------------------------------------------------
//  sky.fx
//
//  Nebula2 sky shaders for shader model 2.0
//
//  (C) 2005 Radon Labs GmbH
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
    shader parameters
*/
shared float4x4 ModelViewProjection;
shared float4x4 Projection;
shared float4x4 View;
shared float4x4 Model;
shared float4x4 InvView;
shared float4x4 InvModel;
shared float3   ModelEyePos;

float4 Saturation;
float4 Brightness;
float4 Position;
float4 TopColor;
float4 BottomColor;
float4 SunColor;
float SunRange;
float SkyBottom;
float SunFlat;
float Intensity0;
float4 Move;
float4 CloudPos;
float4 CloudMod;
float4 CloudGrad;
float Map0uvRes;
float Map1uvRes;
float Glow;
float Weight;
float Density;
float Lightness;
float4 MatDiffuse;
float4 LightDiffuse;
float4 LightDiffuse1;
int AlphaRef;
int AlphaSrcBlend = 5;
int AlphaDstBlend = 6;

texture DiffMap0;                   // CubeMap
texture DiffMap1;					// 2d texture from Rendertarget
texture BumpMap0;					// 2d texture from Rendertarget

static const float MaxWaterDist = 100;
static const float MaxAtmoDist = 1000;

//==============================================================================
//  shader input/output structure definitions
//==============================================================================

//------------------------------------------------------------------------------
/**
    Sky
*/
struct VsInputSky
{
    float4 position : POSITION;
    float2 uv0      : TEXCOORD0;
};

struct VsOutputSky
{
    float4 position 	: POSITION;
	float4 pixel		: TEXCOORD0;
};

//------------------------------------------------------------------------------
/**
    Cloud
*/
struct VsInputCloud
{
    float4 position 		: POSITION;
    float2 uv0      		: TEXCOORD0;
    float3 normal 			: NORMAL;
    float3 tangent 			: TANGENT;
};

struct VsOutputCloud
{
    float4 position 				: POSITION;
   	float4 uv     					: TEXCOORD0;
   	float3 primLightVec 			: TEXCOORD1;
   	float4 gauge_glow_light_alpha 	: TEXCOORD2;	// x=gaugeVal, y = glow, z = lightingAdjust, w = alphaMod
 	float  mapmix	      			: TEXCOORD3;
};

//------------------------------------------------------------------------------
/**
    Sun
*/
struct VsInputSun
{
    float4 position : POSITION;
    float2 uv0 : TEXCOORD0;
};

struct VsOutputSun
{
    float4 position : POSITION;
    float2 uv0 		: TEXCOORD0;
    float4 uvpos	: TEXCOORD1;
};

//------------------------------------------------------------------------------
/**
    Stars
*/
struct VsInputStars
{
    float4 position : POSITION;
    float2 uv0      : TEXCOORD0;
};

struct VsOutputStars
{
    float4 position : POSITION;
    float2 uv0     	: TEXCOORD0;
    float  fade		: TEXCOORD1;
};

//------------------------------------------------------------------------------
/**
    Horizon
*/
struct VsInputHorizon
{
    float4 position : POSITION;
};

struct VsOutputHorizon
{
    float4 position : POSITION;
    float4 CubeV0 	: TEXCOORD0;        // CubeMapVector
//   float4 uvpos	: TEXCOORD1;
};

struct VsOutputHorizonDepth
{
    float4 position : POSITION;
    float4 CubeV0 	: TEXCOORD0;        // CubeMapVector
    float2 z		: TEXCOORD1;
};


//------------------------------------------------------------------------------
/**
    CopyTexture_rgb
*/
struct vsInputCopyTexture
{
    float4 position : POSITION;
    float2 uv0      : TEXCOORD0;
};

struct vsOutputCopyTexture
{
    float4 position  : POSITION;
    float2 uv0 : TEXCOORD0;
};


//==============================================================================
//  sampler definitions
//==============================================================================
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

sampler Diff1Sampler = sampler_state
{
    Texture = <DiffMap1>;
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



sampler CopySampler = sampler_state
{
    Texture = <DiffMap0>;
    AddressU = Clamp;
    AddressV = Clamp;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;
};


//==============================================================================
//  Helper functions
//==============================================================================

//------------------------------------------------------------------------------
/**
    Scale down pseudo-HDR-value into RGB8.
*/
/**
float4 EncodeHDR(in float4 rgba)
{
    return rgba * float4(0.25, 0.25, 0.25, 1.0);
}

float4 DecodeHDR(in float4 rgba)
{
    return rgba * float4(4.0, 4.0, 4.0, 1.0);
} */

float4 EncodeHDR(in float4 rgba)
{
	const float colorSpace = 0.8;
	const float maxHDR = 8;
	const float hdrSpace = 1 - colorSpace;
	//const float hdrPow = log(maxHDR)/log(hdrSpace);
	const float hdrPow = 10;
	//const float hdrRt = 1/hdrPow;
	const float hdrRt = 0.1;

	float3 col = clamp(rgba.rgb,0,1) * colorSpace;
	float3 hdr = pow(clamp(rgba.rgb,1,10),hdrRt)-1;
	float4 result;
	hdr = clamp(hdr, 0, hdrSpace);
    result.rgb = col + hdr;
	result.a = rgba.a;
    return result;
}

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

//------------------------------------------------------------------------------
/**
    Helper function for Depth encoding
*/
float2 EncodeDepth(const float z)
{
	float water = clamp(z/MaxWaterDist,0,1);
	float atmo = clamp((z-MaxWaterDist)/(MaxAtmoDist-MaxWaterDist),0,1);
	return (float2(water,atmo));
}

float DecodeDepth(const float2 waterAtmo)
{
	float water = waterAtmo.x * MaxWaterDist;
	float atmo = waterAtmo.y * (MaxAtmoDist-MaxWaterDist);
	return water+atmo;
}

//==============================================================================
//  Vertex and pixel shader functions.
//==============================================================================

//------------------------------------------------------------------------------
/**
	Sky
    Vertex shader: compute sky colors
*/
VsOutputSky vsSky(const VsInputSky vsIn)
{
    VsOutputSky vsOut;
	vsOut.position = mul(vsIn.position, ModelViewProjection);

    float3 sun = normalize(Position.xyz);
    float3 top = float3(0,1,0);
    float3 V = normalize(vsIn.position);
    float3 sat = Saturation.rgb * Saturation.a;
 	float3 bright = Brightness.rgb * Brightness.a;
	float sunrng = SunRange-1;

	float dotSV = (V.x * sun.x) + (V.y * sun.y) + (V.z * sun.z);
	float quotSVy = 1- saturate(pow(saturate( max(V.y+1, sun.y+1)-min(V.y+1, sun.y+1)), 2) * SunFlat);
	float dotTV = V.x * top.x + V.y * top.y + V.z * top.z;
	float weight = (dotSV*quotSVy*Intensity0 + sunrng)/(1+sunrng)-(1-SunColor.a);

	vsOut.pixel = lerp(BottomColor,TopColor, saturate((dotTV-SkyBottom)/(1-SkyBottom)));
	vsOut.pixel.rgb = (vsOut.pixel.rgb * sat + ((sat - 1) * -0.5) + (bright - 1));
	vsOut.pixel = lerp(vsOut.pixel,SunColor,  saturate(pow(saturate(weight), 2)) );

    return vsOut;
}

//------------------------------------------------------------------------------
/**
	Sky
    Pixel shader: just return the color value
*/
float4 psSky(const VsOutputSky psIn, uniform bool hdr) : COLOR
{
	float4 pixel = saturate(psIn.pixel);

	if (hdr) return EncodeHDR(pixel);
    else     return pixel;
}


//------------------------------------------------------------------------------
/**
	Cloud
    Vertex shader:
*/
VsOutputCloud vsCloud(const VsInputCloud vsIn)
{
// Vertex-shader output
    VsOutputCloud vsOut;

// Transform to spaces
	vsOut.position = mul(vsIn.position, ModelViewProjection);
	float3 sunPos = mul(Position,InvModel);

// compute UV-coordinates
    float modmap_uv_mod = Map0uvRes;													// USER-PARAMETER
    float surface_uv_mod = Map1uvRes;													// USER-PARAMETER
    vsOut.uv.xy = (vsIn.uv0 * modmap_uv_mod) + CloudPos.xy;								// USER-PARAMETER
    vsOut.uv.zw = (vsIn.uv0 * surface_uv_mod) + CloudPos.zw;							// USER-PARAMETER

// force position of each vertex to be flat and on a deeper level, to get a better lighting result
    float3 newPos = vsIn.position;
    newPos.y = -1.5;

// set light to a fixed distance and compute lightvector
    vsOut.primLightVec = normalize(normalize(sunPos)*3 - newPos);

// set min + max vertex-lighting adjust
    float dotTL = dot(float3(0,1,0),vsOut.primLightVec);
    vsOut.gauge_glow_light_alpha.z = clamp(dotTL, 0.3, 0.7) - dotTL;

// compute horizon gradient gauge modificator and add to gauge value
    float2 grad = CloudGrad.zw;
    float2 newPosN =  normalize(newPos.xz);
    float2 gradN = normalize(grad);
	float dotPG = newPosN.x * gradN.x + newPosN.y * gradN.y;
    float lenG = length(grad);
    float lenP = length(newPos.xz);
    float weight = saturate(dotPG*pow(lenP*0.90,3) + lenG);
    float gradGauge = lerp(CloudGrad.x, 0, saturate((1-weight)*CloudGrad.y));
    vsOut.gauge_glow_light_alpha.x = CloudMod.x + gradGauge;						  	// USER-PARAMETER

// compute glow value
    float3 sun = normalize(sunPos);
    float3 V = normalize(vsIn.position.xyz);
    float distSV = sqrt(pow(V.x - sun.x,2) + pow(V.y - sun.y,2) + pow(V.z - sun.z,2));
    float glow = Glow * (1.8 - pow(MatDiffuse.a,2));      								// USER-PARAMETER
    vsOut.gauge_glow_light_alpha.y = glow - glow * sqrt(saturate(distSV / SunRange));	// USER-PARAMETER

// compute weight for mixing cloudmaps
    float mapblend = CloudMod.z;														// USER-PARAMETER
	vsOut.mapmix = mapblend % 1;
	if ((mapblend % 2)>1) vsOut.mapmix = 1-vsOut.mapmix;

// compute alpha modificator
	float alpha = MatDiffuse.a;															// USER-PARAMETER
	vsOut.gauge_glow_light_alpha.w = alpha - alpha*saturate((0.25-saturate(vsIn.position.y))*3.0);  // PARAMETER??????

    return vsOut;
}

//------------------------------------------------------------------------------
/**
	Cloud
    Pixel shader:
*/
float4 psCloud(const VsOutputCloud psIn, uniform bool hdr) :COLOR
{
// read cloudmaps and cloud-surface with normals and height
	float2 modmapUV = psIn.uv.xy;
	float2 surfaceUV = psIn.uv.zw;
	float4 modmap = lerp(tex2D(DiffSampler, modmapUV),
    			         tex2D(Diff1Sampler, modmapUV), psIn.mapmix);
	float4 pixel = tex2D(BumpSampler, surfaceUV);

// compute gauge and alpha
	const float gaugemod = 0.3;
	const float maxGauge = 10;
	float multiplier = CloudMod.y;														// USER-PARAMETER
	float gaugeVal = psIn.gauge_glow_light_alpha.x;
	float alphaMod = psIn.gauge_glow_light_alpha.w;
	gaugeVal += (modmap.a+pixel.a) * 2 -2;
	gaugeVal *= multiplier;
	float gauge = clamp(gaugemod * gaugeVal, 0, maxGauge);
   	float alpha = saturate((1+gaugeVal) * alphaMod);

// compute color intensity
	float density = Density;															// USER-PARAMETER
	float4 cloudcolor = MatDiffuse - (gauge*density);

// compute sunglow
	float glow = psIn.gauge_glow_light_alpha.y;
	glow = 1 + clamp((glow-gauge*0.2),0,5);

// blend normals from bumpmap and cloudmaps and compute tangentsurfacenormal
	float normal_blend_weight = Weight;												// USER-PARAMETER
	pixel.rgb = lerp(pixel.rgb, modmap.rgb, normal_blend_weight);
 	float3 tangentSurfaceNormal = (((float3(pixel.r,pixel.b,pixel.g) - 0.5) * 2));

// compute light intensity with dotNL and a modificator for thin cloud areas
	float lightingAdjust = psIn.gauge_glow_light_alpha.z;
    float dotNL = dot(tangentSurfaceNormal , psIn.primLightVec);
	float primDiffIntensity = saturate((dotNL + lightingAdjust) * glow);
    float secDiffIntensity  = 1 - primDiffIntensity;

// compute resulting cloud color
	float lightness = Lightness;																// USER-PARAMETER ->UNNÖTIG
    float4 diffcol = (primDiffIntensity * LightDiffuse  + secDiffIntensity * LightDiffuse1) * (lightness + glow) * cloudcolor;
// copy alpha value
    diffcol.a = alpha;

    diffcol = saturate(diffcol);
    if (hdr) return EncodeHDR(diffcol);
    else     return diffcol;
}


//------------------------------------------------------------------------------
//  Sun
//------------------------------------------------------------------------------
VsOutputSun vsSun(const VsInputSun vsIn)
{
    VsOutputSun vsOut;
    vsOut.position = mul(vsIn.position, ModelViewProjection);
    vsOut.uv0 = vsIn.uv0;
	vsOut.uvpos = vsOut.position;
    return vsOut;
}

float4 psSun(const VsOutputSun psIn, uniform bool hdr) : COLOR
{
	float2 uv1 = psIn.uvpos.xy/psIn.uvpos.w;
	uv1.y *= -1;
	uv1 = (uv1+1)/2;

	float4 pixel = tex2D(DiffSampler, psIn.uv0);
	float4 back = tex2D(Diff1Sampler,uv1);
	pixel.rgb *= Intensity0 * MatDiffuse.rgb;
	pixel = lerp( back, pixel, saturate(pixel.a - back.a*( 1/saturate(MatDiffuse.a) )) );
	pixel.a = 0;
	if (hdr) return EncodeHDR(pixel);
    else     return pixel;
}

float4 psSun2(const VsOutputSun psIn, uniform bool hdr) : COLOR
{

	float4 pixel = tex2D(DiffSampler, psIn.uv0);
	pixel.rgb *= Intensity0 * MatDiffuse.rgb;
	if (hdr) return EncodeHDR(pixel);
    else     return pixel;
}

//------------------------------------------------------------------------------
// Stars
//------------------------------------------------------------------------------
VsOutputStars vsStars(const VsInputStars vsIn)
{
    VsOutputStars vsOut;
	vsOut.position = mul(vsIn.position, ModelViewProjection);
    vsOut.uv0 = vsIn.uv0;

    float3 sun = normalize(Position.xyz);
	float3 top = float3(0,1,0);
	float3 V = normalize(vsIn.position);

 	float dotSV = V.x * sun.x + V.y * sun.y + V.z * sun.z;
	float dotTV = V.x * top.x + V.y * top.y + V.z * top.z;

    vsOut.fade = saturate(dotTV*4) - saturate(dotSV+0.2);
    return vsOut;
}

float4 psStars(const VsOutputStars psIn, uniform bool hdr) : COLOR
{

 	float4 stars = tex2D(DiffSampler, psIn.uv0 * 10);
 	float4 starspread = tex2D(Diff1Sampler, psIn.uv0);

  	stars *= starspread * Brightness;
  	stars.a = stars.r + stars.g + stars.b + stars.a;
  	stars.a *= psIn.fade;
  	stars.rgb = 1 * Intensity0;
  	float4 pixel = stars;

	if (hdr) return EncodeHDR(pixel);
    else     return pixel;
}

//------------------------------------------------------------------------------
//  Horizon
//------------------------------------------------------------------------------
VsOutputHorizon vsHorizon(const VsInputHorizon vsIn)
{
    VsOutputHorizon vsOut;
	vsOut.position = mul(vsIn.position, ModelViewProjection);
    vsOut.CubeV0 = mul(vsIn.position, Projection);
//    vsOut.uvpos = vsOut.position;
    return vsOut;
}

float4 psHorizon(const VsOutputHorizon psIn, uniform bool hdr) : COLOR
{
//	float2 projUV = psIn.uvpos.xy/psIn.uvpos.w;
//	projUV.y *= -1;
//	projUV = (projUV+1)/2;
	float4 pixel = texCUBE(DiffSampler, psIn.CubeV0)*MatDiffuse;
	pixel.rgb *= Intensity0;
//	float4 sky = tex2D(Diff1Sampler, projUV);
//	if (hdr) sky = DecodeHDR(sky);
//	pixel.rgb = lerp(pixel.rgb, sky.rgb, pixel.a*0.5);
	pixel = saturate(pixel);
    if (hdr) return EncodeHDR(pixel);
    else     return pixel;
}

VsOutputHorizonDepth vsHorizonDepth(const VsInputHorizon vsIn, uniform bool saveZ)
{
    VsOutputHorizonDepth vsOut;
	vsOut.position = mul(vsIn.position, ModelViewProjection);
    vsOut.CubeV0 = mul(vsIn.position, Projection);
	if (saveZ) vsOut.z = EncodeDepth(1000);
    else vsOut.z = 0;
    return vsOut;
}

float4 psHorizonDepth(const VsOutputHorizonDepth psIn, uniform bool saveZ) : COLOR
{
	float4 pixel = texCUBE(DiffSampler, psIn.CubeV0)*MatDiffuse;
	pixel.rgb *= Intensity0;
	clip(pixel.a - (AlphaRef / 255.0));
    if (saveZ) return float4(psIn.z.r * pixel.a, psIn.z.g * pixel.a, 0.0f, 1.0f);
	return float4(0.0f, 0.0f, 0.0f, 1.0f);
}

//------------------------------------------------------------------------------
//  CopyTexture_rgb
//------------------------------------------------------------------------------
vsOutputCopyTexture vsCopyTexture(const vsInputCopyTexture vsIn)
{
    vsOutputCopyTexture vsOut;
    vsOut.position = vsIn.position;
    vsOut.uv0 = vsIn.uv0;
    return vsOut;
}


float4 psCopyTexture_rgb(const vsOutputCopyTexture psIn) : COLOR
{
	float4 pixel = tex2D(CopySampler, psIn.uv0);
	pixel.a = 0;
	return pixel;
}

//==============================================================================
//  Techniques
//==============================================================================

//------------------------------------------------------------------------------
/**
    Techniques for shader "sky"
*/
technique tGradientSky
{
    pass p0
    {
        CullMode = NONE;
        AlphaRef = 0;
        AlphaBlendEnable = true;
        SrcBlend = SrcAlpha;
		DestBlend = InvSrcAlpha;
        ZWriteEnable = false;
        ZEnable          = False;
        VertexShader = compile vs_2_0 vsSky();
        PixelShader  = compile ps_2_0 psSky(false);
    }
}

technique tGradientSkyHDR
{
    pass p0
    {
        CullMode = NONE;
        AlphaRef = 0;
        AlphaBlendEnable = true;
        SrcBlend = SrcAlpha;
		DestBlend = InvSrcAlpha;
        ZWriteEnable = false;
        ZEnable          = False;
        VertexShader = compile vs_2_0 vsSky();
        PixelShader  = compile ps_2_0 psSky(true);
    }
}

//------------------------------------------------------------------------------
/**
    Techniques for shader "cloud"
*/
technique tCloud
{
    pass p0
    {
        CullMode = NONE;
        AlphaRef = 0;
        AlphaBlendEnable = true;
        SrcBlend = SrcAlpha;
		DestBlend = InvSrcAlpha;
        ZWriteEnable = false;
        ZEnable          = False;
        VertexShader = compile vs_2_0 vsCloud();
        PixelShader  = compile ps_2_0 psCloud(false);
    }
}

technique tCloudHDR
{
    pass p0
    {
        CullMode = NONE;
        AlphaRef = 0;
        AlphaBlendEnable = true;
        SrcBlend = SrcAlpha;
		DestBlend = InvSrcAlpha;
        ZWriteEnable = false;
        ZEnable          = False;
        VertexShader = compile vs_2_0 vsCloud();
        PixelShader  = compile ps_2_0 psCloud(true);
    }
}

//------------------------------------------------------------------------------
/**
    Techniques for shader "sun"
*/
technique tSun
{
    pass p0
    {
        CullMode = NONE;
        AlphaRef = 0;
        AlphaBlendEnable = False;
        SrcBlend  = SRCALPHA;
        DestBlend = INVSrcALPHA;
        ZWriteEnable = false;
        ZEnable          = False;
        VertexShader = compile vs_2_0 vsSun();
        PixelShader  = compile ps_2_0 psSun(false);
    }
}

technique tSunHDR
{
    pass p0
    {
        CullMode = NONE;
        AlphaRef = 0;
        AlphaBlendEnable = true;
        SrcBlend  = SRCALPHA;
        DestBlend = INVSrcALPHA;
        ZWriteEnable = false;
        ZEnable          = False;
        VertexShader = compile vs_2_0 vsSun();
        PixelShader  = compile ps_2_0 psSun(true);
    }
}

technique tSun2HDR
{
    pass p0
    {
        CullMode = NONE;
        AlphaRef = 0;
        AlphaBlendEnable = true;
        SrcBlend  = SRCALPHA;
        DestBlend = INVSrcALPHA;
        ZWriteEnable = false;
        ZEnable          = False;
        VertexShader = compile vs_2_0 vsSun();
        PixelShader  = compile ps_2_0 psSun2(true);
    }
}

//------------------------------------------------------------------------------
/**
    Techniques for shader "stars"
*/
technique tStars
{
    pass p0
    {
        CullMode = NONE;
        AlphaRef = 0;
        ColorWriteEnable = RED|GREEN|BLUE;
        AlphaBlendEnable = true;
        SrcBlend = SrcAlpha;
		DestBlend = InvSrcAlpha;
        ZWriteEnable = false;
        ZEnable          = False;
        VertexShader = compile vs_2_0 vsStars();
        PixelShader  = compile ps_2_0 psStars(false);
    }
}

technique tStarsHDR
{
    pass p0
    {
        CullMode = NONE;
        AlphaRef = 0;
        ColorWriteEnable = RED|GREEN|BLUE;
        AlphaBlendEnable = true;
        SrcBlend = SrcAlpha;
		DestBlend = InvSrcAlpha;
        ZWriteEnable = false;
        ZEnable          = False;
        VertexShader = compile vs_2_0 vsStars();
        PixelShader  = compile ps_2_0 psStars(true);
    }
}

//------------------------------------------------------------------------------
/**
    Techniques for shader "stars"
*/
technique tHorizonDepth
{
    pass p0
    {
	    ZWriteEnable = False;
        CullMode     = None;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        Sampler[0]   = <DiffSampler>;
        VertexShader = compile vs_2_0 vsHorizonDepth(false);
        PixelShader  = compile ps_2_0 psHorizonDepth(false);
    }
}

technique tHorizonSaveZ
{
    pass p0
    {
	    ZWriteEnable = False;
        CullMode     = None;
        SrcBlend     = <AlphaSrcBlend>;
        DestBlend    = <AlphaDstBlend>;
        Sampler[0]   = <DiffSampler>;
        VertexShader = compile vs_2_0 vsHorizonDepth(true);
        PixelShader  = compile ps_2_0 psHorizonDepth(true);
    }
}

technique tHorizon
{
    pass p0
    {
        CullMode = NONE;
        AlphaRef = 0;
        AlphaBlendEnable = true;
        SrcBlend = SrcAlpha;
		DestBlend = InvSrcAlpha;
        ZWriteEnable = false;
        ZEnable          = False;
        VertexShader = compile vs_2_0 vsHorizon();
        PixelShader  = compile ps_2_0 psHorizon(false);
    }
}

technique tHorizonHDR
{
    pass p0
    {
        CullMode = NONE;
        AlphaRef = 0;
        AlphaBlendEnable = true;
        SrcBlend = SrcAlpha;
		DestBlend = InvSrcAlpha;
        ZWriteEnable = false;
        ZEnable          = False;
        VertexShader = compile vs_2_0 vsHorizon();
        PixelShader  = compile ps_2_0 psHorizon(true);
    }
}

//------------------------------------------------------------------------------
/**
    Techniques for shader "copytexture_rgb"
*/
technique tCopyTexture_rgb
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

        VertexShader = compile vs_2_0 vsCopyTexture();
        PixelShader = compile ps_2_0 psCopyTexture_rgb();
    }
}