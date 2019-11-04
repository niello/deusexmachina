#pragma once
#include <Render/ShaderParamTable.h>
#include <CEGUI/ShaderWrapper.h>
#include <CEGUI/Renderer.h>
#include <vector>

namespace CEGUI
{
class CDEMRenderer;

class CDEMShaderWrapper: public ShaderWrapper
{
protected:

	CDEMRenderer&    _Renderer;
	Render::PEffect  _Effect;
	CStrID           _CurrInputSet;
	Render::PSampler _LinearSampler; //???can define in effect?

public:

	CDEMShaderWrapper(CDEMRenderer& Owner, Render::CEffect& Effect);
	virtual ~CDEMShaderWrapper();

	void         setInputSet(BlendMode BlendMode, bool Clipped, bool Opaque);
	virtual void prepareForRendering(const ShaderParameterBindings* shaderParameterBindings) override;
};

}
