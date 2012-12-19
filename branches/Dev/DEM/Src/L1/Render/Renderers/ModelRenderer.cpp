#include "ModelRenderer.h"

#include <Scene/Model.h>

namespace Render
{
ImplementRTTI(Render::IModelRenderer, Render::IRenderer);

void IModelRenderer::AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects)
{
	for (int i = 0; i < Objects.Size(); ++i)
	{
		if (!Objects[i]->IsA(Scene::CModel::RTTI)) continue;

		// Filter by renderer settings (alpha only etc)
	}
}
//---------------------------------------------------------------------

// NB: It is overriden to empty method in CModelRendererNoLight
void IModelRenderer::AddLights(const nArray<Scene::CLight*>& Lights)
{
	for (int i = 0; i < Lights.Size(); ++i)
	{
		// Perform something with lights or just store array ref
	}
}
//---------------------------------------------------------------------

}