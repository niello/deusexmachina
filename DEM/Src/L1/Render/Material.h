#pragma once
#ifndef __DEM_L1_RENDER_MATERIAL_H__
#define __DEM_L1_RENDER_MATERIAL_H__

#include <Render/Shader.h>
#include <Data/Array.h>

// Material consists of a shader, material feature flags (which will be combined with
// other feature flags to determine shader techniques) and shader variable values, that
// are the same for all objects using this material. //???default values for per-object vars too?

// NB: Material depends on other resource, Shader

namespace Render
{
typedef Ptr<class CMaterial> PMaterial;

class CMaterial: public Resources::CResourceObject
{
	__DeclareClass(CMaterial);

protected:

	// All this matches any tech that is valid for this material:
	// IndexInMeta -> CB
	// Handle -> Rsrc
	// Handle -> Samp

	//???or in objects? how to sort in renderer? select one LOD and sort by it? (Mtl + LODIdx?)
	struct CLOD
	{
		PShader			Shader;
		//CFeatureFlags	FeatureFlags;
		//!!!var values!
		//one shader var can store Name, Handle and Value (for binding)
		//???or how to implement binding now?
	};

	//???material LOD distances here or per-phase or per-renderer or per-object?

	CArray<CLOD>	LOD;
	PMaterial		FallbackMaterial;

public:

	virtual ~CMaterial();// { if (IsLoaded()) Unload(); }

	bool					Setup(CShader* pShader, DWORD ShaderFeatureFlags); //, const CShaderVarMap& StaticShaderVars);
	virtual void			Unload();

	virtual bool			IsResourceValid() const { FAIL; }

	//???GetTech(LOD, FFlags)? with auto-fallback

	//CShader*				GetShader() const { return Shader.GetUnsafe(); }
	//CFeatureFlags			GetFeatureFlags() const { return FeatureFlags; }
	//const CShaderVarMap&	GetStaticVars() const { return StaticVars; }
};

}

#endif
