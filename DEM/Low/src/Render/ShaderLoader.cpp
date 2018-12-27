#include "ShaderLoader.h"

namespace Resources
{
__ImplementClassNoFactory(Resources::CShaderLoader, Resources::CResourceLoader);

PResourceLoader CShaderLoader::Clone()
{
	PShaderLoader NewLoader = (CShaderLoader*)GetRTTI()->CreateClassInstance();
	NewLoader->GPU = GPU;
	NewLoader->ShaderLibrary = ShaderLibrary;
	return NewLoader.GetUnsafe();
}
//---------------------------------------------------------------------

}