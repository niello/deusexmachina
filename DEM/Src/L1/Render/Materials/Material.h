#pragma once
#ifndef __DEM_L1_RENDER_MATERIAL_H__
#define __DEM_L1_RENDER_MATERIAL_H__

#include <Render/Materials/ShaderVar.h>
//#include <Resources/Resource.h>
//#include <Data/StringID.h>
//#include <util/ndictionary.h>

// Material consists of material shader (typically phong, cook-torrance or smth), feature flags that specify
// additional features (light, alpha etc) and variable info. Textures are managed as all other variables.
// Material is all that is needed to render geometry.

// NB: Material is dependent on other resource, Shader
//???load textures? some vars can be PTexture

namespace Render
{
//typedef Ptr<class CShader> PShader;
//typedef nDictionary<CStrID, class CShaderVar> CShaderVarMap;

class CMaterial: public Resources::CResource
{
protected:

	PShader			Shader;
	DWORD			FeatureFlags;
	CShaderVarMap	StaticVars;

public:

	CMaterial(CStrID ID): CResource(ID), FeatureFlags(0) {}

	bool					Setup(CShader* pShader, DWORD ShaderFeatureFlags, const CShaderVarMap& StaticShaderVars);
	virtual void			Unload();

	CShader*				GetShader() const { return Shader.get_unsafe(); }
	DWORD					GetFeatureFlags() const { return FeatureFlags; }
	const CShaderVarMap&	GetStaticVars() const { return StaticVars; }
};

typedef Ptr<CMaterial> PMaterial;

}

#endif
