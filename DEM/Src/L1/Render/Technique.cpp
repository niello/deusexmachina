#include "Technique.h"

namespace Render
{

//???!!!move to a tool, add checks, exclude material constants?!
void CTechnique::BuildConstantTable()
{
	if (pConstantTable)
	{
		n_free(pConstantTable);
		pConstantTable = NULL;
		ConstNameToHandles.Clear();
		ShaderTypeCount = 0;
	}

	// For lit techs, try to select variation with lights, as it may use
	// constants variation without lights doesn't use.
	UPTR LightCount = 1;
	const CPassList* pPasses = GetPasses(LightCount);
	if (!pPasses) return;

	//
}
//---------------------------------------------------------------------

}