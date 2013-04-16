#pragma once
#ifndef __DEM_L1_RENDER_FRAME_SHADER_H__
#define __DEM_L1_RENDER_FRAME_SHADER_H__

#include <Render/Pass.h>

// Collection of 1 or more passes that describes rendering of a complex frame

namespace Render
{

class CFrameShader: public Core::CRefCounted
{
//protected:
public:

	CStrID			Name;
	nArray<PPass>	Passes;		//???smartptr?
	CShaderVarMap	ShaderVars;

// Pass mgmt
//!!!can store required sorting, light usage etc here, as summary of each pass/batch requirements!

public:

	bool Init(const Data::CParams& Desc);
	void Render(const nArray<Scene::CRenderObject*>* pObjects, const nArray<Scene::CLight*>* pLights);
};

typedef Ptr<CFrameShader> PFrameShader;

inline void CFrameShader::Render(const nArray<Scene::CRenderObject*>* pObjects, const nArray<Scene::CLight*>* pLights)
{
	//!!!PERF: for passes and batches - if shader is the same as set, don't reset it!
	for (int i = 0; i < Passes.Size(); ++i)
		Passes[i]->Render(pObjects, pLights);
}
//---------------------------------------------------------------------

}

#endif
