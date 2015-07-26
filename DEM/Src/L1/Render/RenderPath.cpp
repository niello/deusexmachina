#include "RenderPath.h"

#include <Render/Camera.h>
#include <Render/SPS.h>
#include <Render/SceneNodeUpdateInSPS.h>
#include <Render/RenderPhase.h>
//#include <IO/IOServer.h>

namespace Render
{

bool CRenderPath::Init(CGPUDriver& Driver, const Data::CParams& Desc)
{
	// Load shared shader variables
	// For once, optionally can read values
	// For others can read default values
	// Can use null (empty CData) if there is no default value


/*
	CString ShaderPath;
	Desc.Get(ShaderPath, CStrID("ShaderPath"));

	Data::CParam* pPrm;
	if (Desc.Get(pPrm, CStrID("Shaders")))
	{
		CString ShaderPathMangled = IOSrv->ResolveAssigns(ShaderPath) + "/";
		Data::CParams& List = *pPrm->GetValue<Data::PParams>();
		for (int i = 0; i < List.GetCount(); ++i)
		{
			CStrID ShaderID = List[i].GetName();
			CString FileName = ShaderPathMangled + List[i].GetValue<CString>();

			PShader Shader = RenderSrv->ShaderMgr.GetOrCreateTypedResource(ShaderID);
			n_assert(!Shader->IsLoaded()); //!!!now just to check!

			const char* pExt = FileName.GetExtension();
			if (pExt)
			{
				if (!n_stricmp(pExt, "fx")) LoadShaderFromFX(FileName, ShaderPath, Shader);
				else LoadShaderFromFXO(FileName, Shader);
			}
			else
			{
				if (!LoadShaderFromFXO(FileName + ".fxo", Shader))
					if (!LoadShaderFromFX(FileName + ".fx", ShaderPath, Shader))
						if (!LoadShaderFromFXO(FileName, Shader)) FAIL;
			}

			if (!Shader->IsLoaded()) FAIL;
		}
	}

	//!!!DUPLICATE CODE!+
	ShaderVars.BeginAdd();

	if (Desc.Get(pPrm, CStrID("ShaderVars")))
	{
		Data::CParams& Vars = *pPrm->GetValue<Data::PParams>();
		for (int i = 0; i < Vars.GetCount(); ++i)
		{
			Data::CParam& PrmVar = Vars.Get(i);
			CShaderVar& Var = ShaderVars.Add(PrmVar.GetName());
			Var.SetName(PrmVar.GetName());
			Var.Value = PrmVar.GetRawValue();
		}
	}

	if (Desc.Get(pPrm, CStrID("Textures"))) //!!!can init above from string values!
	{
		Data::CParams& Vars = *pPrm->GetValue<Data::PParams>();
		for (int i = 0; i < Vars.GetCount(); ++i)
		{
			Data::CParam& PrmVar = Vars.Get(i);
			CShaderVar& Var = ShaderVars.Add(PrmVar.GetName());
			Var.SetName(PrmVar.GetName());
			Var.Value = RenderSrv->TextureMgr.GetOrCreateTypedResource(CStrID(PrmVar.GetValue<CString>().CStr()));
		}
	}

	ShaderVars.EndAdd();
	//!!!DUPLICATE CODE!-

	CDict<CStrID, PRenderTarget> RenderTargets;

	if (Desc.Get(pPrm, CStrID("RenderTargets")))
	{
		Data::CParams& RTList = *pPrm->GetValue<Data::PParams>();
		RenderTargets.BeginAdd(RTList.GetCount());
		for (int i = 0; i < RTList.GetCount(); ++i)
		{
			const Data::CParam& RTPrm = RTList[i];
			Data::CParams& RTDesc = *RTPrm.GetValue<Data::PParams>();

			EPixelFormat RTFmt = RenderSrv->GetPixelFormat(RTDesc.Get<CString>(CStrID("RTFormat"))); //???CStrID?
			EPixelFormat DSFmt = RenderSrv->GetPixelFormat(RTDesc.Get<CString>(CStrID("DSFormat"), NULL)); //???CStrID?
			float W = RTDesc.Get<float>(CStrID("Width"), 1.f);
			float H = RTDesc.Get<float>(CStrID("Height"), 1.f);
			bool IsWHAbs = RTDesc.Get<bool>(CStrID("IsSizeAbsolute"), false);
			bool UseAutoDS = RTDesc.Get<bool>(CStrID("UseAutoDepthStencil"), false);

			//!!!read msaa, texture sizes!

			PRenderTarget& RT = RenderTargets.Add(RTPrm.GetName());
			RT = n_new(CRenderTarget);
			if (!RT->Create(RTPrm.GetName(), RTFmt, DSFmt, W, H, IsWHAbs, MSAA_None, 0, 0, UseAutoDS)) FAIL;
		}
		RenderTargets.EndAdd();
	}

	if (Desc.Get(pPrm, CStrID("Passes")))
	{
		Data::CParams& PassList = *pPrm->GetValue<Data::PParams>();
		PPass* pCurrPass = Passes.Reserve(PassList.GetCount());
		for (int i = 0; i < PassList.GetCount(); ++i, ++pCurrPass)
		{
			const Data::CParam& PassPrm = PassList[i];
			Data::CParams& PassDesc = *PassPrm.GetValue<Data::PParams>();

			//!!!???Use factory?!
			const CString& PassType = PassDesc.Get<CString>(CStrID("Type"), NULL);
			if (PassType.IsEmpty()) *pCurrPass = n_new(CPassGeometry);
			else if (PassType == "Occlusion") *pCurrPass = n_new(CPassOcclusion);
			else if (PassType == "Posteffect") *pCurrPass = n_new(CPassPosteffect);
			else //if (PassType == "Geometry")
				*pCurrPass = n_new(CPassGeometry);

			if (!(*pCurrPass)->Init(PassPrm.GetName(), PassDesc, RenderTargets)) FAIL;
		}
	}
	*/

	OK;
}
//---------------------------------------------------------------------

bool CRenderPath::Render(const CCamera& MainCamera, CSPS& SPS)
{
	n_assert_dbg(!VisibleObjects.GetCount() && !VisibleLights.GetCount());

	if (!Phases.GetCount()) OK;

	bool UseLighting = true; //!!!settings/states!

	//???apply cumulative filter which is an union of filters of each phase?
	CArray<CLight*>* pVisibleLights = UseLighting ? &VisibleLights : NULL;
	SPS.QueryVisibleObjectsAndLights(MainCamera.GetViewProjMatrix(), &VisibleObjects, pVisibleLights);

	//!!!set commons which will not be reset by the first phase //???or let the phase set them?

	//!!!clear all phases' render targets and DS surfaces
	//at the beginning of the frame, as recommended, especially for SLI

	for (DWORD i = 0; i < Phases.GetCount(); ++i)
	{
		if (!Phases[i]->Render(MainCamera, SPS, *this))
		{
			VisibleObjects.Clear();
			VisibleLights.Clear();
			FAIL;
		}
	}

	VisibleObjects.Clear();
	VisibleLights.Clear();

	OK;
}
//---------------------------------------------------------------------

}