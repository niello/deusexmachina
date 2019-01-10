#include "Technique.h"
#include <Render/RenderState.h>
#include <Render/ShaderConstant.h>

namespace Render
{
CTechnique::CTechnique() {}
CTechnique::~CTechnique() {}

// NB: Always reduces LightCount if exact requested value can't be used
const CPassList* CTechnique::GetPasses(UPTR& LightCount) const
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

const CEffectConstant* CTechnique::GetConstant(CStrID Name) const
{
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		const CEffectConstant& CurrConst = Consts[i];
		if (CurrConst.ID == Name) return &CurrConst;
	}
	return NULL;
}
//---------------------------------------------------------------------

const CEffectResource* CTechnique::GetResource(CStrID Name) const
{
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		const CEffectResource& CurrConst = Resources[i];
		if (CurrConst.ID == Name) return &CurrConst;
	}
	return NULL;
}
//---------------------------------------------------------------------

const CEffectSampler* CTechnique::GetSampler(CStrID Name) const
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
