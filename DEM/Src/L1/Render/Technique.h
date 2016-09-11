#pragma once
#ifndef __DEM_L1_RENDER_TECHNIQUE_H__
#define __DEM_L1_RENDER_TECHNIQUE_H__

#include <Render/RenderFwd.h>
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

	//!!!need binary search by ID!
	CFixedArray<CEffectConstant>	Consts;
	CFixedArray<CEffectResource>	Resources;
	CFixedArray<CEffectSampler>		Samplers;

	friend class Resources::CEffectLoader;

public:

	CTechnique() {}

	CStrID					GetName() const { return Name; }
	UPTR					GetShaderInputSetID() const { return ShaderInputSetID; }
	IPTR					GetMaxLightCount() const { return PassesByLightCount.GetCount() - 1; }
	const CPassList*		GetPasses(UPTR& LightCount) const;
	const CEffectConstant*	GetConstant(CStrID Name) const;
	const CEffectResource*	GetResource(CStrID Name) const;
	const CEffectSampler*	GetSampler(CStrID Name) const;
};

typedef Ptr<CTechnique> PTechnique;

// NB: Always reduces LightCount if exact requested value can't be used
inline const CPassList*	CTechnique::GetPasses(UPTR& LightCount) const
{
	UPTR DifferentLightCounts = PassesByLightCount.GetCount();
	if (!DifferentLightCounts) return NULL;

	UPTR Idx = LightCount;
	if (Idx >= PassesByLightCount.GetCount()) Idx = PassesByLightCount.GetCount() - 1;

	// If the exact light count requested is not supported, find any supported count less than that
	while (Idx > 0 && !PassesByLightCount[Idx].GetCount()) --Idx;

	if (!PassesByLightCount[Idx].GetCount()) return NULL;

	LightCount = Idx;
	return &PassesByLightCount[Idx];
}
//---------------------------------------------------------------------

inline const CEffectConstant* CTechnique::GetConstant(CStrID Name) const
{
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		const CEffectConstant& CurrConst = Consts[i];
		if (CurrConst.ID == Name) return &CurrConst;
	}
	return NULL;
}
//---------------------------------------------------------------------

inline const CEffectResource* CTechnique::GetResource(CStrID Name) const
{
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		const CEffectResource& CurrConst = Resources[i];
		if (CurrConst.ID == Name) return &CurrConst;
	}
	return NULL;
}
//---------------------------------------------------------------------

inline const CEffectSampler* CTechnique::GetSampler(CStrID Name) const
{
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		const CEffectSampler& CurrConst = Samplers[i];
		if (CurrConst.ID == Name) return &CurrConst;
	}
	return NULL;
}
//---------------------------------------------------------------------

}

#endif
