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

// Renderables are queued and filtered by this type. For example, opaque objects are typically
// rendered before alpha-blended ones, and are not included in a depth pre-pass at all. This enum
// is designed with no runtime extensibility in mind, although it is possible. I see no benefit in
// this, because after this enum is established during a development, it is very unlikely to change.
// Also I don't like hacks like "render A after B and C for no reason". But you may implement it.
enum EEffectType
{
	EffectType_Opaque,		// No transparency, all possible depth optimizations
	EffectType_AlphaTest,	// No transparency, Z-write in a PS
	EffectType_Skybox,		// Infinitely far opaque object for non-filled part of the screen only
	EffectType_AlphaBlend,	// Back to front, requires all objects behind to be already drawn

	EffectType_Other
};

class CEffect: public Resources::CResourceObject
{
	__DeclareClassNoFactory;

protected:

	EEffectType						Type;

	CDict<CStrID, PTechnique>		TechsByName;
	CDict<UPTR, PTechnique>			TechsByInputSet;

	//!!!need binary search by ID in fixed arrays!
	CFixedArray<CEffectConstant>	MaterialConsts;
	CFixedArray<CEffectResource>	MaterialResources;
	CFixedArray<CEffectSampler>		MaterialSamplers;
	CDict<CStrID, void*>			DefaultConsts;
	CDict<CStrID, PTexture>			DefaultResources;
	CDict<CStrID, PSampler>			DefaultSamplers;
	char*							pMaterialConstDefaultValues = nullptr;
	UPTR							MaterialConstantBufferCount = 0;

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

	CEffect();
	~CEffect();

	virtual bool						IsResourceValid() const { return !!TechsByInputSet.GetCount(); }

	const CTechnique*					GetTechByName(CStrID Name) const;
	const CTechnique*					GetTechByInputSet(UPTR InputSet) const;

	EEffectType							GetType() const { return Type; }
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

}

#endif
