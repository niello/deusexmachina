#include "ModelRendererNoLight.h"

namespace Render
{
ImplementRTTI(Render::CModelRendererNoLight, Render::IModelRenderer);
ImplementFactory(Render::CModelRendererNoLight);

void CModelRendererNoLight::Render()
{
	//!!!if requires sorting by distance, sort all objects once!
	//mb set in FrameShader flag "sort by distance to camera", if at least one batch requires it?
	//check is it better than sorting only subsets collected by renderers, but many times

	// sort collected models by shader, by shader features, by material

	for (int i = 0; i < Models.Size(); ++i)
	{
	}

	Models.Clear();
	//pLights = NULL; // In lighting renderers
}
//---------------------------------------------------------------------

}