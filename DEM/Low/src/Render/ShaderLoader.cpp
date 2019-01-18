#include "ShaderLoader.h"
#include <Render/GPUDriver.h>
#include <Render/ShaderLibrary.h>

namespace Resources
{
__ImplementClassNoFactory(Resources::CShaderLoader, Resources::IResourceCreator);

CShaderLoader::CShaderLoader() {}
CShaderLoader::~CShaderLoader() {}

PResourceLoader CShaderLoader::Clone()
{
	PShaderLoader NewLoader = (CShaderLoader*)GetRTTI()->CreateClassInstance();
	NewLoader->GPU = GPU;
	NewLoader->ShaderLibrary = ShaderLibrary;
	return NewLoader.Get();
}
//---------------------------------------------------------------------

}