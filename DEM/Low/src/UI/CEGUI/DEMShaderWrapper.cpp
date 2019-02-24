#include <StdCfg.h>
#include "DEMShaderWrapper.h"

//#include <Render/GPUDriver.h>
//#include <UI/CEGUI/DEMRenderer.h>
#include <CEGUI/ShaderParameterBindings.h>

namespace CEGUI
{

CDEMShaderWrapper::CDEMShaderWrapper(/*CDEMRenderer& owner,*/const Render::IShaderMetadata* pVSMeta, const Render::IShaderMetadata* pPSMeta)
{
	// prepare string -> handle mapping to avoid scanning both shaders each time
	// or do it externally in "addParam"
}
//---------------------------------------------------------------------

void CDEMShaderWrapper::prepareForRendering(const ShaderParameterBindings* shaderParameterBindings)
{
	// bind shader, using current renderer settings like opacity etc

	// begin CB writing

	const ShaderParameterBindings::ShaderParameterBindingsMap& paramMap = shaderParameterBindings->getShaderParameterBindings();
	for (auto&& param : paramMap)
	{
		// set param
		// may prepare string -> handle mapping to avoid scanning both shaders each time
	}

	// end CB writing
}
//---------------------------------------------------------------------

}
