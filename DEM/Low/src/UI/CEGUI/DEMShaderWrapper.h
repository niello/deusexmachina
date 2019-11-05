#pragma once
#include <Render/ShaderParamStorage.h>
#include <CEGUI/ShaderWrapper.h>
#include <CEGUI/Renderer.h>
#include <vector>
#include <map>

namespace CEGUI
{
class CDEMRenderer;

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

	CDEMRenderer&    _Renderer;
	Render::PEffect  _Effect;
	CStrID           _CurrInputSet;
	CTechCache*      _pCurrCache = nullptr;
	Render::PSampler _LinearSampler; //???can define in effect?

	std::map<CStrID, CTechCache> _TechCache;

public:

	CDEMShaderWrapper(CDEMRenderer& Owner, Render::CEffect& Effect);
	virtual ~CDEMShaderWrapper();

	void         setInputSet(BlendMode BlendMode, bool Clipped, bool Opaque);
	virtual void prepareForRendering(const ShaderParameterBindings* shaderParameterBindings) override;
};

}
