#pragma once
#ifndef __DEM_L1_RENDER_SHADER_H__
#define __DEM_L1_RENDER_SHADER_H__

#include <Render/Materials/Texture.h>
#include <util/ndictionary.h>
#define WIN32_LEAN_AND_MEAN
#include <d3dx9.h>

// Encapsulates graphics hardware shader (in different variations) and associated variable mapping

namespace Data
{
	class CStream;
}

namespace Render
{

class CShader: public Resources::CResource
{
	//DeclareRTTI;

public:

	typedef D3DXHANDLE HVar;
	//enum { InvalidVar = 0xffffffff };

protected:

	ID3DXEffect*					pEffect;

	//nDictionary<CStrID, D3DXHANDLE>	NameToTech;
	nDictionary<DWORD, D3DXHANDLE>	FlagsToTech;
	//!!!store current tech to avoid resetting tech already active!

	//???!!!need both?!
	nDictionary<CStrID, HVar>		NameToHVar;
	nDictionary<CStrID, HVar>		SemanticToHVar;

	//!!!OnDeviceLost, OnDeviceReset events!

public:

	CShader(CStrID ID, Resources::IResourceManager* pHost): CResource(ID, pHost), pEffect(NULL) {}
	virtual ~CShader() { if (IsLoaded()) Unload(); }

	bool			Setup(ID3DXEffect* pFX);
	virtual void	Unload();

	bool			Set(HVar Var, const Data::CData& Value);
	void			SetBool(HVar Var, bool Value) { n_assert(SUCCEEDED(pEffect->SetBool(Var, Value))); }
	void			SetInt(HVar Var, int Value) { n_assert(SUCCEEDED(pEffect->SetInt(Var, Value))); }
	void			SetFloat(HVar Var, float Value) { n_assert(SUCCEEDED(pEffect->SetFloat(Var, Value))); }
	void			SetFloat4(HVar Var, const vector4& Value) { n_assert(SUCCEEDED(pEffect->SetVector(Var, (CONST D3DXVECTOR4*)&Value))); }
	void			SetMatrix(HVar Var, const matrix44& Value) { n_assert(SUCCEEDED(pEffect->SetMatrix(Var, (CONST D3DXMATRIX*)&Value))); }
	void			SetMatrixArray(HVar Var, const matrix44* pArray, int Count) { n_assert(SUCCEEDED(pEffect->SetMatrixArray(Var, (CONST D3DXMATRIX*)pArray, Count))); }
	void			SetMatrixPointerArray(HVar Var, const matrix44** pArray, int Count) { n_assert(SUCCEEDED(pEffect->SetMatrixPointerArray(Var, (CONST D3DXMATRIX**)pArray, Count))); }
	void			SetTexture(HVar Var, const CTexture& Value) { n_assert(SUCCEEDED(pEffect->SetTexture(Var, Value.GetD3D9BaseTexture()))); }
	//pEffect->SetRawValue

	bool			SetActiveFeatures(DWORD FeatureFlags);
	DWORD			Begin(bool SaveState);
	void			BeginPass(DWORD PassIdx) { n_assert(SUCCEEDED(pEffect->BeginPass(PassIdx))); }
	void			CommitChanges() { n_assert(SUCCEEDED(pEffect->CommitChanges())); }
	void			EndPass() { n_assert(SUCCEEDED(pEffect->EndPass())); }
	void			End() { n_assert(SUCCEEDED(pEffect->End())); }

	HVar			GetVarHandleByName(CStrID Name) const;
	HVar			GetVarHandleBySemantic(CStrID Semantic) const;
	bool			HasVarByName(CStrID Name) const { return NameToHVar.FindIndex(Name) != INVALID_INDEX; }
	bool			HasVarBySemantic(CStrID Semantic) const { return SemanticToHVar.FindIndex(Semantic) != INVALID_INDEX; }
	ID3DXEffect*	GetD3D9Effect() const { return pEffect; }
};

typedef Ptr<CShader> PShader;

// Doesn't support arrays for now (no corresponding types)
inline bool CShader::Set(HVar Var, const Data::CData& Value)
{
	n_assert_dbg(Var && Value.IsValid());
	if (Value.IsA<bool>()) SetBool(Var, Value);
	else if (Value.IsA<int>()) SetInt(Var, Value);
	else if (Value.IsA<float>()) SetFloat(Var, Value);
	else if (Value.IsA<vector4>()) SetFloat4(Var, Value);
	else if (Value.IsA<matrix44>()) SetMatrix(Var, Value);
	else if (Value.IsA<PTexture>()) SetTexture(Var, *Value.GetValue<PTexture>());
	else FAIL;
	OK;
}
//---------------------------------------------------------------------

inline DWORD CShader::Begin(bool SaveState)
{
	UINT Passes;
	DWORD Flags = SaveState ? 0 : D3DXFX_DONOTSAVESTATE | D3DXFX_DONOTSAVESAMPLERSTATE | D3DXFX_DONOTSAVESHADERSTATE;
	n_assert(SUCCEEDED(pEffect->Begin(&Passes, Flags)));
	return Passes;
}
//---------------------------------------------------------------------

inline CShader::HVar CShader::GetVarHandleByName(CStrID Name) const
{
	int Idx = NameToHVar.FindIndex(Name);
	return (Idx == INVALID_INDEX) ? 0 : NameToHVar.ValueAtIndex(Idx);
}
//---------------------------------------------------------------------

inline CShader::HVar CShader::GetVarHandleBySemantic(CStrID Semantic) const
{
	int Idx = NameToHVar.FindIndex(Semantic);
	return (Idx == INVALID_INDEX) ? 0 : SemanticToHVar.ValueAtIndex(Idx);
}
//---------------------------------------------------------------------

}

#endif
