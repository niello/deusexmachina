//#include "D3D9Shader.h"
//
//#include <Render/RenderServer.h>
//#include <Events/EventServer.h>
//#include <Core/Factory.h>
//
//#ifdef DEM_USE_D3DX9
//#define WIN32_LEAN_AND_MEAN
//#define D3D_DISABLE_9EX
//#include <d3dx9.h>
//#endif
//
//namespace Render
//{
//__ImplementResourceClass(Render::CShader, 'SHDR', Resources::CResource);
//
//bool CShader::Setup(ID3DXEffect* pFX)
//{
//	if (!pFX) FAIL;
//
//	pEffect = pFX;
//
//	D3DXEFFECT_DESC Desc = { 0 };    
//	n_assert(SUCCEEDED(pEffect->GetDesc(&Desc)));
//
//	n_assert(Desc.Techniques > 0);
//	FlagsToTech.BeginAdd();
//	for (UINT i = 0; i < Desc.Techniques; ++i)
//	{
//		D3DXHANDLE hTech = pEffect->GetTechnique(i);
//		D3DXTECHNIQUE_DESC TechDesc;
//		n_assert(SUCCEEDED(pEffect->GetTechniqueDesc(hTech, &TechDesc)));
//		//NameToTech.Add(CStrID(TechDesc.Name), hTech);
//
//		D3DXHANDLE hFeatureAnnotation = pEffect->GetAnnotationByName(hTech, "Mask");
//		if (hFeatureAnnotation)
//		{
// CShader::LPCSTR pFeatMask = NULL;
// CShader::n_assert(SUCCEEDED(pEffect->GetString(hFeatureAnnotation, &pFeatMask)));
//
// CShader::Data::CStringTokenizer StrTok(pFeatMask, ",");
// CShader::while (StrTok.GetNextTokenSingleChar())
// CShader::	FlagsToTech.Add(RenderSrv->ShaderFeatures.GetMask(StrTok.GetCurrToken()), hTech);
//
// CShader::n_assert_dbg(FlagsToTech.GetCount());
//		}
//		else Sys::Log("WARNING: No feature mask annotation in technique '%s'!\n", TechDesc.Name);
//	}
//	FlagsToTech.EndAdd();
//
//	// It is good for pass and batch shaders with only one tech
//	// It seems, pass shaders use the only tech without setting it
//	//n_assert(SetTech(FlagsToTech.ValueAt(0)));
//
//	for (UINT i = 0; i < Desc.Parameters; ++i)
//	{
//		HVar hVar = pEffect->GetParameter(NULL, i);
//		D3DXPARAMETER_DESC ParamDesc = { 0 };
//		n_assert(SUCCEEDED(pEffect->GetParameterDesc(hVar, &ParamDesc)));
//		NameToHVar.Add(CStrID(ParamDesc.Name), hVar);
//		if (ParamDesc.Semantic) SemanticToHVar.Add(CStrID(ParamDesc.Semantic), hVar);
//		// Can also check type, if needed
//	}
//
//	SUBSCRIBE_PEVENT(OnRenderDeviceLost, CShader, OnDeviceLost);
//	SUBSCRIBE_PEVENT(OnRenderDeviceReset, CShader, OnDeviceReset);
//
//	State = Resources::Rsrc_Loaded;
//	OK;
//}
////---------------------------------------------------------------------
//
//void CShader::Unload()
//{
//	UNSUBSCRIBE_EVENT(OnRenderDeviceLost);
//	UNSUBSCRIBE_EVENT(OnRenderDeviceReset);
//
//	hCurrTech = NULL;
//	FlagsToTech.Clear();
//	NameToHVar.Clear();
//	SemanticToHVar.Clear();
//	SAFE_RELEASE(pEffect);
//	State = Resources::Rsrc_NotLoaded;
//}
////---------------------------------------------------------------------
//
//bool CShader::OnDeviceLost(const Events::CEventBase& Ev)
//{
//	pEffect->OnLostDevice();
//	OK;
//}
////---------------------------------------------------------------------
//
//bool CShader::OnDeviceReset(const Events::CEventBase& Ev)
//{
//	pEffect->OnResetDevice();
//	OK;
//}
////---------------------------------------------------------------------
//
//DWORD CShader::Begin(bool SaveState)
//{
//	UINT Passes;
//	DWORD Flags = SaveState ? 0 : D3DXFX_DONOTSAVESTATE | D3DXFX_DONOTSAVESAMPLERSTATE | D3DXFX_DONOTSAVESHADERSTATE;
//	n_assert(SUCCEEDED(pEffect->Begin(&Passes, Flags)));
//	return Passes;
//}
////---------------------------------------------------------------------
//
//bool CShader::SetTech(CShader::HTech hTech)
//{
//	if (hTech != hCurrTech)
//	{
//		hCurrTech = hTech;
//		n_assert(SUCCEEDED(pEffect->SetTechnique(hCurrTech)));
//	}
//	return !!hCurrTech;
//}
////---------------------------------------------------------------------
//
//void CShader::SetBool(HVar Var, bool Value) { n_assert(SUCCEEDED(pEffect->SetBool(Var, Value))); }
//void CShader::SetInt(HVar Var, int Value) { n_assert(SUCCEEDED(pEffect->SetInt(Var, Value))); }
//void CShader::SetIntArray(HVar Var, const int* pArray, DWORD Count) { n_assert(SUCCEEDED(pEffect->SetIntArray(Var, pArray, Count))); }
//void CShader::SetFloat(HVar Var, float Value) { n_assert(SUCCEEDED(pEffect->SetFloat(Var, Value))); }
//void CShader::SetFloatArray(HVar Var, const float* pArray, DWORD Count) { n_assert(SUCCEEDED(pEffect->SetFloatArray(Var, pArray, Count))); }
//void CShader::SetFloat4(HVar Var, const vector4& Value) { n_assert(SUCCEEDED(pEffect->SetVector(Var, (CONST D3DXVECTOR4*)&Value))); }
//void CShader::SetFloat4Array(HVar Var, const vector4* pArray, DWORD Count) { n_assert(SUCCEEDED(pEffect->SetVectorArray(Var, (CONST D3DXVECTOR4*)pArray, Count))); }
//void CShader::SetMatrix(HVar Var, const matrix44& Value) { n_assert(SUCCEEDED(pEffect->SetMatrix(Var, (CONST D3DXMATRIX*)&Value))); }
//void CShader::SetMatrixArray(HVar Var, const matrix44* pArray, DWORD Count) { n_assert(SUCCEEDED(pEffect->SetMatrixArray(Var, (CONST D3DXMATRIX*)pArray, Count))); }
//void CShader::SetMatrixPointerArray(HVar Var, const matrix44** pArray, DWORD Count) { n_assert(SUCCEEDED(pEffect->SetMatrixPointerArray(Var, (CONST D3DXMATRIX**)pArray, Count))); }
//void CShader::SetTexture(HVar Var, const CTexture& Value) { n_assert(SUCCEEDED(pEffect->SetTexture(Var, Value.GetD3D9BaseTexture()))); }
////pEffect->SetRawValue
//
//void CShader::BeginPass(DWORD PassIdx) { n_assert(SUCCEEDED(pEffect->BeginPass(PassIdx))); }
//void CShader::CommitChanges() { n_assert(SUCCEEDED(pEffect->CommitChanges())); } // For changes inside a pass
//void CShader::EndPass() { n_assert(SUCCEEDED(pEffect->EndPass())); }
//void CShader::End() { n_assert(SUCCEEDED(pEffect->End())); }
//
//bool CShader::IsVarUsed(HVar hVar) const { return hCurrTech && pEffect->IsParameterUsed(hVar, hCurrTech); } //!!!can use lookup 2D table to eliminate virtual call and hidden complexity!
//
//}