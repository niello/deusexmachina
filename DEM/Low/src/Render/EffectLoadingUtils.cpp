#include <Render/Shader.h>
#include <Render/ShaderLibrary.h>
#include <Render/ShaderMetadata.h>
#include <Render/ShaderConstant.h>
#include <Render/Texture.h>
#include <Render/TextureLoader.h>
#include <Render/SamplerDesc.h>
#include <Render/Sampler.h>
#include <Render/GPUDriver.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Data/FixedArray.h>

// Utility functions for loading data blocks common to several effect-related formats (EFF, RP, MTL).
// No header file exist, include function declarations into other translation units instead.

namespace Resources
{

}