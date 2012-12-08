#pragma once
#ifndef __DEM_L1_RENDER_MATERIAL_H__
#define __DEM_L1_RENDER_MATERIAL_H__

#include <Render/Materials/ShaderVar.h>

// Material consists of material shader (typically phong, cook-torrance or smth), feature flags that specify
// additional features (light, alpha etc) and variable info. Textures are managed as all other variables.
// Material is all that is needed to render geometry.

namespace Render
{

class CMaterial: public Resources::CResource
{
protected:

	PShader			Shader;
	DWORD			FeatureFlags;
	CShaderVarMap	StaticVars;

	//!!!mb Loaded, but definitely Invalid until shader is Loaded!

public:

	CMaterial(CStrID ID, Resources::IResourceManager* pHost): CResource(ID, pHost), FeatureFlags(0) {}

	void		SetShader(CShader* pShader);
	CShader*	GetShader() const { return Shader.get_unsafe(); }
};

typedef Ptr<CMaterial> PMaterial;

inline void CMaterial::SetShader(CShader* pShader)
{
	n_assert(pShader);
	Shader = pShader;

	if (Shader->IsLoaded())
	{
		//???set active feature for test? mb it is never used with these flags, always adding Skinned etc

		for (int i = 0; i < StaticVars.Size(); ++i)
			StaticVars.ValueAtIndex(i).Bind(*pShader);
	}
}
//---------------------------------------------------------------------

}

#endif
