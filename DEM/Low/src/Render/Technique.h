#pragma once
#include <Render/RenderFwd.h>
#include <Render/ShaderParamTable.h>
#include <Data/StringID.h>
#include <Data/RefCounted.h>

// A particular implementation of an effect for a specific input data. It includes
// geometry specifics (static/skinned), environment (light count, fog) and may be
// some others. A combination of input states is represented as a shader input set.
// When IRenderer renders some object, it chooses a technique whose input set ID
// matches an ID of input set the renderer provides.

namespace Render
{

class CTechnique: public Data::CRefCounted
{
private:

	CStrID                    _Name;
	std::vector<PRenderState> _Passes;
	IPTR                      _MaxLightCount;
	PShaderParamTable         _Params;

	friend class CEffect;

public:

	CTechnique(CStrID Name, std::vector<PRenderState>&& Passes, IPTR MaxLights, PShaderParamTable Params);
	virtual ~CTechnique() override;

	CStrID             GetName() const { return _Name; }
	const auto&        GetPasses(UPTR& LightCount) const { return _Passes; }
	const auto&        GetPasses() const { return _Passes; }
	IPTR               GetMaxLightCount() const { return _MaxLightCount; }
	CShaderParamTable& GetParamTable() const { return *_Params; }
};

typedef Ptr<CTechnique> PTechnique;

}
