#include "FrameShader.h"

#include <Data/Params.h>

//!!!while no factory!
#include <Render/PassGeometry.h>
#include <Render/PassOcclusion.h>
#include <Render/PassPosteffect.h>

namespace Render
{

bool CFrameShader::Init(const Data::CParams& Desc)
{
	Desc.Get(ShaderPath, CStrID("ShaderPath"));

//!!!ShaderVars!

	nDictionary<CStrID, PRenderTarget> RT;

	Data::CParam* pParam;
	if (Desc.Get(pParam, CStrID("RenderTargets")))
	{
		Data::CParams& RTList = *pParam->GetValue<Data::PParams>();
		for (int i = 0; i < RTList.GetCount(); ++i)
		{
			const Data::CParam& RTPrm = RTList[i];
			// Create RT (name, rtformat, dsformat, size/relsize, msaa)
		}
	}

	if (Desc.Get(pParam, CStrID("Passes")))
	{
		Data::CParams& PassList = *pParam->GetValue<Data::PParams>();
		PPass* pCurrPass = Passes.Reserve(PassList.GetCount());
		for (int i = 0; i < PassList.GetCount(); ++i, ++pCurrPass)
		{
			const Data::CParam& PassPrm = PassList[i];
			Data::CParams PassDesc = *PassPrm.GetValue<Data::PParams>();

			//!!!???Use factory?!
			const nString& PassType = PassDesc.Get<nString>(CStrID("Type"), NULL);
			if (PassType.IsEmpty()) *pCurrPass = n_new(CPassGeometry);
			else if (PassType == "Occlusion") *pCurrPass = n_new(CPassOcclusion);
			else if (PassType == "Posteffect") *pCurrPass = n_new(CPassPosteffect);
			else /*if (PassType == "Geometry")*/ *pCurrPass = n_new(CPassGeometry);

			if (!(*pCurrPass)->Init(PassPrm.GetName(), PassDesc, RT)) FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

//!!!OLD!
int CFrameShader::FindShaderIndex(const nString& ShaderName) const
{
	for (int i = 0; i < shaders.Size(); ++i)
		if (shaders[i].GetName() == ShaderName)
			return i;
	return -1;
}
//---------------------------------------------------------------------

//!!!OLD!
int CFrameShader::FindRenderTargetIndex(const nString& RTName) const
{
	for (int i = 0; i < renderTargets.Size(); ++i)
		if (renderTargets[i].GetName() == RTName)
			return i;
	return -1;
}
//---------------------------------------------------------------------

}