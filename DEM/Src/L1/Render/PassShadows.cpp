#include "PassShadows.h"

namespace Render
{

void CPassShadows::Render(const CArray<CRenderObject*>* pObjects, const CArray<CLight*>* pLights)
{
	// Shadow pass:
	// - begin pass
	// - select shadow-casting light(s)
	// - instead of collecting meshes visible from the light's camera, we collect ones in the light range
	// - for directional lights and PSSM this process will differ, may be extruded shadow boxes will be necessary
	// - collect ONLY shadow casters
	// - do not modify the main mesh list
	// - note: some caster meshes invisible from the main camera can have visible shadows. Anyway they are
	//         casting shadows on all visible receivers as their shadows are rendered to shadow buffer texture
	// - render casters to the shadow buffer texture
	// - end pass
}
//---------------------------------------------------------------------

}