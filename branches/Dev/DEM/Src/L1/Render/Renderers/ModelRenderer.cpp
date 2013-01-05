#include "ModelRenderer.h"

#include <Scene/Model.h>

namespace Render
{
ImplementRTTI(Render::IModelRenderer, Render::IRenderer);

// Forward rendering:
// - Render solid objects to depth buffer, front to back (only if render to texture?)
// - Render atest objects to depth buffer, front to back (only if render to texture?)
// - Occlusion (against z-buffer filled by 1 and 2)
// - Render sky without zwrite and mb without ztest //???better to render sky after all other non-alpha/additive geometry?
// - Render terrain (lightmapped/unlit/...?) FTB //???render after all opaque except skybox?
// - Render opaque geometry (static, skinned, blended, envmapped) FTB
// - Render alpha-tested geometry (static, leaf, tree) FTB
// - Render alpha-blended geometry (alpha, alpha_soft, skinned_alpha, env_alpha, water) BTF
// - Render particles (alpha, then additive) BTF?
// - HDR

//!!!rendering front-to-back with existing z-buffer has no point!
//z-pass FtB has meaning, if pixel shader is not empty!

void IModelRenderer::AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects)
{
	for (int i = 0; i < Objects.Size(); ++i)
	{
		if (!Objects[i]->IsA(Scene::CModel::RTTI)) continue;
		Scene::CModel* pModel = (Scene::CModel*)Objects[i];
		if (!(pModel->Material->GetBatchType() & AllowedBatchTypes)) continue;

		// Find desired tech:
		// Get object feature flags (material + geometry)
		// If renderer has optional flags, remove them in object's feature flags
		// Else if tech has optional flags, it handles them in shader dictionary (all possible combinations)

		DWORD FeatFlags = pModel->FeatureFlags | pModel->Material->GetFeatureFlags();

		// Model renderer will add render mode flags itself (forex Depth)

		// Sort tech, then material

		Models.Append(pModel);
	}
}
//---------------------------------------------------------------------

// NB: It is overriden to empty method in CModelRendererNoLight
void IModelRenderer::AddLights(const nArray<Scene::CLight*>& Lights)
{
	pLights = &Lights;
	//for (int i = 0; i < Lights.Size(); ++i)
	//{
	//	// Perform something with lights or just store array ref
	//}
}
//---------------------------------------------------------------------

}