#pragma once
#include <Render/ShaderParamStorage.h>
#include <CEGUI/ShaderWrapper.h>
#include <CEGUI/Renderer.h>
#include <vector>
#include <map>

namespace CEGUI
{
class CDEMRenderer;
typedef std::unique_ptr<class CDEMShaderWrapper> PDEMShaderWrapper;

class CDEMShaderWrapper: public ShaderWrapper
{
protected:

	struct CTechCache
	{
		const Render::CTechnique*    pTech = nullptr;
		Render::IResourceParam*      pMainTextureParam = nullptr;
		Render::ISamplerParam*       pLinearSamplerParam = nullptr;
		Render::CShaderConstantParam WVPParam;
		Render::CShaderConstantParam AlphaPercentageParam;
		Render::CShaderParamStorage  Storage;
	};

	Render::PGPUDriver _GPU;
	Render::PEffect    _Effect;
	CStrID             _CurrInputSet;
	CTechCache*        _pCurrCache = nullptr;
	Render::PSampler   _LinearSampler; //!!!???can define in effect?!

	std::map<CStrID, CTechCache> _TechCache;

public:

	CDEMShaderWrapper(Render::PGPUDriver GPU, Render::CEffect& Effect, Render::PSampler LinearSampler);
	virtual ~CDEMShaderWrapper() override;

	void         setInputSet(BlendMode BlendMode, bool Clipped, bool Opaque);
	void         resetInputSet() { _CurrInputSet = CStrID::Empty; }
	virtual void prepareForRendering(const ShaderParameterBindings* shaderParameterBindings) override;
};

}
