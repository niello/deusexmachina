//// Loads material desc from .prm file
//// Use function declaration instead of header file where you want to call this loader.
//
////!!!NEED BINARY .MTL LOADER! Write .mtl by data scheme.
//
//#include <Render/RenderServer.h>
//#include <Data/DataServer.h>
//#include <Data/Params.h>
//
//namespace Render
//{
//
//bool LoadMaterialFromPRM(Data::CParams& In, PMaterial OutMaterial)
//{
//	if (!OutMaterial.IsValid()) FAIL;
//
//	CStrID ShaderID = In.Get<CStrID>(CStrID("Shader"));
//	PShader Shader = RenderSrv->ShaderMgr.GetOrCreateTypedResource(ShaderID);
//	DWORD FeatFlags = RenderSrv->ShaderFeatures.GetMask(In.Get<CString>(CStrID("FeatureFlags")));
//
//	CShaderVarMap VarMap;
//	VarMap.BeginAdd();
//
//	Data::CParam* pPrmVars;
//	if (In.Get(pPrmVars, CStrID("ShaderVars")))
//	{
//		Data::CParams& Vars = *pPrmVars->GetValue<Data::PParams>();
//		for (int i = 0; i < Vars.GetCount(); ++i)
//		{
//			Data::CParam& PrmVar = Vars.Get(i);
//			CShaderVar& Var = VarMap.Add(PrmVar.GetName());
//			Var.SetName(PrmVar.GetName());
//			Var.Value = PrmVar.GetRawValue();
//		}
//	}
//
//	if (In.Get(pPrmVars, CStrID("Textures")))
//	{
//		Data::CParams& Vars = *pPrmVars->GetValue<Data::PParams>();
//		for (int i = 0; i < Vars.GetCount(); ++i)
//		{
//			Data::CParam& PrmVar = Vars.Get(i);
//			CShaderVar& Var = VarMap.Add(PrmVar.GetName());
//			Var.SetName(PrmVar.GetName());
//			Var.Value = RenderSrv->TextureMgr.GetOrCreateTypedResource(CStrID(PrmVar.GetValue<CStrID>().CStr()));
//		}
//	}
//
//	VarMap.EndAdd();
//
//	return OutMaterial->Setup(Shader, FeatFlags, VarMap);
//}
////---------------------------------------------------------------------
//
//bool LoadMaterialFromPRM(const CString& FileName, PMaterial OutMaterial)
//{
//	Data::PParams Desc = DataSrv->LoadPRM(FileName, false);
//	return Desc.IsValid() && LoadMaterialFromPRM(*Desc, OutMaterial);
//}
////---------------------------------------------------------------------
//
//}