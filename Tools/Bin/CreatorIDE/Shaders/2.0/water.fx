//------------------------------------------------------------------------------
//  water.fx
//
//  Water shader from Mathias.
//
//  (C) 2005 RadonLabs GmbH
//------------------------------------------------------------------------------
#line 2 "water.fx"

#include "shaders:../lib/std.fx"

shared float4x4 InvView;
shared float4x4 ModelViewProjection;
shared float4x4 Model;

shared float Time;

float4 MatDiffuse;
float4 MatSpecular;

float4 TexGenS;                     // texgen parameters for u
float4 TexGenT;                     // texgen parameters for v

float4 Velocity = float4(0.0, 0.0, 0.0, 0.0); // UVAnimaton2D, Water movement

float BumpScale = 0.0f;

float FresnelBias;
float FresnelPower;

bool  AlphaBlendEnable = true;
int CullMode = 2;

texture DiffMap1;
texture DiffMap2;
texture BumpMap0;

//------------------------------------------------------------------------------
/**
    shader input/output structure definitions
*/
struct vsInputWater
{
    float4 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv0      : TEXCOORD0;
    float3 tangent  : TANGENT;
    float3 binormal : BINORMAL;
};

struct vsOutputWater
{
    float4 position     : POSITION;
    float2 uv0          : TEXCOORD0;
    float3 row0         : TEXCOORD1; // first row of the 3x3 transform from tangent to cube space
    float3 row1         : TEXCOORD2; // second row of the 3x3 transform from tangent to cube space
    float3 row2         : TEXCOORD3; // third row of the 3x3 transform from tangent to cube space
    float4 bumpCoord0   : TEXCOORD4;
    float4 pos          : TEXCOORD5;
    float3 eyeVector    : TEXCOORD6;
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

sampler Diff2Sampler = sampler_state
{
    Texture = <DiffMap2>;
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
//  vertex shader
//------------------------------------------------------------------------------
vsOutputWater vsWater(const vsInputWater vsIn)
{
    vsOutputWater vsOut;

    float4 pos = vsIn.position;

    // compute output vertex position
    vsOut.position = mul(pos, ModelViewProjection);

    //position for calculate texture in screenspace (pixelshader)
    vsOut.pos = vsOut.position;

    // pass texture coordinates for fetching the normal map
    vsOut.uv0.xy = vsIn.uv0;

    float modTime = fmod(Time, 100.0f);
    vsOut.bumpCoord0.xy = vsIn.uv0 * TexGenS.xy + modTime * Velocity.xy;
    vsOut.bumpCoord0.zw = vsIn.uv0 * TexGenS.xy + modTime * Velocity.xy * -1.0f;

    // compute the 3x3 tranform from tangent space to object space
    // first rows are the tangent and binormal scaled by the bump scale
    float3x3 objToTangentSpace;

    objToTangentSpace[0] = BumpScale * vsIn.tangent;
    objToTangentSpace[1] = BumpScale * cross(vsIn.normal, vsIn.tangent);
    objToTangentSpace[2] = vsIn.normal;

    vsOut.row0.xyz = mul(objToTangentSpace, Model[0].xyz);
    vsOut.row1.xyz = mul(objToTangentSpace, Model[1].xyz);
    vsOut.row2.xyz = mul(objToTangentSpace, Model[2].xyz);

    // compute the eye vector (going from shaded point to eye) in cube space
    float4 worldPos = mul(pos, Model);

    vsOut.eyeVector = InvView[3] - worldPos; // view inv. transpose contains eye position in world space in last row

    return vsOut;
}

//------------------------------------------------------------------------------
//  pixel shader ueberwasser
//------------------------------------------------------------------------------
float4 psWater(vsOutputWater psIn, uniform bool hdr, uniform bool refraction) : COLOR
{
    // compute the normals
    // with 2 texture it looks more intressting
    float4 N0 = tex2D(BumpSampler, psIn.bumpCoord0.xy) * 2.0 - 1.0;
    float4 N1 = tex2D(BumpSampler, psIn.bumpCoord0.zw) * 2.0 - 1.0;

    // add both normals
    float3 N = N0.xyz + N1.xyz;

    // bring normals in worldspace
    half3x3 m;
    m[0] = psIn.row0;
    m[1] = psIn.row1;
    m[2] = psIn.row2;
    float3 Nw = mul(m, N.xyz);
    Nw = normalize(Nw);

    // compute screen pos for the reflection refraction texture because both textures are in screencoordinates
    float4 pos = psIn.pos;

    // prepare pos - pos is in model view projection when pos.x and pos.y get divided with pos.w
    // you get a value between -1 and 1 but you need a number between 0 and 1
    // later pos.x and pos.y will divided with pos.w so here we precalculate
    pos.x += psIn.pos.w;
    pos.w  = 2 * psIn.pos.w;

    // compute the distortion and prepare pos for texture lookup
    float3 E = normalize(psIn.eyeVector);

    // angle between eyevector and normal important for fresnel and distortion
    float angle = dot(E, Nw);

    //compute fresnel
    float facing = 1.0 - max(angle, 0);
    float fresnel = FresnelBias + (1.0 - FresnelBias) * pow(facing , FresnelPower);

    // compute distortion - the math is not correct but this is more faster information in my document
    // is computed with angle, because when you look from the side the distortion is stronger then you look from over the water
    // is computed with pos.z, because the distortions are in screencoordinates so the distortions have to get smaller with the distance
    float4 disVecRefract    = float4(Nw.x, Nw.y, Nw.z, 0.0) * (1 - angle) * 0.4 / pos.z * pos.w;

    // refraction rotate the refraction texture with 180 degree
    pos.y  = -psIn.pos.y + psIn.pos.w;

    float refractLookUp;
    float4 refractionComp;
    if(refraction)
    {
        // read the depth from the water with full distortion - this is pasted in the alpha value
        refractLookUp   = tex2Dproj(Diff2Sampler, pos + disVecRefract).a;
        // read the refractioncolor with distortion multiplied with the depth of the water so the distortion is at the strand smaller
        refractionComp  = tex2Dproj(Diff2Sampler, pos + disVecRefract * refractLookUp);
        // apply water color in deep direction
        refractionComp.rgb = lerp(refractionComp.rgb, MatDiffuse.rgb, saturate(refractionComp.a));
    }
    else
    {
        // read the depth from the water WITHOUT distortion - this is pasted in the alpha value
        refractLookUp   = tex2Dproj(Diff2Sampler, pos).a;
        // init the refractioncolor
        refractionComp  = float4(0.0, 0.0, 0.0, refractLookUp);
        // apply water color in deep direction
        refractionComp.rgb = MatDiffuse.rgb;
    }

    // apply water color in shallow direction
    refractionComp.rgb = lerp(refractionComp.rgb, MatSpecular.rgb, facing * refractionComp.a);

    // reflection
    pos.y = psIn.pos.y + psIn.pos.w;

    // same as refraction
    float reflectionLookUp  = tex2Dproj(Diff1Sampler, pos + disVecRefract).a;
    float4 reflectionComp   = tex2Dproj(Diff1Sampler, pos + disVecRefract * reflectionLookUp);

    // compute all together
    float4 colorWater;

    // reflection is multiplied with deep because so the reflection is on the strand less and you get soft edges
    colorWater = (1 - refractionComp.a) * refractionComp + refractionComp.a * reflectionComp * fresnel;
    colorWater.a = refraction ? 1.0f : refractLookUp;

    if (hdr)
    {
        return EncodeHDR(colorWater);
    }
    else
    {
        return colorWater;
    }
}

//------------------------------------------------------------------------------
technique tWater
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = true;
        SrcBlend  = SrcAlpha;
        DestBlend = InvSrcAlpha;
        VertexShader = compile vs_2_0 vsWater();
        PixelShader  = compile ps_2_0 psWater(false, false);
    }
}

technique tWaterHDR
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = true;
        SrcBlend  = SrcAlpha;
        DestBlend = InvSrcAlpha;
        VertexShader = compile vs_2_0 vsWater();
        PixelShader  = compile ps_2_0 psWater(true, false);
    }
}

technique tWaterHDRRefraction
{
    pass p0
    {
        CullMode = <CullMode>;
        AlphaBlendEnable = <AlphaBlendEnable>;
        VertexShader = compile vs_2_0 vsWater();
        PixelShader  = compile ps_2_0 psWater(true, true);
    }
}