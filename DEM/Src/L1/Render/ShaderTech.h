#pragma once
#ifndef __DEM_L1_RENDER_SHADER_TECH_H__
#define __DEM_L1_RENDER_SHADER_TECH_H__

#include <Render/ShaderPass.h>
#include <Data/Array.h>

// A particular shader implementation for a specific input data. It includes
// geometry specifics (static/skinned), environment (light count, fog),
// additional material effects (parallax, alpha test). A combination of input
// states is represented as a set of feature flags. Technique can accept different
// combinations of these flags, though ones close to each other. So, instead of storing
// each particular combination, we specify what flags should be set and what should be
// unset, and allow other flags to have any state.

namespace Render
{
typedef Data::CFlags CFeatureFlags;

class CShaderTech
{
public:

	CShaderVars			Vars; //???ptr to abstract base for API-specific class?
	//???state?
	CFeatureFlags		IncludedFFlags;	// Feature flags that must be set to use this tech
	CFeatureFlags		ExcludedFFlags;	// Feature flags that must be unset to use this tech

	CArray<CShaderPass>	Passes;

	bool IsApplicableForFeatureFlags(CFeatureFlags FFlags) const { return FFlags.Is(IncludedFFlags) && FFlags.IsNot(ExcludedFFlags); }
};

}

#endif
