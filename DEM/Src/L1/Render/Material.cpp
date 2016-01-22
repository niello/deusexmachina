//#include "Material.h"
//
//#include <Render/Materials/ShaderVar.h>
//#include <Core/Factory.h>
//
//namespace Render
//{
//__ImplementClass(Render::CMaterial, 'MTRL', Resources::CResourceObject);
//
//bool LoadTextureUsingD3DX(const CString& FileName, PTexture OutTexture);
//
//bool CMaterial::Setup(CShader* pShader, UPTR ShaderFeatureFlags, const CShaderVarMap& StaticShaderVars)
//{
//	if (!pShader) FAIL;
//	Shader = pShader;
//
//	//???try dynamic shader loading here? anyway without frame shader set this has no meaning
//	if (!pShader->IsLoaded())
//	{
//		State = Resources::Rsrc_Failed;
//		FAIL;
//	}
//
//	FeatureFlags = ShaderFeatureFlags;
//	//???set active feature for test? mb it is never used with these flags, always adding Skinned etc
//
//	static const CString StrTextures("Textures:");
//
//	StaticVars = StaticShaderVars;
//	for (int i = 0; i < StaticVars.GetCount(); ++i)
//	{
//		CShaderVar& Var = StaticVars.ValueAt(i);
//		Var.Bind(*pShader);
//
//		//!!!non-file textures (forex RTs) will fail to load here! ensure they are
//		// in loaded state or they load themselvef properly!
//		if (Var.Value.IsA<PTexture>())
//		{
//			PTexture Tex = Var.Value.GetValue<PTexture>();
//			if (!Tex->IsLoaded()) LoadTextureUsingD3DX(StrTextures + Tex->GetUID().CStr(), Tex);
//			//if (!Tex->IsLoaded() && !LoadTextureUsingD3DX(Tex->GetUID().CStr(), Tex))
//			//{
//			//	State = Resources::Rsrc_Failed;
//			//	FAIL;
//			//}
//		}
//	}
//
//	//???load textures?
//
//	State = Resources::Rsrc_Loaded;
//	OK;
//}
////---------------------------------------------------------------------
//
//void CMaterial::Unload()
//{
//	Shader = NULL;
//	FeatureFlags = 0;
//	StaticVars.Clear();
//	State = Resources::Rsrc_NotLoaded;
//}
////---------------------------------------------------------------------
//
//}