#include "ModelRendererMultiPassLight.h"

namespace Render
{
ImplementRTTI(Render::CModelRendererMultiPassLight, Render::IModelRenderer);
ImplementFactory(Render::CModelRendererMultiPassLight);

void CModelRendererMultiPassLight::Render()
{
	//

	Models.Clear();
	pLights = NULL;
}
//---------------------------------------------------------------------

}