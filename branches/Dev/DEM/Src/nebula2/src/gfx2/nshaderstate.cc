//------------------------------------------------------------------------------
//  nshaderstate.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/nshaderstate.h"
#include <string.h>

/// the shader parameter type string table
static const char* TypeTable[nShaderState::NumTypes] =
{
    "v",
    "b",
    "i",
    "f",
    "f4",
    "m44",
    "t"
};

/// the shader parameter name string table
static const char* StateTable[nShaderState::NumParameters] =
{
    "Model",
    "InvModel",
    "View",
    "InvView",
    "Projection",
    "ModelView",
    "InvModelView",
    "ModelViewProjection",
    "ModelShadowProjection",
    "EyeDir",
    "EyePos",
    "ModelEyePos",
    "ModelLightPos",
    "LightPos",
    "LightType",
    "LightRange",
    "LightAmbient",
    "LightDiffuse",
    "LightDiffuse1",
    "LightSpecular",
    "MatAmbient",
    "MatDiffuse",
    "MatEmissive",
    "MatEmissiveIntensity",
    "MatSpecular",
    "MatSpecularPower",
    "MatTransparency",
    "MatFresnel",
    "Scale",
    "Noise",
    "MatTranslucency",
    "AlphaRef",
    "CullMode",
    "DirAmbient",
    "FogDistances",
    "FogColor",
    "DiffMap0",
    "DiffMap1",
    "DiffMap2",
    "DiffMap3",
    "DiffMap4",
    "DiffMap5",
    "DiffMap6",
    "DiffMap7",
    "SpecMap0",
    "SpecMap1",
    "SpecMap2",
    "SpecMap3",
    "AmbientMap0",
    "AmbientMap1",
    "AmbientMap2",
    "AmbientMap3",
    "BumpMap0",
    "BumpMap1",
    "BumpMap2",
    "BumpMap3",
    "CubeMap0",
    "CubeMap1",
    "CubeMap2",
    "CubeMap3",
    "NoiseMap0",
    "NoiseMap1",
    "NoiseMap2",
    "NoiseMap3",
    "LightModMap",
    "ShadowMap",
    "SpecularMap",
    "ShadowModMap",
    "JointPalette",
    "Time",
    "Wind",
    "Swing",
    "InnerLightIntensity",
    "OuterLightIntensity",
    "BoxMinPos",
    "BoxMaxPos",
    "BoxCenter",
    "MinDist",
    "MaxDist",
    "SpriteSize",
    "MinSpriteSize",
    "MaxSpriteSize",
    "SpriteSwingAngle",
    "SpriteSwingTime",
    "SpriteSwingTranslate",
    "DisplayResolution",
    "TexGenS",
    "TexGenT",
    "TexGenR",
    "TexGenQ",
    "TextureTransform0",
    "TextureTransform1",
    "TextureTransform2",
    "TextureTransform3",
    "SampleOffsets",
    "SampleWeights",
    "VertexStreams",
    "VertexWeights1",
    "VertexWeights2",
    "AlphaBlendEnable",
    "AlphaSrcBlend",
    "AlphaDstBlend",
    "BumpScale",
    "FresnelBias",
    "FresnelPower",
    "Intensity0",
    "Intensity1",
    "Intensity2",
    "Intensity3",
    "Amplitude",
    "Frequency",
    "Velocity",
    "StencilFrontZFailOp",
    "StencilFrontPassOp",
    "StencilBackZFailOp",
    "StencilBackPassOp",
    "ZWriteEnable",
    "ZEnable",
    "ShadowIndex",
    "CameraFocus",
    "Color0",
    "Color1",
    "Color2",
    "Color3",
    "HalfPixelSize",
    "MLPUVStretch",
    "MLPSpecIntensity",
    "UVStretch0",
    "UVStretch1",
    "UVStretch2",
    "UVStretch3",
    "UVStretch4",
    "UVStretch5",
    "UVStretch6",
    "UVStretch7",
    "LeafCluster",
    "LeafAngleMatrices",
    "WindMatrices",
    "RenderTargetOffset",
    "RenderComplexity",
    "SkyBottom",
    "SunFlat",
    "SunRange",
    "SunColor",
    "CloudMod",
    "CloudPos",
    "CloudGrad",
    "Brightness",
    "Lightness",
    "Density",
    "Glow",
    "Saturation",
    "Weight",
    "TopColor",
    "BottomColor",
    "Move",
    "Position",
    "ScaleVector",
    "Map0uvRes",
    "Map1uvRes",
    "BumpFactor",
};

//------------------------------------------------------------------------------
/**
*/
const char*
nShaderState::TypeToString(nShaderState::Type t)
{
    n_assert((t >= 0) && (t < nShaderState::NumTypes));
    return TypeTable[t];
}

//------------------------------------------------------------------------------
/**
*/
nShaderState::Type
nShaderState::StringToType(const char* str)
{
    n_assert(str);
    int i;
    for (i = 0; i < nShaderState::NumTypes; i++)
    {
        if (0 == strcmp(str, TypeTable[i]))
        {
            return (nShaderState::Type) i;
        }
    }
    // fallthrough: state not found
    return nShaderState::InvalidType;
}

//------------------------------------------------------------------------------
/**
*/
const char*
nShaderState::ParamToString(nShaderState::Param p)
{
    n_assert((p >= 0) && (p < nShaderState::NumParameters));
    return StateTable[p];
}

//------------------------------------------------------------------------------
/**
*/
nShaderState::Param
nShaderState::StringToParam(const char* str)
{
    n_assert(str);
    int i;
    for (i = 0; i < nShaderState::NumParameters; i++)
    {
        if (0 == strcmp(str, StateTable[i]))
        {
            return (nShaderState::Param) i;
        }
    }
    // fallthrough: state not found
    return nShaderState::InvalidParameter;
}
