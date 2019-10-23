#pragma once
#include <Render/RenderFwd.h>
#include <Render/ShaderParamTable.h>
#include <Data/FixedArray.h>
#include <Data/StringID.h>
#include <Data/Dictionary.h>
#include <Data/RefCounted.h>

// A particular implementation of an effect for a specific input data. It includes
// geometry specifics (static/skinned), environment (light count, fog) and may be
// some others. A combination of input states is represented as a shader input set.
// When IRenderer renders some object, it chooses a technique whose input set ID
// matches an ID of input set the renderer provides.

namespace Resources
{
	class CEffectLoader;
}

namespace Render
{
typedef CFixedArray<PRenderState> CPassList;

class CTechnique: public Data::CRefCounted
{
private:

	CStrID							Name;
	UPTR							ShaderInputSetID;
	//EGPUFeatureLevel				MinFeatureLevel;
	CFixedArray<CPassList>			PassesByLightCount; // Light count is an index

	CShaderParamTable Params;

	friend class CEffect;

public:

	CTechnique();
	virtual ~CTechnique() override;

	CStrID           GetName() const { return Name; }
	UPTR             GetShaderInputSetID() const { return ShaderInputSetID; }
	IPTR             GetMaxLightCount() const { return PassesByLightCount.GetCount() - 1; }
	const CPassList* GetPasses(UPTR& LightCount) const;
	const auto&      GetParamTable() const { return Params; }
};

typedef Ptr<CTechnique> PTechnique;

}
