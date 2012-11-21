#include "PassGeometry.h"

#include <scene/nsceneserver.h>
#include <gfx2/ngfxserver2.h>
#include <Render/FrameShader.h> //!!!TMP for shader!

namespace Render
{

void CPassGeometry::Render()
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
    if (clearFlags != 0)
    {
        nGfxServer2::Instance()->Clear(clearFlags,
                         clearColor.x,
                         clearColor.y,
                         clearColor.z,
                         clearColor.w,
                         clearDepth,
                         clearStencil);
    }

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

	for (int phaseIndex = 0; phaseIndex < phases.Size(); phaseIndex++)
	{
		//!!!phase sorting curPhase.GetSortingOrder!
		nRpPhase& curPhase = phases[phaseIndex];
		if (curPhase.GetLightMode() == nRpPhase::Off)
			nSceneServer::Instance()->RenderPhaseLightModeOff(curPhase);
		else nSceneServer::Instance()->RenderPhaseLightModeShader(curPhase);
	}

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
void CPassGeometry::Validate()
{
	CPass::Validate();

	for (int i = 0; i < phases.Size(); ++i)
		phases[i].Validate();
}
//---------------------------------------------------------------------

}
