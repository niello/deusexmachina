#include "ModelRenderer.h"

#include <Render/RenderFwd.h>
#include <Render/RenderNode.h>
#include <Render/Model.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CModelRenderer, 'MDLR', Core::CRTTIBaseClass); //Render::IRenderer);

CModelRenderer::CModelRenderer()
{
	// Setup dynamic enumeration
	InputSet_Model = GetShaderInputSetID(CStrID("Model"));
	InputSet_ModelSkinned = GetShaderInputSetID(CStrID("ModelSkinned"));
	InputSet_ModelInstanced = GetShaderInputSetID(CStrID("ModelInstanced"));
}
//---------------------------------------------------------------------

CArray<CRenderNode>::CIterator CModelRenderer::Render(CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr)
{
	CArray<CRenderNode>::CIterator ItEnd = RenderQueue.End();
	while (ItCurr != ItEnd)
	{
		if (ItCurr->pRenderer != this) return ItCurr;

		n_assert_dbg(ItCurr->pRenderable->IsA<Render::CModel>());

		Sys::DbgOut("CModel rendered\n");

		// Find tech
		// If tech supports instancing, try to collect more objects
		// Render CModel
		// Add skinning
		// Add instancing

		++ItCurr;
	};

	return ItEnd;
}
//---------------------------------------------------------------------

}