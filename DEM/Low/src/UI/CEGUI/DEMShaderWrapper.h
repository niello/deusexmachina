#pragma once
#include <Render/ShaderParamTable.h> // FIXME: ShaderParams / ShaderConstant?
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

	//???need? or use storage?
	struct CConstRecord
	{
		CStrID							Name;
		Render::CShaderConstantParam	Constant;
		Render::PConstantBuffer			Buffer;
		Render::EShaderType				ShaderType;
	};

	CDEMRenderer&						Renderer;

	Render::PRenderState				RegularUnclipped;
	Render::PRenderState				RegularClipped;
	Render::PRenderState				PremultipliedUnclipped;
	Render::PRenderState				PremultipliedClipped;
	Render::PRenderState				OpaqueUnclipped;
	Render::PRenderState				OpaqueClipped;

	const Render::CShaderParamTable*	pVSParams = nullptr;
	const Render::CShaderParamTable*	pPSParams = nullptr;

	std::vector<CConstRecord>			Constants;

	// Main texture (CEGUI 'texture0')
	Render::PResourceParam				TextureParam;
	Render::PSamplerParam				LinearSamplerParam;
	Render::PSampler					LinearSampler;

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
