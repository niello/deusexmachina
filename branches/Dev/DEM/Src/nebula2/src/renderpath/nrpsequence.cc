//------------------------------------------------------------------------------
//  nrpsequence.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "renderpath/nrpsequence.h"
#include "gfx2/ngfxserver2.h"
#include <Render/FrameShader.h>

//------------------------------------------------------------------------------
/**
*/
nRpSequence::nRpSequence() :
    pFrameShader(0),
    rpShaderIndex(-1),
    firstLightAlphaEnabled(false),
    shaderUpdatesEnabled(true),
    mvpOnly(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
int
nRpSequence::GetShaderBucketIndex() const
{
    n_assert(-1 != this->rpShaderIndex);
    n_assert(this->pFrameShader);
    return pFrameShader->shaders[this->rpShaderIndex].GetBucketIndex();
}

//------------------------------------------------------------------------------
/**
*/
void
nRpSequence::Validate()
{
    n_assert(this->pFrameShader);

    if (-1 == this->rpShaderIndex)
    {
        n_assert(!this->shaderAlias.IsEmpty());
        this->rpShaderIndex = this->pFrameShader->FindShaderIndex(this->shaderAlias);
        if (-1 == this->rpShaderIndex)
        {
            n_error("nRpSequence::Validate(): couldn't find shader alias '%s' in render path xml file!", this->shaderAlias.Get());
        }
    }
}

//------------------------------------------------------------------------------
/**
    NOTE: sequence shaders generally do NOT backup the current state.
*/
int
nRpSequence::Begin()
{
    n_assert(-1 != this->rpShaderIndex && pFrameShader);

    nGfxServer2::Instance()->SetHint(nGfxServer2::MvpOnly, this->mvpOnly);
    nShader2* shd = this->pFrameShader->shaders[this->rpShaderIndex].GetShader();
    if (this->shaderUpdatesEnabled)
    {
		for (int varIndex = 0; varIndex < varContext.GetNumVariables(); varIndex++)
		{
			const nVariable& paramVar = this->varContext.GetVariableAt(varIndex);

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
				default: n_error("CPass: Invalid shader arg datatype!");
			}

			// update the shader parameter
			this->shaderParams.SetArg(shaderParam, shaderArg);
		}
        shd->SetParams(this->shaderParams);
    }
    if (!technique.IsEmpty()) shd->SetTechnique(this->technique.Get());
    return shd->Begin(false);
}

//------------------------------------------------------------------------------
/**
*/
void
nRpSequence::BeginPass(int pass)
{
    n_assert(-1 != this->rpShaderIndex && pass >= 0 && pFrameShader);
    this->pFrameShader->shaders[this->rpShaderIndex].GetShader()->BeginPass(pass);
}

//------------------------------------------------------------------------------
/**
*/
void
nRpSequence::EndPass()
{
    n_assert(-1 != this->rpShaderIndex && pFrameShader);
    this->pFrameShader->shaders[this->rpShaderIndex].GetShader()->EndPass();
}

//------------------------------------------------------------------------------
/**
    NOTE: currently, the previous technique is not restored (should this
    even be the intended behavior???)
*/
void
nRpSequence::End()
{
    n_assert(-1 != rpShaderIndex && pFrameShader);
    this->pFrameShader->shaders[this->rpShaderIndex].GetShader()->End();
    nGfxServer2::Instance()->SetHint(nGfxServer2::MvpOnly, false);
}
