#pragma once
#ifndef __DEM_L1_RENDER_MATERIAL_H__
#define __DEM_L1_RENDER_MATERIAL_H__

#include <Render/Shader.h>
#include <Resources/Resource.h>

// Material consists of a shader, material feature flags (which will be combined with
// other feature flags to determine shader techniques) and shader variable values, that
// are the same for all objects using this material. //???default values for per-object vars too?

// NB: Material depends on other resource, Shader

namespace Render
{
typedef Ptr<class CMaterial> PMaterial;

class CMaterial: public Resources::CResource
{
	__DeclareClass(CMaterial);

protected:

	struct CLOD
	{
		PShader			Shader;
		CFeatureFlags	FeatureFlags;
		//!!!var values!
		//one shader var can store Name, Handle and Value (for binding)
		//???or how to implement binding now?
	};

	//???material LOD distances here or per-phase or per-renderer or per-object?

	CArray<CLOD>	LOD;
	PMaterial		FallbackMaterial;

public:

	CMaterial(CStrID ID): CResource(ID), FeatureFlags(0) {}
	virtual ~CMaterial() { if (IsLoaded()) Unload(); }

	bool					Setup(CShader* pShader, DWORD ShaderFeatureFlags) //, const CShaderVarMap& StaticShaderVars);
	virtual void			Unload();

	//???GetTech(LOD, FFlags)? with auto-fallback

	//CShader*				GetShader() const { return Shader.GetUnsafe(); }
	//CFeatureFlags			GetFeatureFlags() const { return FeatureFlags; }
	//const CShaderVarMap&	GetStaticVars() const { return StaticVars; }
};

}

#endif
