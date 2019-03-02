#pragma once
#include <Render/RenderFwd.h>
#include <CEGUI/ShaderWrapper.h>
#include <CEGUI/Renderer.h>
#include <vector>

namespace CEGUI
{
class CDEMRenderer;

//!!!texture & sampler are not needed for non-textured shader!

class CDEMShaderWrapper: public ShaderWrapper
{
protected:

	struct CConstRecord
	{
		CStrID					Name;
		Render::PShaderConstant	Constant;
		Render::PConstantBuffer	Buffer;
		Render::EShaderType		ShaderType;
	};

	CDEMRenderer&						Renderer;

	Render::PRenderState				RegularUnclipped;
	Render::PRenderState				RegularClipped;
	Render::PRenderState				PremultipliedUnclipped;
	Render::PRenderState				PremultipliedClipped;
	Render::PRenderState				OpaqueUnclipped;
	Render::PRenderState				OpaqueClipped;

	const Render::IShaderMetadata*		pVSMeta = nullptr;
	const Render::IShaderMetadata*		pPSMeta = nullptr;

	std::vector<CConstRecord>			Constants;

	// Main texture (CEGUI 'texture0')
	Render::HResource					hTexture = INVALID_HANDLE;
	Render::HSampler					hLinearSampler = INVALID_HANDLE;
	Render::PSampler					LinearSampler;

	//Render::PConstantBuffer				WMCB;
	//Render::PConstantBuffer				PMCB;
	//Render::HConstantBuffer				hWMCB;
	//Render::HConstantBuffer				hPMCB;
	//Render::PShaderConstant				ConstWorldMatrix;
	//Render::PShaderConstant				ConstProjMatrix;

	void			setupParameterForShader(CStrID Name, Render::EShaderType ShaderType);

public:

	CDEMShaderWrapper(CDEMRenderer& Owner, Render::PShader VS, Render::PShader PSRegular, Render::PShader PSOpaque);
	virtual ~CDEMShaderWrapper();

	void			setupParameter(const char* pName);
	void			setupMainTexture(const char* pTextureName, const char* pSamplerName);
	void			bindRenderState(BlendMode BlendMode, bool Clipped, bool Opaque) const;
	virtual void	prepareForRendering(const ShaderParameterBindings* shaderParameterBindings) override;
};

}
