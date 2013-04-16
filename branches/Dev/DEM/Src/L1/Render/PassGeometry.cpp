#include "PassGeometry.h"

#include <Data/Params.h>
#include <Data/DataArray.h>

#include <gfx2/ngfxserver2.h>
#include <Render/FrameShader.h> //!!!TMP for shader!

namespace Render
{

bool CPassGeometry::Init(CStrID PassName, const Data::CParams& Desc, const nDictionary<CStrID, PRenderTarget>& RenderTargets)
{
	if (!CPass::Init(PassName, Desc, RenderTargets)) FAIL;

	Data::CDataArray& Batches = *Desc.Get<Data::PDataArray>(CStrID("Batches"));
	for (int i = 0; i < Batches.Size(); ++i)
	{
		Data::CParams& BatchDesc = *(Data::PParams)Batches[i];
		PRenderer& Renderer = BatchRenderers.At(i);
		Renderer = (IRenderer*)CoreFct->Create(BatchDesc.Get<nString>(CStrID("Renderer")));
		if (!Renderer->Init(BatchDesc)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

void CPassGeometry::Render(const nArray<Scene::CRenderObject*>* pObjects, const nArray<Scene::CLight*>* pLights)
{
	if (Shader.isvalid())
	{
		for (int i = 0; i < ShaderVars.Size(); ++i)
			ShaderVars.ValueAtIndex(i).Apply(*Shader.get_unsafe());
		n_assert(Shader->Begin(true) == 1); //!!!PERF: saves state!
		Shader->BeginPass(0);
	}

	for (int i = 0; i < CRenderServer::MaxRenderTargetCount; ++i)
		//if (RT[i].isvalid()) // Now sets NULL RTs too to clear unused RTs from prev. passes. See alternative below.
			RenderSrv->SetRenderTarget(i, RT[i].get_unsafe());

	RenderSrv->Clear(ClearFlags, ClearColor, ClearDepth, ClearStencil);

	// N3: set pixel size and half pixel size shared shader vars //???why not committed in N3?
	//!!!soft particles use it to sample depth!

	for (int i = 0; i < BatchRenderers.Size(); ++i)
	{
		IRenderer* pRenderer = BatchRenderers[i];
		if (pLights) pRenderer->AddLights(*pLights);
		if (pObjects) pRenderer->AddRenderObjects(*pObjects);
		pRenderer->Render();
	}

	for (int i = 0; i < CRenderServer::MaxRenderTargetCount; ++i)
		if (RT[i].isvalid()) //???break on first invalid? RTs must be set in order
		{
			RT[i]->Resolve();
			// N3: if (i > 0) RenderSrv->SetRenderTarget(i, NULL);
		}

	if (Shader.isvalid())
	{
		Shader->EndPass();
		Shader->End();
	}
}
//---------------------------------------------------------------------

}
