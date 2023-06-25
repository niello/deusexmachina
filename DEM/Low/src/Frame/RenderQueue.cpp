#include "RenderPhase.h"

//#include <Data/Params.h>
//#include <Data/DataArray.h>

namespace Frame
{

	/*
bool CRenderPhase::Init(const Data::CParams& Desc, const CRenderPath& Owner)
{
	Name = PassName;

	CStrID ShaderID = Desc.Get(CStrID("Shader"), CStrID::Empty);
	if (ShaderID.IsValid())
	{
		Shader = RenderSrv->ShaderMgr.GetTypedResource(ShaderID);
		if (!Shader->IsLoaded()) FAIL;
	}

	//!!!DUPLICATE CODE!+
	ShaderVars.BeginAdd();

	Data::CParam* pPrm;
	if (Desc.Get(pPrm, CStrID("ShaderVars")))
	{
		Data::CParams& Vars = *pPrm->GetValue<Data::PParams>();
		for (int i = 0; i < Vars.GetCount(); ++i)
		{
			Data::CParam& PrmVar = Vars.Get(i);
			CShaderVar& Var = ShaderVars.Add(PrmVar.GetName());
			Var.SetName(PrmVar.GetName());
			Var.Value = PrmVar.GetRawValue();
		}
	}

	if (Desc.Get(pPrm, CStrID("Textures")))
	{
		Data::CParams& Vars = *pPrm->GetValue<Data::PParams>();
		for (int i = 0; i < Vars.GetCount(); ++i)
		{
			Data::CParam& PrmVar = Vars.Get(i);
			CShaderVar& Var = ShaderVars.Add(PrmVar.GetName());
			Var.SetName(PrmVar.GetName());
			Var.Value = RenderSrv->TextureMgr.GetOrCreateTypedResource(CStrID(PrmVar.GetValue<CString>().CStr()));
		}
	}

	ShaderVars.EndAdd();
	//!!!DUPLICATE CODE!-

	Data::PDataArray RTNames;
	if (Desc.Get<Data::PDataArray>(RTNames, CStrID("RenderTargets")))
		for (int i = 0; i < RTNames->GetCount() && i < CRenderServer::MaxRenderTargetCount; ++i)
			RT[i] = RenderTargets[RTNames->At(i).GetValue<CStrID>()];

	ClearFlags = 0;

	if (Desc.Get(pPrm, CStrID("ClearColor")))
	{
		if (pPrm->IsA<int>()) ClearColor = pPrm->GetValue<int>();
		else if (pPrm->IsA<vector4>())
		{
			const vector4& Color = pPrm->GetValue<vector4>();
			ClearColor = N_COLORVALUE(Color.x, Color.y, Color.z, Color.w);
		}
		else Sys::Error("CRenderPhase::Init() -> Invalid type of ClearColor");
		ClearFlags |= Clear_Color;
	}

	if (Desc.Get(ClearDepth, CStrID("ClearDepth"))) ClearFlags |= Clear_Depth;

	int StencilVal;
	if (Desc.Get(StencilVal, CStrID("ClearStencil")))
	{
		ClearStencil = (U8)StencilVal;
		ClearFlags |= Clear_Stencil;
	}

	OK;

	FAIL;
}
//---------------------------------------------------------------------
	*/

}
