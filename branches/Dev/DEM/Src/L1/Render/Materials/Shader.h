#pragma once
#ifndef __DEM_L1_RENDER_SHADER_H__
#define __DEM_L1_RENDER_SHADER_H__

#include <Resources/Resource.h>
#include <util/ndictionary.h>
#include <Render/D3D9Fwd.h>

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

	nDictionary<DWORD, D3DXHANDLE>	FlagsToTech;
	//!!!store current tech to avoid resetting tech already active!

	//???!!!need both?!
	nDictionary<CStrID, HVar>		NameToHVar;
	nDictionary<CStrID, HVar>		SemanticToHVar;

	//!!!OnDeviceLost, OnDeviceReset events!

	bool			SetupFromStream(Data::CStream& Stream);

public:

	CShader(CStrID ID, Resources::IResourceManager* pHost): CResource(ID, pHost), pEffect(NULL) {}

	bool			SetActiveFeatures(DWORD FeatureFlags);
	DWORD			Begin(); //bool SaveState);
	void			BeginPass(DWORD PassIdx);
	void			CommitChanges();
	void			EndPass();
	void			End();

	HVar			GetVarHandleByName(CStrID Name) const;
	HVar			GetVarHandleBySemantic(CStrID Semantic) const;
	bool			HasVarByName(CStrID Name) const { return NameToHVar.FindIndex(Name) != INVALID_INDEX; }
	bool			HasVarBySemantic(CStrID Semantic) const { return SemanticToHVar.FindIndex(Semantic) != INVALID_INDEX; }
	ID3DXEffect*	GetD3D9Effect() const { return pEffect; }

	//!!!SetInt etc!!!
};

typedef Ptr<CShader> PShader;

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
