#include "Pass.h"

#include <Render/FrameShader.h> //!!!TMP for Validate!
#include <Data/Params.h>
#include <Data/DataArray.h>

namespace Render
{

bool CPass::Init(CStrID PassName, const Data::CParams& Desc, const nDictionary<CStrID, PRenderTarget>& RenderTargets)
{
	Name = PassName;

	CStrID ShaderID = Desc.Get(CStrID("Shader"), CStrID::Empty);
	if (ShaderID.IsValid()) Shader = RenderSrv->ShaderMgr.GetTypedResource(ShaderID);

	ClearFlags = 0;

	Data::CParam* pPrm;
	if (Desc.Get(pPrm, CStrID("ClearColor")))
	{
		ClearFlags |= Clear_Depth;
		if (pPrm->IsA<int>()) ClearColor = pPrm->GetValue<int>();
		else if (pPrm->IsA<vector4>())
		{
			const vector4& Color = pPrm->GetValue<vector4>();
			ClearColor = N_COLORVALUE(Color.x, Color.y, Color.z, Color.w);
		}
		else n_error("CPass::Init() -> Invalid type of ClearColor");
	}

	if (Desc.Get(ClearDepth, CStrID("ClearDepth"))) ClearFlags |= Clear_Depth;

	int StencilVal;
	if (Desc.Get(StencilVal, CStrID("ClearStencil")))
	{
		ClearStencil = (uchar)StencilVal;
		ClearFlags |= Clear_Stencil;
	}

//!!!ShaderVars!

	Data::PDataArray RTNames;
	if (Desc.Get<Data::PDataArray>(RTNames, CStrID("RenderTargets")))
		for (int i = 0; i < RTNames->Size() && i < CRenderServer::MaxRenderTargetCount; ++i)
			RT[i] = RenderTargets[RTNames->At(i).GetValue<CStrID>()];

	OK;
}
//---------------------------------------------------------------------

//!!!OLD!
void CPass::Validate()
{
	n_assert(pFrameShader);

	// find shader
	if (rpShaderIndex == -1 && shaderAlias.IsValid())
	{
		rpShaderIndex = pFrameShader->FindShaderIndex(shaderAlias);
		if (rpShaderIndex == -1)
			n_error("nRpPass::Validate(): couldn't find shader alias '%s' in render path xml file!", shaderAlias.Get());
	}
}
//---------------------------------------------------------------------

}
