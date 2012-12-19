#include "PassPosteffect.h"

#include <Render/FrameShader.h> //!!!TMP for shader!
#include <gfx2/ngfxserver2.h>
#include <variable/nvariableserver.h>

namespace Render
{

CPassPosteffect::~CPassPosteffect()
{
	if (refQuadMesh.isvalid())
	{
		refQuadMesh->Release();
		refQuadMesh.invalidate();
	}
}
//---------------------------------------------------------------------

void CPassPosteffect::Render(const nArray<Scene::CRenderObject*>* pObjects, const nArray<Scene::CLight*>* pLights)
{
    // gfx stats enabled?
    nGfxServer2::Instance()->SetHint(nGfxServer2::CountStats, true); //statsEnabled);

    // set render targets
    for (int i = 0; i < renderTargetNames.Size(); i++)
    {
        // special case default render target
        if (renderTargetNames[i].IsEmpty())
        {
            if (i == 0) nGfxServer2::Instance()->SetRenderTarget(i, 0);
        }
        else
        {
            int renderTargetIndex = pFrameShader->FindRenderTargetIndex(renderTargetNames[i]);
            if (-1 == renderTargetIndex)
                n_error("nRpPass: invalid render target name: %s!", renderTargetNames[i].Get());
            nGfxServer2::Instance()->SetRenderTarget(i, pFrameShader->renderTargets[renderTargetIndex].GetTexture());
        }
    }

    // invoke begin scene
    n_assert(nGfxServer2::Instance()->BeginScene());

    // clear render target?
    nGfxServer2::Instance()->Clear(ClearFlags, ClearColor, ClearDepth, ClearStencil);

    // apply shader (note: save/restore all shader state for pass shaders!)
	if (rpShaderIndex != -1)
    {
		nShader2* shd = pFrameShader->shaders[rpShaderIndex].GetShader();

		for (int varIndex = 0; varIndex < varContext.GetNumVariables(); varIndex++)
		{
			const nVariable& paramVar = varContext.GetVariableAt(varIndex);

			// get shader state from variable
			nShaderState::Param shaderParam = (nShaderState::Param) paramVar.GetInt();

			// get the current value
			const nVariable* valueVar = nVariableServer::Instance()->GetGlobalVariable(paramVar.GetHandle());
			n_assert(valueVar);
			nShaderArg shaderArg;
			switch (valueVar->GetType())
			{
				case nVariable::Int: shaderArg.SetInt(valueVar->GetInt()); break;
				case nVariable::Float: shaderArg.SetFloat(valueVar->GetFloat()); break;
				case nVariable::Float4: shaderArg.SetFloat4(valueVar->GetFloat4()); break;
				case nVariable::Object: shaderArg.SetTexture((nTexture2*) valueVar->GetObj()); break;
				case nVariable::Matrix: shaderArg.SetMatrix44(&valueVar->GetMatrix()); break;
				case nVariable::Vector4: shaderArg.SetVector4(valueVar->GetVector4()); break;
				default: n_error("nRpPass: Invalid shader arg datatype!");
			}

			shaderParams.SetArg(shaderParam, shaderArg);
		}

		if (!technique.IsEmpty()) shd->SetTechnique(technique.Get());
        shd->SetParams(shaderParams);
        nGfxServer2::Instance()->SetShader(shd);
        int numShaderPasses = shd->Begin(true);
        n_assert(1 == numShaderPasses); // assume 1-pass for pass shaders!
        shd->BeginPass(0);
    }

	UpdateMeshCoords();

	// draw the quad
	nGfxServer2::Instance()->PushTransform(nGfxServer2::Model, matrix44::identity);
	nGfxServer2::Instance()->PushTransform(nGfxServer2::View, matrix44::identity);
	nGfxServer2::Instance()->PushTransform(nGfxServer2::Projection, matrix44::identity);
	nGfxServer2::Instance()->SetMesh(this->refQuadMesh, this->refQuadMesh);
	nGfxServer2::Instance()->SetVertexRange(0, 4);
	nGfxServer2::Instance()->SetIndexRange(0, 6);
	nGfxServer2::Instance()->DrawIndexedNS(nGfxServer2::TriangleList);
	nGfxServer2::Instance()->SetMesh(0, 0);  // FIXME FLOH: find out why this is necessary! if not done mesh data will be broken...
	nGfxServer2::Instance()->PopTransform(nGfxServer2::Projection);
	nGfxServer2::Instance()->PopTransform(nGfxServer2::View);
	nGfxServer2::Instance()->PopTransform(nGfxServer2::Model);

	if (rpShaderIndex != -1)
	{
		nShader2* shd = pFrameShader->shaders[rpShaderIndex].GetShader();
		shd->EndPass();
		shd->End();
	}

	nGfxServer2::Instance()->EndScene();

	// Disable used render targets
	if (!renderTargetNames[0].IsEmpty())
		for (int i = 0; i < renderTargetNames.Size(); i++)
			nGfxServer2::Instance()->SetRenderTarget(i, 0);
}
//---------------------------------------------------------------------

//!!!OLD!
void CPassPosteffect::Validate()
{
	CPass::Validate();

	if (!refQuadMesh.isvalid())
	{
		nMesh2* mesh = nGfxServer2::Instance()->NewMesh("_rpmesh");
		if (!mesh->IsLoaded()) mesh->CreateNew(4, 6, nMesh2::WriteOnly, nMesh2::Coord | nMesh2::Uv0);
		refQuadMesh = mesh;
	}
}
//---------------------------------------------------------------------

//!!!OLD!
/**
    Update the mesh coordinates. This takes several things into account:
    - 3d coordinates are created for direct mapping between texels and pixels
    - uv coordinates are take the render targets, or source texture's
      border color into account (hmm, tricky...)
*/
void CPassPosteffect::UpdateMeshCoords()
{
    // compute half pixel size for current render target
    nTexture2* renderTarget = nGfxServer2::Instance()->GetRenderTarget(0);
    int w, h;
    if (renderTarget)
    {
        w = renderTarget->GetWidth();
        h = renderTarget->GetHeight();
    }
    else
    {
        const CDisplayMode& mode = nGfxServer2::Instance()->GetDisplayMode();
        w = mode.Width;
        h = mode.Height;
    }
    vector2 pixelSize(1.0f / float(w), 1.0f / float(h));
    vector2 halfPixelSize = pixelSize * 0.5f;

    float x0 = -1.0f;
    float x1 = +1.0f - pixelSize.x;
    float y0 = -1.0f + pixelSize.y;
    float y1 = +1.0f;

    float u0 = 0.0f + halfPixelSize.x;
    float u1 = 1.0f - halfPixelSize.x;
    float v0 = 0.0f + halfPixelSize.y;
    float v1 = 1.0f - halfPixelSize.y;

    nMesh2* mesh = this->refQuadMesh;
    float* vPtr = mesh->LockVertices();
    n_assert(vPtr);
    *vPtr++ = x0; *vPtr++ = y1; *vPtr++ = 0.0f; *vPtr++ = u0; *vPtr++ = v0;
    *vPtr++ = x0; *vPtr++ = y0; *vPtr++ = 0.0f; *vPtr++ = u0; *vPtr++ = v1;
    *vPtr++ = x1; *vPtr++ = y1; *vPtr++ = 0.0f; *vPtr++ = u1; *vPtr++ = v0;
    *vPtr++ = x1; *vPtr++ = y0; *vPtr++ = 0.0f; *vPtr++ = u1; *vPtr++ = v1;
    mesh->UnlockVertices();

    ushort* iPtr = mesh->LockIndices();
    n_assert(iPtr);
    *iPtr++ = 0; *iPtr++ = 1; *iPtr++ = 2;
    *iPtr++ = 1; *iPtr++ = 3; *iPtr++ = 2;
    mesh->UnlockIndices();
}
//---------------------------------------------------------------------

}
