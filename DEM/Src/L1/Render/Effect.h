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

class CEffect: public Resources::CResourceObject
{
	__DeclareClassNoFactory;

protected:

	CDict<CStrID, PTechnique>		TechsByName;
	CDict<UPTR, PTechnique>			TechsByInputSet;

	//!!!need binary search by ID in fixed arrays!
	CFixedArray<CEffectConstant>	MaterialConsts;
	CFixedArray<CEffectResource>	MaterialResources;
	CFixedArray<CEffectSampler>		MaterialSamplers;
	CDict<CStrID, void*>			DefaultConsts;
	CDict<CStrID, PTexture>			DefaultResources;
	CDict<CStrID, PSampler>			DefaultSamplers;
	char*							pMaterialConstDefaultValues;
	UPTR							MaterialConstantBufferCount;

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

	CEffect(): pMaterialConstDefaultValues(NULL), MaterialConstantBufferCount(0) {}
	~CEffect() { SAFE_FREE(pMaterialConstDefaultValues); }

	virtual bool						IsResourceValid() const { return !!TechsByInputSet.GetCount(); }

	const CTechnique*					GetTechByName(CStrID Name) const;
	const CTechnique*					GetTechByInputSet(UPTR InputSet) const;

	const CFixedArray<CEffectConstant>&	GetMaterialConstants() const { return MaterialConsts; }
	const CFixedArray<CEffectResource>&	GetMaterialResources() const { return MaterialResources; }
	const CFixedArray<CEffectSampler>&	GetMaterialSamplers() const { return MaterialSamplers; }
	UPTR								GetMaterialConstantBufferCount() const { return MaterialConstantBufferCount; }
	void*								GetConstantDefaultValue(CStrID ID) const;
	PTexture							GetResourceDefaultValue(CStrID ID) const;
	PSampler							GetSamplerDefaultValue(CStrID ID) const;
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

inline void* CEffect::GetConstantDefaultValue(CStrID ID) const
{
	IPTR Idx = DefaultConsts.FindIndex(ID);
	return Idx == INVALID_INDEX ? NULL : DefaultConsts.ValueAt(Idx);
}
//---------------------------------------------------------------------

inline PTexture CEffect::GetResourceDefaultValue(CStrID ID) const
{
	IPTR Idx = DefaultResources.FindIndex(ID);
	return Idx == INVALID_INDEX ? NULL : DefaultResources.ValueAt(Idx);
}
//---------------------------------------------------------------------

inline PSampler CEffect::GetSamplerDefaultValue(CStrID ID) const
{
	IPTR Idx = DefaultSamplers.FindIndex(ID);
	return Idx == INVALID_INDEX ? NULL : DefaultSamplers.ValueAt(Idx);
}
//---------------------------------------------------------------------

}

#endif
