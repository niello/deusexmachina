#include "ModelRendererSinglePassLight.h"

namespace Render
{
ImplementRTTI(Render::CModelRendererSinglePassLight, Render::IModelRenderer);
ImplementFactory(Render::CModelRendererSinglePassLight);

void CModelRendererSinglePassLight::Render()
{
	//

	Models.Clear();
	pLights = NULL;
}
//---------------------------------------------------------------------

}