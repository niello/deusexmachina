#pragma once
#ifndef __DEM_L1_RENDER_FRAME_SHADER_H__
#define __DEM_L1_RENDER_FRAME_SHADER_H__

#include <Render/Pass.h>

//!!!OLD!
#include "renderpath/nrprendertarget.h"
#include "renderpath/nrpshader.h"

// Collection of 1 or more passes that describes rendering of a complex frame

namespace Render
{

class CFrameShader: public Core::CRefCounted
{
//protected:
public:

	CStrID			Name;
	nArray<PPass>	Passes;		//???smartptr?
	nString			ShaderPath;	//???need or OLD? in fact can add to any shader references in this frame shader on load!

// Pass mgmt, Textures and shader vars
//!!!can store required sorting, light usage etc here, as summary of each pass/batch requirements!

//!!!OLD!
	nArray<nRpRenderTarget> renderTargets;
	nArray<nRpShader> shaders;
	nArray<nVariable::Handle> variableHandles;

public:

	bool Init(const Data::CParams& Desc);
	void Render(const nArray<Scene::CRenderObject*>* pObjects, const nArray<Scene::CLight*>* pLights);

	//!!!OLD!
	void Validate();
	int FindShaderIndex(const nString& ShaderName) const;
	int FindRenderTargetIndex(const nString& RTName) const;
};

typedef Ptr<CFrameShader> PFrameShader;

inline void CFrameShader::Render(const nArray<Scene::CRenderObject*>* pObjects, const nArray<Scene::CLight*>* pLights)
{
	for (int i = 0; i < Passes.Size(); ++i)
		Passes[i]->Render(pObjects, pLights);
}
//---------------------------------------------------------------------

//!!!OLD!
inline void CFrameShader::Validate()
{
	for (int i = 0; i < Passes.Size(); ++i)
		Passes[i]->Validate();
}
//---------------------------------------------------------------------

}

#endif
