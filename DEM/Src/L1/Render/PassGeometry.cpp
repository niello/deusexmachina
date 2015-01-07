#include "PassGeometry.h"

#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Core/Factory.h>

namespace Render
{

bool CPassGeometry::Init(CStrID PassName, const Data::CParams& Desc, const CDict<CStrID, PRenderTarget>& RenderTargets)
{
	if (!CRenderPhase::Init(PassName, Desc, RenderTargets)) FAIL;

	Data::CDataArray& Batches = *Desc.Get<Data::PDataArray>(CStrID("Batches"));
	for (int i = 0; i < Batches.GetCount(); ++i)
	{
		Data::CParams& BatchDesc = *(Data::PParams)Batches[i];
		PRenderer& Renderer = BatchRenderers.At(i);
		Renderer = (IRenderer*)Factory->Create(BatchDesc.Get<CString>(CStrID("Renderer")));
		if (!Renderer->Init(BatchDesc)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

void CPassGeometry::Render(const CArray<CRenderObject*>* pObjects, const CArray<CLight*>* pLights)
{
	if (Shader.IsValid())
	{
		for (int i = 0; i < ShaderVars.GetCount(); ++i)
			ShaderVars.ValueAt(i).Apply(*Shader.GetUnsafe());
		n_assert(Shader->Begin(true) == 1); //!!!PERF: saves state!
		Shader->BeginPass(0);
	}

	for (int i = 0; i < CRenderServer::MaxRenderTargetCount; ++i)
		//if (RT[i].IsValid()) // Now sets NULL RTs too to clear unused RTs from prev. passes. See alternative below.
			RenderSrv->SetRenderTarget(i, RT[i].GetUnsafe());

	RenderSrv->Clear(ClearFlags, ClearColor, ClearDepth, ClearStencil);

	// N3: set pixel size and half pixel size shared shader vars //???why not committed in N3?
	//!!!soft particles use it to sample depth!

	for (int i = 0; i < BatchRenderers.GetCount(); ++i)
	{
		IRenderer* pRenderer = BatchRenderers[i];
		if (pLights) pRenderer->AddLights(*pLights);
		if (pObjects) pRenderer->AddRenderObjects(*pObjects);
		pRenderer->Render();
	}

	for (int i = 0; i < CRenderServer::MaxRenderTargetCount; ++i)
		if (RT[i].IsValid()) //???break on first invalid? RTs must be set in order
		{
			RT[i]->Resolve();
			// N3: if (i > 0) RenderSrv->SetRenderTarget(i, NULL);
		}

	if (Shader.IsValid())
	{
		Shader->EndPass();
		Shader->End();
	}
}
//---------------------------------------------------------------------

}
