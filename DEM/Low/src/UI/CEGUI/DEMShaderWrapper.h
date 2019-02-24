#pragma once
#include <Render/RenderFwd.h>
#include <CEGUI/ShaderWrapper.h>
#include <CEGUI/Renderer.h>

namespace CEGUI
{
class CDEMRenderer;

//!!!texture & sampler are not needed for non-textured shader!

class CDEMShaderWrapper: public ShaderWrapper
{
protected:

	CDEMRenderer&						Renderer;

	Render::PRenderState				NormalUnclipped;
	Render::PRenderState				NormalClipped;
	Render::PRenderState				PremultipliedUnclipped;
	Render::PRenderState				PremultipliedClipped;
	Render::PRenderState				OpaqueUnclipped;
	Render::PRenderState				OpaqueClipped;

	Render::PConstantBuffer				WMCB;
	Render::PConstantBuffer				PMCB;
	Render::PSampler					LinearSampler;
	Render::HConstantBuffer				hWMCB;
	Render::HConstantBuffer				hPMCB;
	Render::PShaderConstant				ConstWorldMatrix;
	Render::PShaderConstant				ConstProjMatrix;
	Render::HResource					hTexture;
	Render::HSampler					hLinearSampler;

public:

	CDEMShaderWrapper(CDEMRenderer& Owner, Render::PShader VS, Render::PShader PSRegular, Render::PShader PSOpaque);
	virtual ~CDEMShaderWrapper();

	void			bindRenderState(BlendMode BlendMode, bool Clipped, bool Opaque);
	virtual void	prepareForRendering(const ShaderParameterBindings* shaderParameterBindings) override;
};

}
