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
protected:

	// all dynamic buffers used by any valid tech
	//???need this all per-shader or can store in driver? dynamic buffers one per size.
	//CArray<PConstantBuffer>	DynamicBuffers;		// Map-Discard with mutable content, for frequent updating (per-object data)

	CDict<CStrID, CTechnique>	Techs;

	// call Material LOD CEffectParams / CEffectInstance?
	//!!!check hardware support on load! Render state invalid -> tech invalid
	// if Mtl->GetShaderTech(FFlags, LOD) fails, use Mtl->FallbackMtl->GetShaderTech(FFlags, LOD)
	// selects material LOD inside, uses its Shader
	//???!!!LOD distances in material itself?! per-mtl calc, if common, per-renderer/per-phase calc

public:

	const CTechnique*	GetTechByName(CStrID Name) const;
	const CTechnique*	GetTechByInputID(CStrID InputID) const;
};

typedef Ptr<CEffect> PEffect;

inline const CTechnique* CEffect::GetTechByName(CStrID Name) const
{
	IPTR Idx = Techs.FindIndex(Name);
	return Idx == INVALID_INDEX ? NULL : &Techs.ValueAt(Idx);
}
//---------------------------------------------------------------------

}

#endif
