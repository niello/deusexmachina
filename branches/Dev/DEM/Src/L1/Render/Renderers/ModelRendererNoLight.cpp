#include "ModelRendererNoLight.h"

namespace Render
{
ImplementRTTI(Render::CModelRendererNoLight, Render::IModelRenderer);
ImplementFactory(Render::CModelRendererNoLight);

void CModelRendererNoLight::Render()
{
	//???sort here or in add?

	// -loop-
		// SetTech
		// Begin
		// BeginPass
			// -loop-
			// DIP (instanced if possible)
			// [Set new mtl params, commit changes, if they were]
		// -end loop-
		// EndPass
		// End
	// -end loop-

	for (int i = 0; i < Models.Size(); ++i)
	{
	}

	Models.Clear();
}
//---------------------------------------------------------------------

}