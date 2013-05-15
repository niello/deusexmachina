#include "Shader.h"

#include <Render/RenderServer.h>
#include <Events/EventManager.h>

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
			nString FeatAnnotation(pFeatMask);
			nArray<nString> FeatureMasks;
			FeatAnnotation.Tokenize(",", FeatureMasks);
			n_assert_dbg(FeatureMasks.GetCount());
			for (int MaskIdx = 0; MaskIdx < FeatureMasks.GetCount(); ++MaskIdx)
				FlagsToTech.Add(RenderSrv->ShaderFeatureStringToMask(FeatureMasks[MaskIdx]), hTech);
		}
		else n_printf("WARNING: No feature mask annotation in technique '%s'!\n", TechDesc.Name);
	}

	// It is good for pass and batch shaders with only one tech
	// It seems, pass shaders use the only tech without setting it
	//n_assert(SetTech(FlagsToTech.ValueAtIndex(0)));

	for (UINT i = 0; i < Desc.Parameters; ++i)
	{
		HVar hVar = pEffect->GetParameter(NULL, i);
		D3DXPARAMETER_DESC ParamDesc = { 0 };
		n_assert(SUCCEEDED(pEffect->GetParameterDesc(hVar, &ParamDesc)));
		NameToHVar.Add(CStrID(ParamDesc.Name), hVar);
		if (ParamDesc.Semantic) SemanticToHVar.Add(CStrID(ParamDesc.Semantic), hVar);
		// Can also check type, if needed
	}

	SUBSCRIBE_PEVENT(OnRenderDeviceLost, CShader, OnDeviceLost);
	SUBSCRIBE_PEVENT(OnRenderDeviceReset, CShader, OnDeviceReset);

	State = Resources::Rsrc_Loaded;
	OK;
}
//---------------------------------------------------------------------

void CShader::Unload()
{
	UNSUBSCRIBE_EVENT(OnRenderDeviceLost);
	UNSUBSCRIBE_EVENT(OnRenderDeviceReset);

	hCurrTech = NULL;
	FlagsToTech.Clear();
	NameToHVar.Clear();
	SemanticToHVar.Clear();
	SAFE_RELEASE(pEffect);
	State = Resources::Rsrc_NotLoaded;
}
//---------------------------------------------------------------------

bool CShader::OnDeviceLost(const Events::CEventBase& Ev)
{
	pEffect->OnLostDevice();
	OK;
}
//---------------------------------------------------------------------

bool CShader::OnDeviceReset(const Events::CEventBase& Ev)
{
	pEffect->OnResetDevice();
	OK;
}
//---------------------------------------------------------------------

}