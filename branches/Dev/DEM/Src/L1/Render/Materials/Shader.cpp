#include "Shader.h"

#include <Render/Renderer.h>

namespace Render
{

bool CShader::Setup(ID3DXEffect* pFX)
{
	if (!pFX) FAIL;

	pEffect = pFX;

	D3DXEFFECT_DESC Desc = { 0 };    
	n_assert(SUCCEEDED(pEffect->GetDesc(&Desc)));

	n_assert(Desc.Techniques > 0);
	for (UINT i = 0; i < Desc.Techniques; ++i)
	{
		D3DXHANDLE hTech = pEffect->GetTechnique(i);
		D3DXTECHNIQUE_DESC TechDesc;
		n_assert(SUCCEEDED(pEffect->GetTechniqueDesc(hTech, &TechDesc)));
		//NameToTech.Add(CStrID(TechDesc.Name), hTech);

		D3DXHANDLE hFeatureAnnotation = pEffect->GetAnnotationByName(hTech, "Mask");
		if (hFeatureAnnotation)
		{
			LPCSTR pFeatMask = NULL;
			n_assert(SUCCEEDED(pEffect->GetString(hFeatureAnnotation, &pFeatMask)));
			DWORD Mask = RenderSrv->ShaderFeatureStringToMask(pFeatMask);
			FlagsToTech.Add(Mask, hTech);
		}
		else n_printf("WARNING: No feature mask annotation in technique '%s'!\n", TechDesc.Name);
	}

	for (UINT i = 0; i < Desc.Parameters; ++i)
	{
		HVar hVar = pEffect->GetParameter(NULL, i);
		D3DXPARAMETER_DESC ParamDesc = { 0 };
		n_assert(SUCCEEDED(pEffect->GetParameterDesc(hVar, &ParamDesc)));
		NameToHVar.Add(CStrID(ParamDesc.Name), hVar);
		SemanticToHVar.Add(CStrID(ParamDesc.Semantic), hVar);
		// Can also check type, if needed
	}

	//!!!subscribe lost & reset!

	State = Resources::Rsrc_Loaded;
	OK;
}
//---------------------------------------------------------------------

void CShader::Unload()
{
	//!!!unsubscribe lost & reset!
	SAFE_RELEASE(pEffect);
	State = Resources::Rsrc_NotLoaded;
}
//---------------------------------------------------------------------

}