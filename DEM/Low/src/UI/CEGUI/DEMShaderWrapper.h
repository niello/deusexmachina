#pragma once
#include <CEGUI/ShaderWrapper.h>

namespace Render
{
	class IShaderMetadata;
}

namespace CEGUI
{

class CDEMShaderWrapper: public ShaderWrapper
{
protected:

	const Render::IShaderMetadata* pVSMetadata = nullptr;
	const Render::IShaderMetadata* pPSMetadata = nullptr;

	// CB instances here? in param record may store variable handle + actual CB!

public:

	CDEMShaderWrapper(/*CDEMRenderer& owner,*/const Render::IShaderMetadata* pVSMeta, const Render::IShaderMetadata* pPSMeta);
	//virtual ~CDEMShaderWrapper();

	virtual void prepareForRendering(const ShaderParameterBindings* shaderParameterBindings) override;
};

}
