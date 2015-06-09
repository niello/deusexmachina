#pragma once
#ifndef __DEM_L1_RENDER_SHADER_H__
#define __DEM_L1_RENDER_SHADER_H__

#include <Core/Object.h>
#include <Render/ShaderTech.h>

// GPU shader that implements a complete (possibly multipass) shading effect on an abstract input.
// Since input data may be different, although the desired shading effect is the same, a shader
// incapsulates a family of techniques, each of which implements an effect for a specific input data.
// An unique set of feature flags defines each input data case, so, techniques are mapped to these flags,
// and clients can request an apropriate tech.

//!!!strip out reflection and debug data for release builds!

namespace Render
{

class CShader: public Core::CObject
{
protected:

	// all dynamic buffers and input signatures used by any valid tech
	//???need this all per-shader or can store in driver? signatures - unique ones, dynamic buffers by size.
	//if store in driver, no need to store here, but harder to delete when become unref, as driver never dies
	//but input signatures are much better in a driver than lots of duplicates per-shader
	//can store unique geom vertex decl IDs/pointers and associate layouts by ID pairs!
	//CArray<CShaderInputDesc>	InputSignatures;	//???store ID inside? or dict by ID? or here store IDs too?
	//CArray<PConstantBuffer>	DynamicBuffers;		// Singletons with mutable content, for frequent updating (per-object data)

	CArray<CShaderTech>	Techs;
	//!!!check hardware support on load! API shader invalid -> pass inalid -> tech invalid
	// if Mtl->GetShaderTech(FFlags, LOD) fails, use Mtl->FallbackMtl->GetShaderTech(FFlags, LOD)
	// selects material LOD inside, uses its Shader
	//???!!!LOD distances in material itself?! per-mtl calc, if common, per-renderer/per-phase calc

public:

	CShaderTech* GetTechByFeatures(DWORD FFlags) const;
};

typedef Ptr<CShader> PShader;

}

#endif
