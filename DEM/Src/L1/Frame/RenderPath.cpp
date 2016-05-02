#include "RenderPath.h"

#include <Frame/RenderPhase.h>
#include <Frame/View.h>
#include <Render/GPUDriver.h>

namespace Frame
{
__ImplementClassNoFactory(Frame::CRenderPath, Resources::CResourceObject);

bool CRenderPath::Render(CView& View)
{
	//!!!clear all phases' render targets and DS surfaces at the beginning
	//of the frame, as recommended, especially for SLI, also it helps rendering
	//as pases must not clear RTs and therefore know are they the first to
	//render into or RT already contains this frame data
	//!!!DBG TMP!
	View.GPU->ClearRenderTarget(*View.RTs[0], vector4(0.1f, 0.7f, 0.1f, 1.f));

	for (UPTR i = 0; i < Phases.GetCount(); ++i)
	{
		if (!Phases[i]->Render(View))
		{
			//???clear tmp view data?
			FAIL;
		}
	}

	//???clear tmp view data?

	OK;
}
//---------------------------------------------------------------------

}