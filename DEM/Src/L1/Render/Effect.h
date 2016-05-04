#pragma once
#ifndef __DEM_L1_RENDER_EFFECT_H__
#define __DEM_L1_RENDER_EFFECT_H__

#include <Resources/ResourceObject.h>
#include <Render/Technique.h>
#include <Data/StringID.h>
#include <Data/Dictionary.h>

// A complete (possibly multipass) shading effect on an abstract input data.
// Since input data may be different, although the desired shading effect is the same, a shader
// incapsulates a family of techniques, each of which implements an effect for a specific input data.
// An unique set of feature flags defines each input data case, so, techniques are mapped to these
// flags, and clients can request an apropriate tech by its features. Request by ID is supported too.

namespace Render
{

struct CEffectConstant
{
	CStrID			ID;
	EShaderType		ShaderType;
	HConst			Handle;
	HConstBuffer	BufferHandle;
	void*			pDefaultValue;
	//???store val size?
};

struct CEffectResource
{
	CStrID			ID;
	EShaderType		ShaderType;
	HResource		Handle;
	PTexture		DefaultValue;
};

struct CEffectSampler
{
	CStrID			ID;
	EShaderType		ShaderType;
	HSampler		Handle;
	PSampler		DefaultValue;
};

class CEffect: public Resources::CResourceObject
{
protected:

	CDict<CStrID, PTechnique>		TechsByName;
	CDict<UPTR, PTechnique>			TechsByInputSet;

	//global params signature for validation against a current render path

	//!!!can use fixed arrays with binary search by ID!
	//material can reference this data by index, since these arrays never change after effect loading
	CFixedArray<CEffectConstant>	MaterialConsts;
	CFixedArray<CEffectResource>	MaterialResources;
	CFixedArray<CEffectSampler>		MaterialSamplers;
	char*							pMaterialConstDefaultValues;

	// call Material LOD CEffectParams / CEffectInstance?
	//!!!check hardware support on load! Render state invalid -> tech invalid
	// if Mtl->GetShaderTech(FFlags, LOD) fails, use Mtl->FallbackMtl->GetShaderTech(FFlags, LOD)
	// selects material LOD inside, uses its Shader
	//???!!!LOD distances in material itself?! per-mtl calc, if common, per-renderer/per-phase calc

	void								BeginAddTechs(UPTR Count) { TechsByName.BeginAdd(Count); TechsByInputSet.BeginAdd(Count); }
	void								AddTech(PTechnique Tech) { TechsByName.Add(Tech->GetName(), Tech); TechsByInputSet.Add(Tech->GetShaderInputSetID(), Tech); }
	void								EndAddTechs() { TechsByName.EndAdd(); TechsByInputSet.EndAdd(); }

	friend class Resources::CEffectLoader;

public:

	CEffect(): pMaterialConstDefaultValues(NULL) {}
	~CEffect() { SAFE_FREE(pMaterialConstDefaultValues); }

	virtual bool						IsResourceValid() const { return !!TechsByInputSet.GetCount(); }

	const CTechnique*					GetTechByName(CStrID Name) const;
	const CTechnique*					GetTechByInputSet(UPTR InputSet) const;

	const CFixedArray<CEffectConstant>&	GetMaterialConstants() const { return MaterialConsts; }
	const CFixedArray<CEffectResource>&	GetMaterialResources() const { return MaterialResources; }
	const CFixedArray<CEffectSampler>&	GetMaterialSamplers() const { return MaterialSamplers; }
};

typedef Ptr<CEffect> PEffect;

inline const CTechnique* CEffect::GetTechByName(CStrID Name) const
{
	IPTR Idx = TechsByName.FindIndex(Name);
	return Idx == INVALID_INDEX ? NULL : TechsByName.ValueAt(Idx).GetUnsafe();
}
//---------------------------------------------------------------------

inline const CTechnique* CEffect::GetTechByInputSet(UPTR InputSet) const
{
	IPTR Idx = TechsByInputSet.FindIndex(InputSet);
	return Idx == INVALID_INDEX ? NULL : TechsByInputSet.ValueAt(Idx).GetUnsafe();
}
//---------------------------------------------------------------------

}

#endif
