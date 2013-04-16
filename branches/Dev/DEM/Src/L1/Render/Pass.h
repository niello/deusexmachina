#pragma once
#ifndef __DEM_L1_RENDER_PASS_H__
#define __DEM_L1_RENDER_PASS_H__

#include <Render/RenderServer.h>

// Pass encapsulates rendering to one RT or MRT

namespace Data
{
	class CParams;
}

namespace Scene
{
	class CRenderObject;
	class CLight;
}

namespace Render
{
//!!!TMP!
//class CFrameShader;

class CPass: public Core::CRefCounted
{
	//DeclareRTTI;

//protected:
public:

	CStrID			Name;
	PShader			Shader;
	CShaderVarMap	ShaderVars;
	PRenderTarget	RT[CRenderServer::MaxRenderTargetCount];
	DWORD			ClearFlags;
	DWORD			ClearColor;		// ARGB
	float			ClearDepth;
	uchar			ClearStencil;

	// batches - now only in PassGeometry

	//bool Profile;

	//!!TMP!
	//CFrameShader* pFrameShader;

public:

	CPass(): ClearFlags(0), ClearColor(0xff000000), ClearDepth(1.f), ClearStencil(0) {}
		//pFrameShader(NULL) {}

	virtual ~CPass() {}

	virtual bool Init(CStrID PassName, const Data::CParams& Desc, const nDictionary<CStrID, PRenderTarget>& RenderTargets);
	virtual void Render(const nArray<Scene::CRenderObject*>* pObjects, const nArray<Scene::CLight*>* pLights) = 0;
};

typedef Ptr<CPass> PPass;

}

#endif
