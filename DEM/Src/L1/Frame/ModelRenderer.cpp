#include "ModelRenderer.h"

#include <Frame/RenderNode.h>
#include <Frame/Model.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CModelRenderer, 'MDLR', Core::CRTTIBaseClass); //Frame::IRenderer);

CArray<CRenderNode>::CIterator CModelRenderer::Render(CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr)
{
	CArray<CRenderNode>::CIterator ItEnd = RenderQueue.End();
	while (ItCurr != ItEnd)
	{
		if (ItCurr->pRenderer != this) return ItCurr;

		n_assert_dbg(ItCurr->RenderObject->IsA<Frame::CModel>());

		// Render CModel
		// Add skinning
		// Add instancing

		++ItCurr;
	};

	return ItEnd;
}
//---------------------------------------------------------------------

}