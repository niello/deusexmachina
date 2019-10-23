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
	if (!DifferentLightCounts) return nullptr;

	UPTR Idx = LightCount;
	if (Idx >= PassesByLightCount.GetCount()) Idx = PassesByLightCount.GetCount() - 1;

	// If the exact light count requested is not supported, find any supported count less than that
	while (Idx > 0 && !PassesByLightCount[Idx].GetCount()) --Idx;

	if (!PassesByLightCount[Idx].GetCount()) return nullptr;

	LightCount = Idx;
	return &PassesByLightCount[Idx];
}
//---------------------------------------------------------------------

}
