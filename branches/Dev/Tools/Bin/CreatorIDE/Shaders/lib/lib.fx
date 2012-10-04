//------------------------------------------------------------------------------
//  lib.fx
//
//  Support functions for RadonLabs shader library.
//
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------

#include "shaders:../lib/std.fx"

shared float4   HalfPixelSize;
shared float4   LightDiffuse;
shared float4   LightSpecular;
shared float4   LightAmbient;
shared int      LightType;
shared float    LightRange;
shared float4   ShadowIndex;

float3x3 Swing;                     // the swing rotation matrix
float3 BoxMinPos;                   // model space bounding box min
float3 BoxMaxPos;                   // model space bounding box max

float4 MatAmbient;
float4 MatDiffuse;
float  MatSpecularPower;
float4 MatSpecular;
float4 MatEmissive = float4(0.0f, 0.0f, 0.0f, 0.0f);
float  MatEmissiveIntensity = 0.0f;
float BumpScale = 0.0f;

shared texture AmbientMap1;
//------------------------------------------------------------------------------
/**
    sampler definitions
*/
sampler ShadowSampler = sampler_state
{
    Texture = <AmbientMap1>;
    AddressU  = Clamp;
    AddressV  = Clamp;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = None;
};

//------------------------------------------------------------------------------
/**
	transformStatic()

	Transform position into modelview-projection space without deformations.

	@param	pos		a position in model space
	@param	mvp		the modelViewProjection matrix
	@return         transformed position
*/
float4
transformStatic(const float3 pos, const float4x4 mvp)
{
	return mul(float4(pos, 1.0), mvp);
}

//------------------------------------------------------------------------------
/**
    skinnedPosition()

    Compute a skinned position.

    @param  inPos           input vertex position
    @param  weights         4 weights
    @param  indices         4 joint indices into the joint palette
    @param  jointPalette    joint palette as vector4 array
    @return                 the skinned position
*/
float4
skinnedPosition(const float4 inPos, const float4 weights, const float4 indices, const matrix<float,4,3> jointPalette[72])
{
    float3 pos[4];

    int i;
    for (i = 0; i < 4; i++)
    {
        pos[i] = (mul(inPos, jointPalette[indices[i]])) * weights[i];
    }
    return float4(pos[0] + pos[1] + pos[2] + pos[3], 1.0f);
}

//------------------------------------------------------------------------------
/**
    skinnedNormal()

    Compute a skinned normal vector (without renormalization).

    @param  inNormal        input normal vector
    @param  weights         4 weights
    @param  indices         4 joint indices into the joint palette
    @param  jointPalette    joint palette as vector4 array
    @return                 the skinned normal
*/
float3
skinnedNormal(const float3 inNormal, const float4 weights, const float4 indices, const matrix<float,4,3> jointPalette[72])
{
    float3 normal[4];
    int i;
    for (i = 0; i < 4; i++)
    {
        normal[i] = mul(inNormal, (matrix<float,3,3>)jointPalette[indices[i]]) * weights[i];
    }
    return float3(normal[0] + normal[1] + normal[2] + normal[3]);
}

//------------------------------------------------------------------------------
/**
	tangentSpaceVector()

	Compute an unnormalized tangent space vector from a vertex position, reference
	position, normal and tangent (all in model space). This will compute
	an unnormalized light vector, and a binormal behind the scene.
*/
float3
tangentSpaceVector(const float3 pos, const float3 refPos, const float3 normal, const float3 tangent)
{
    // compute the light vector in model space
    float3 vec = refPos - pos;

    // compute the binormal
    float3 binormal = cross(normal, tangent);

	// transform with transpose of tangent matrix!
	float3 outVec = mul(float3x3(tangent, binormal, normal), vec);
	return outVec;
}

//------------------------------------------------------------------------------
/**
	tangentSpaceHalfVector()

	Compute the unnormalized tangent space half vector from a vertex position, light
	position, eye position, normal and tangent (all in model space). This
	will compute a normalized lightVec, a normalized eyeVec, an unnormalized
	half vector and a binormal behind the scenes.
*/
float3
tangentSpaceHalfVector(const float3 pos,
                       const float3 lightPos,
                       const float3 eyePos,
                       const float3 normal,
                       const float3 tangent)
{
    // compute the light vector, eye vector and half vector in model space
    float3 lightVec = normalize(lightPos - pos);
    float3 eyeVec   = normalize(eyePos - pos);
    float3 halfVec  = lightVec + eyeVec;

    // compute the binormal
    float3 binormal = cross(normal, tangent);

	// transform with transpose of tangent matrix!
	float3 outVec = mul(float3x3(tangent, binormal, normal), halfVec);
    return outVec;
}

//------------------------------------------------------------------------------
/**
	tangentSpaceEyeHalfVector()

	Compute tangent space eye and half vectors.
*/
void
tangentSpaceEyeHalfVector(in const float3 pos,
                          in const float3 lightPos,
                          in const float3 eyePos,
                          in const float3 normal,
                          in const float3 tangent,
                          out float3 eyeVec,
                          out float3 halfVec)
{
    // compute the light vector, eye vector and half vector in model space
    float3 lVec = normalize(lightPos - pos);
    float3 eVec = normalize(eyePos - pos);
    float3 hVec = lVec + eVec;

    // compute the binormal and tangent matrix
    float3 binormal = cross(normal, tangent);
    float3x3 tangentMatrix = float3x3(tangent, binormal, normal);

	// transform with transpose of tangent matrix!
    eyeVec = mul(tangentMatrix, eVec);
    halfVec = mul(tangentMatrix, hVec);
}

//------------------------------------------------------------------------------
/**
	tangentSpaceLightHalfEyeVector()

	Compute tangent space light and half vectors.
*/
void
tangentSpaceLightHalfEyeVector(in const float3 pos,
                               in const float3 lightPos,
                               in const float3 eyePos,
                               in const float3 normal,
                               in const float3 tangent,
                               out float3 lightVec,
                               out float3 halfVec,
                               out float3 eyeVec)
{
    // compute the light vector, eye vector and half vector in model space
    float3 lVec = normalize(lightPos - pos);
    float3 eVec = normalize(eyePos - pos);
    float3 hVec = lVec + eVec;

    // compute the binormal and tangent matrix
    float3 binormal = cross(normal, tangent);
    float3x3 tangentMatrix = float3x3(tangent, binormal, normal);

	// transform with transpose of tangent matrix!
    lightVec = mul(tangentMatrix, lVec);
    halfVec  = mul(tangentMatrix, hVec);
    eyeVec   = mul(tangentMatrix, eVec);
}

//------------------------------------------------------------------------------
/**
    reflectionVector()

    Returns the eye vector reflected around the surface normal in world space.
*/
float3
reflectionVector(const float3 pos,
                 const float3 eyePos,
                 const float3 normal,
                 const float4x4 model)
{
    float3 eyeVec = eyePos - pos;
    float3 reflVec = reflect(eyeVec, normal);
    float3 worldVec = mul(reflVec, (float3x3)model);
    return 0.5f * (1.0f + normalize(worldVec));
}

//------------------------------------------------------------------------------
/**
    fog()

    Compute a distance/layer fog.

    @param  pos                     the current vertex position in model space
    @param  worldPos,               the current vertex position in world space
    @param  modelEyePos             the eye position in model space
    @param  fogDistances            fog plane distances, x=near, y=far, z=bottom, w=top
    @param  fogNearBottomColor      the color at the near bottom, rgb=color, a=intensity
    @param  fogNearTopColor         the color at the near top
    @param  fogFarBottomColor       the color at the far bottom
    @param  fogFarTopColor          the color at the far top
*/
float4
fog(const float3 pos,
    const float3 worldPos,
    const float3 modelEyePos,
    const float4 fogDistances,
    const float4 fogNearBottomColor,
    const float4 fogNearTopColor,
    const float4 fogFarBottomColor,
    const float4 fogFarTopColor)
{
    // get normalized vertical and horizontal distance
    float2 dist;
    dist.x = clamp(distance(pos.xz, modelEyePos.xz), fogDistances.x, fogDistances.y);
    dist.y = clamp(worldPos.y, fogDistances.z, fogDistances.w);
    dist.x = (dist.x - fogDistances.x) / (fogDistances.y - fogDistances.x);
    dist.y = (dist.y - fogDistances.z) / (fogDistances.w - fogDistances.z);

    // get 2 horizontal interpolated colors
    float4 topColor = lerp(fogNearTopColor, fogFarTopColor, dist.x);
    float4 bottomColor = lerp(fogNearBottomColor, fogFarBottomColor, dist.x);

    // get resulting fog color
    float4 fogColor = lerp(bottomColor, topColor, dist.y);
    return fogColor;
}

//------------------------------------------------------------------------------
/**
    shadow()

    Compute the shadow modulation color.

    @param  shadowPos           position in shadow space
    @param  noiseSampler        sampler with a noise texture
    @param  shadowMapSampler    sampler with shadow map
    @param  shadowModSampler    shadow modulation sampler to fade shadow color in/out
    @return                     a shadow modulation color
*/
float4
shadow(const float4 shadowPos, float distOffset, sampler shadowMapSampler)
{
    // get projected position in shadow space
    float3 projShadowPos = shadowPos.xyz / shadowPos.w;

    // jitter shadow map position using noise texture lookup
//    projShadowPos.xy += tex2D(noiseSampler, projShadowPos.xy * 1234.5f).xy * 0.0005f;

    // sample shadow depth from shadow map
    float4 shadowDepth = tex2D(shadowMapSampler, projShadowPos.xy) + distOffset;

    // in/out test
    float4 shadowModulate;
    if ((projShadowPos.x < 0.0f) ||
        (projShadowPos.x > 1.0f) ||
        (projShadowPos.y < 0.0f) ||
        (projShadowPos.y > 1.0f))
    {
        // outside shadow projection
        shadowModulate = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else if ((shadowDepth.x > projShadowPos.z) || (shadowPos.z > shadowPos.w))
    {
        // not in shadow
        shadowModulate = float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        shadowModulate = float4(0.5f, 0.5f, 0.5f, 1.0f);

        // in shadow
        //shadowModulate = tex2D(shadowModSampler, projShadowPos.xy);
        //float4 shadowColor = tex2D(shadowModSampler, projShadowPos.xy);
        //float4 blendColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
        //float relDist = saturate((projShadowPos.z - shadowDepth.x) * 20.0f);
        //shadowModulate = lerp(shadowColor, blendColor, relDist);
    }
    return shadowModulate;
}

//------------------------------------------------------------------------------
//  vsLighting
//
//  Vertex shader support function for simpler per-pixel lighting.
//
//  @param  pos                 [in] vertex position
//  @param  normal              [in] vertex normal
//  @param  tangent             [in] vertex tangent
//  @param  primLightVec        [out] xyz: primary light vector in tangent space, w: distance to light
//  @param  primHalfVec         [out] primary half vector in tangent space
//------------------------------------------------------------------------------
void
vsLighting(in const float4 pos,
           in const float3 normal,
           in const float3 tangent,
           in const float3 lightPos,
           in const float3 eyePos,
           out float3 lightVec,
           out float3 halfVec)
{
    // compute the light vector, eye vector and half vector in model space
    float3 lVec = lightPos - pos;
    float3 eVec = eyePos - pos;
    float3 hVec = normalize(normalize(lVec) + normalize(eVec));

    // compute the binormal and tangent matrix
    float3 binormal = cross(normal, tangent);
    float3x3 tangentMatrix = float3x3(tangent, binormal, normal);

	// transform with transpose of tangent matrix!
    lightVec.xyz = mul(tangentMatrix, lVec);
    halfVec      = mul(tangentMatrix, hVec);
}

//------------------------------------------------------------------------------
//  psLighting
//
//  Pixel shader functionality for simple per-pixel lighting.
//
//  @param  mapColor                the diffuse color
//  @param  tangentSurfaceNormal
//  @param  primLightVec            the primary light vector
//  @param  primHalfVec             the primary light half vector
//------------------------------------------------------------------------------
float4
psLighting(in float4 mapColor,
           in float3 tangentSurfaceNormal,
           in const float3 primLightVec,
           in const float3 primHalfVec)
{
    // compute light intensities
    float primDiffIntensity = saturate(dot(tangentSurfaceNormal, normalize(primLightVec)));
    float primSpecIntensity = pow(saturate(dot(tangentSurfaceNormal, normalize(primHalfVec))), MatSpecularPower);

    // compute light colors
    float4 diffColor = primDiffIntensity * mapColor * LightDiffuse[0] * MatDiffuse;
    float4 specColor = primSpecIntensity * LightSpecular[0] * MatSpecular * primDiffIntensity;
    float4 envColor  = mapColor * LightAmbient[0] * MatAmbient;
    float4 color;
    color.rgba = (diffColor + specColor) + envColor;
    color.a = mapColor.a;
    return color;
}

//------------------------------------------------------------------------------
//  psLightingAlpha
//
//  Pixel shader functionality for simple per-pixel lighting. The specular
//  highlight will also increase the alpha channels value (nice for glass
//  and other transparent surfaces).
//
//  @param  diffSampler         the diffuse texture sampler
//  @param  bumpSampler         the bump map sampler
//  @param  uv                  uv coordinates for diffuse and bump sampler
//  @param  primLightVec        the primary light vector
//  @param  primHalfVec         the primary light half vector
//  @param  primDiffuse         the primary diffuse color
//  @param  primSpecular        the primary specular color
//  @param  primAmbient         the primary ambient color
//  @param  matSpecularPower    the material specular power
//------------------------------------------------------------------------------
float4
psLightingAlpha(in const float4 mapColor,
                in const float3 tangentSurfaceNormal,
                in const float3 primLightVec,
                in const float3 primHalfVec,
                in const float  specModulate)
{
    // compute light intensities
    float dotNL = dot(tangentSurfaceNormal, normalize(primLightVec));
    float dotNH = dot(tangentSurfaceNormal, normalize(primHalfVec));
    float primDiffIntensity = saturate(dotNL);
    float primSpecIntensity = pow(saturate(dotNH), MatSpecularPower);

    // compute light colors
    float4 diffColor = (primDiffIntensity * LightDiffuse[0] * MatDiffuse) * mapColor;
    float4 specColor = primSpecIntensity * LightSpecular[0] * MatSpecular * primDiffIntensity * specModulate;
    float4 envColor  = mapColor * LightAmbient[0] * MatAmbient;
    float4 color;
    color.rgba = (diffColor + specColor) + envColor;
    color.a = mapColor.a + primSpecIntensity;
    return color;
}

//------------------------------------------------------------------------------
//  psLightingLeaf
//
//  Pixel shader functionality for simple lighting specialized for leafs.
//
//  @param  diffSampler         the diffuse texture sampler
//  @param  primLightVec        the primary light vector
//  @param  primDiffuse         the primary diffuse color
//------------------------------------------------------------------------------
float4
psLightingLeaf(in sampler diffSampler,
               in const float2 uv,
               in const float4 primDiffuse,
               in const float4 primAmbient)
{
    // sample diffuse and bump texture
    float4 mapColor = tex2D(diffSampler, uv);

    // compute light colors
    float4 diffColor = mapColor * primDiffuse;
    float4 envColor  = mapColor * primAmbient;
    float4 color;
    color.rgb = diffColor + envColor;
    color.a = mapColor.a;
    return color;
}

//------------------------------------------------------------------------------
/**
    vsExpFog()

    Compute exponential fog in vertex shader. Returns fog color in rgb and
    fog density in a.

    legend:
    fogParams.x     -> fog layer ground height
    fogParams.y     -> fog horizontal density (EXP)
    fogParams.z     -> fog vertical density (EXP)
*/
float
vsExpFog(in const float3 modelVertexPos,
         in const float3 worldVertexPos,
         in const float3 modelEyePos,
         in const float4 fogParams)
{
    const float e = 2.71828;
    float eyeDist = distance(modelVertexPos, modelEyePos);
    float vertDist = max(worldVertexPos.y - fogParams.x, 0);
    float heightModulate = 1.0 / pow(e, vertDist * fogParams.z);
    float fogDensity = 1.0 / pow(e, eyeDist * fogParams.y);
    return heightModulate * (1.0 - fogDensity);
}

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
    Vertex shader part for per-pixel-lighting. Computes the light
    and half vector in tangent space which are then passed to the
    interpolators.
*/
void vsLight(in float3 position,
             in float3 normal,
             in float3 tangent,
             in float3 binormal,
             in float3 modelEyePos,
             in float3 modelLightPos,
             out float3 tangentLightVec,
             out float3 modelLightVec,
             out float3 halfVec,
             out float3 tangentEyePos)
{
    float3 eVec = normalize(modelEyePos - position);
    if (LightType == 0)
    {
        // point light
        modelLightVec = modelLightPos - position;
    }
    else
    {
        // directional light
        modelLightVec = modelLightPos;
    }
    float3 hVec = normalize(normalize(modelLightVec) + eVec);
    float3x3 tangentMatrix = float3x3(tangent, binormal, normal);
    tangentLightVec = mul(tangentMatrix, modelLightVec);
    halfVec = mul(tangentMatrix, hVec);
    tangentEyePos = normalize(mul(tangentMatrix, eVec));
}

//------------------------------------------------------------------------------
/**
    Compute per-pixel lighting.

    NOTE: lightVec.w contains the distance to the light source
*/
color4 psLight(in color4 mapColor, in float3 tangentSurfaceNormal, in float3 lightVec, in float3 modelLightVec, in float3 halfVec, in half shadowValue)
{
    color4 color = mapColor * color4(LightAmbient.rgb + MatEmissive.rgb * MatEmissiveIntensity, MatDiffuse.a);

    // light intensities
    half specIntensity = pow(saturate(dot(tangentSurfaceNormal, normalize(halfVec))), MatSpecularPower); // Specular-Modulation * mapColor.a;
    half diffIntensity = dot(tangentSurfaceNormal, normalize(lightVec));

    color3 diffColor = mapColor.rgb * LightDiffuse.rgb * MatDiffuse.rgb;
    color3 specColor = specIntensity * LightSpecular.rgb * MatSpecular.rgb;

    // attenuation
    if (LightType == 0)
    {
        // point light source
        diffIntensity *= shadowValue * (1.0f - saturate(length(modelLightVec) / LightRange));
    }
    else
    {
        #if DIRLIGHTS_ENABLEOPPOSITECOLOR
        color.rgb += saturate(-diffIntensity) * DIRLIGHTS_OPPOSITECOLOR * mapColor.rgb * MatDiffuse.rgb;
        #endif
        diffIntensity *= shadowValue;
    }
    color.rgb += saturate(diffIntensity) * (diffColor.rgb + specColor.rgb);
    return color;
}

//------------------------------------------------------------------------------
/**
    Vertex shader part to compute texture coordinate shadow texture lookups.
*/
float4 vsComputeScreenCoord(float4 pos)
{
    return pos;
}

//------------------------------------------------------------------------------
/**
    Pixel shader part to compute texture coordinate shadow texture lookups.
*/
float2 psComputeScreenCoord(float4 pos)
{
    float2 screenCoord = ((pos.xy / pos.ww) * float2(0.5, -0.5)) + float2(0.5f, 0.5f);
    screenCoord += HalfPixelSize.xy;
    return screenCoord;
}

//------------------------------------------------------------------------------
/**
    Compute shadow value. Returns a shadow intensity between 1 (not in shadow)
    and 0 (fully in shadow)
*/
half psShadow(in float2 screenCoord)
{
    // determine if pixel is in shadow
    color4 shadow = tex2D(ShadowSampler, screenCoord);
    half shadowValue = 1.0 - saturate(length(ShadowIndex * shadow));
    return shadowValue;
}

//------------------------------------------------------------------------------
/**
    Returns the uv offset for parallax mapping
*/
half2 ParallaxUv(float2 uv, sampler bumpMap, float3 eyeVect)
{
    return ((tex2D(bumpMap, uv).a - 0.5) * (BumpScale / 1000.0f)) * eyeVect;
}

//------------------------------------------------------------------------------
/**
    Helper function for tree and leaf vertex shaders which computes
    the swayed vertex position.

    @param  inPosition  input model space position
    @return             swayed model space position
*/
float4 ComputeSwayedPosition(const float4 inPosition)
{
    float ipol = (inPosition.y - BoxMinPos.y) / (BoxMaxPos.y - BoxMinPos.y);
    float4 rotPosition  = float4(mul(Swing, inPosition), 1.0f);
    return lerp(inPosition, rotPosition, ipol);
}
