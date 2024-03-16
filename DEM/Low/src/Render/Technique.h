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

	const CEffect*            _pOwner = nullptr;
	CStrID                    _Name;
	std::vector<PRenderState> _Passes;
	IPTR                      _MaxLightCount;
	PShaderParamTable         _Params;
	U8                        _SortingKey = 0;

	friend class CEffect;

public:

	CTechnique(CStrID Name, U8 SortingKey, std::vector<PRenderState>&& Passes, IPTR MaxLights, PShaderParamTable Params);
	virtual ~CTechnique() override;

	const CEffect*     GetEffect() const { return _pOwner; }
	CStrID             GetName() const { return _Name; }
	const auto&        GetPasses() const { return _Passes; }
	IPTR               GetMaxLightCount() const { return _MaxLightCount; }
	CShaderParamTable& GetParamTable() const { return *_Params; }
	U8                 GetSortingKey() const { return _SortingKey; }
};

typedef Ptr<CTechnique> PTechnique;

}
