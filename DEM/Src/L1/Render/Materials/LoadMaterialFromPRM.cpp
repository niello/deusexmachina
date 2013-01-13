// Loads material desc from .prm file
// Use function declaration instead of header file where you want to call this loader.

//!!!NEED BINARY .MTL LOADER! Write .mtl by data scheme.

#include <Render/RenderServer.h>
#include <Data/DataServer.h>
#include <Data/Params.h>

namespace Render
{

bool LoadMaterialFromPRM(Data::CParams& In, PMaterial OutMaterial)
{
	if (!OutMaterial.isvalid()) FAIL;

	CStrID ShaderID = In.Get<CStrID>(CStrID("Shader"));
	PShader Shader = RenderSrv->ShaderMgr.GetTypedResource(ShaderID);
	DWORD FeatFlags = RenderSrv->ShaderFeatureStringToMask(In.Get<nString>(CStrID("FeatureFlags")));

	CShaderVarMap VarMap;
	VarMap.BeginAdd();

	Data::CParam* pPrmVars;
	if (In.Get(pPrmVars, CStrID("ShaderVars")))
	{
		Data::CParams& Vars = *pPrmVars->GetValue<Data::PParams>();
		for (int i = 0; i < Vars.GetCount(); ++i)
		{
			Data::CParam& PrmVar = Vars.Get(i);
			CShaderVar& Var = VarMap.Add(PrmVar.GetName());
			Var.SetName(PrmVar.GetName());
			Var.Value = PrmVar.GetRawValue();
		}
	}

	if (In.Get(pPrmVars, CStrID("Textures")))
	{
		Data::CParams& Vars = *pPrmVars->GetValue<Data::PParams>();
		for (int i = 0; i < Vars.GetCount(); ++i)
		{
			Data::CParam& PrmVar = Vars.Get(i);
			CShaderVar& Var = VarMap.Add(PrmVar.GetName());
			Var.SetName(PrmVar.GetName());
			Var.Value = RenderSrv->TextureMgr.GetTypedResource(CStrID(PrmVar.GetValue<nString>().Get()));
		}
	}

	VarMap.EndAdd();

	return OutMaterial->Setup(Shader, FeatFlags, VarMap);
}
//---------------------------------------------------------------------

bool LoadMaterialFromPRM(const nString& FileName, PMaterial OutMaterial)
{
	Data::PParams Desc = DataSrv->LoadPRM(FileName, false);
	return Desc.isvalid() && LoadMaterialFromPRM(*Desc, OutMaterial);
}
//---------------------------------------------------------------------

}