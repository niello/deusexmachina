#pragma once
#ifndef __DEM_L1_RENDER_SHADER_H__
#define __DEM_L1_RENDER_SHADER_H__

#include <Core/Object.h>
#include <Render/ShaderTech.h>

// GPU shader that implements a complete (possibly multipass) shading effect on an abstract input.
// Since input data may be different, although the desired shading effect is the same, a shader
// incapsulates a family of techniques, each of which implements an effect for a specific input data.
// An unique set of feature flags defines each input data case, so, techniques are mapped to these flags
// and clients can request an apropriate tech by passing flags they combined.

namespace Render
{

class CShader: public Core::CObject
{
protected:

	CArray<CShaderTech>	Techs;
	//!!!check hardware support on load! API shader invalid -> pass inalid -> tech invalid
	// if Mtl->GetShaderTech(FFlags, LOD) fails, use Mtl->FallbackMtl->GetShaderTech(FFlags, LOD)
	// selects material LOD inside, uses its Shader
	//???!!!LOD distances in material itself?! per-mtl calc, if common, per-renderer/per-phase calc

public:

	CShaderVars			Vars; //???ptr to abstract base for API-specific class?
	//???state?

	CShaderTech* GetTechByFeatures(DWORD FFlags) const;
};

typedef Ptr<CShader> PShader;

}

#endif
