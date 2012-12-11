#include "Material.h"

#include <Render/Materials/ShaderVar.h>

namespace Render
{
//!!!TMP! write more elegant!
bool LoadShaderFromFX(const nString& FileName, PShader OutShader);
bool LoadShaderFromFXO(const nString& FileName, PShader OutShader);

bool CMaterial::Setup(CShader* pShader, DWORD ShaderFeatureFlags, const CShaderVarMap& StaticShaderVars)
{
	if (!pShader) FAIL;
	Shader = pShader;

	if (!pShader->IsLoaded())
	{
		//!!!TMP! write more elegant! Hide actual shader loading logic somewhere in Loader or smth.
		nString ShaderFile = pShader->GetUID();
		if (ShaderFile.CheckExtension("fxo"))
			LoadShaderFromFXO(ShaderFile, Shader);
		else
			LoadShaderFromFX(ShaderFile, Shader);

		if (!pShader->IsLoaded())
		{
			State = Resources::Rsrc_Failed;
			FAIL;
		}
	}

	FeatureFlags = ShaderFeatureFlags;
	//???set active feature for test? mb it is never used with these flags, always adding Skinned etc

	StaticVars = StaticShaderVars;
	for (int i = 0; i < StaticVars.Size(); ++i)
		StaticVars.ValueAtIndex(i).Bind(*pShader);

	//???load textures?

	State = Resources::Rsrc_Loaded;
	OK;
}
//---------------------------------------------------------------------

void CMaterial::Unload()
{
	Shader = NULL;
	FeatureFlags = 0;
	StaticVars.Clear();
	State = Resources::Rsrc_NotLoaded;
}
//---------------------------------------------------------------------

}