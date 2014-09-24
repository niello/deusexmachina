#pragma once
#ifndef __DEM_L1_RENDER_PASS_H__
#define __DEM_L1_RENDER_PASS_H__

#include <Render/RenderServer.h>

// Pass encapsulates rendering to one RT or MRT

namespace Data
{
	class CParams;
}

namespace Render
{
class CRenderObject;
class CLight;

class CPass: public Core::CObject
{
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

public:

	CPass(): ClearFlags(0), ClearColor(0xff000000), ClearDepth(1.f), ClearStencil(0) {}
		//pFrameShader(NULL) {}

	virtual ~CPass() {}

	virtual bool Init(CStrID PassName, const Data::CParams& Desc, const CDict<CStrID, PRenderTarget>& RenderTargets);
	virtual void Render(const CArray<CRenderObject*>* pObjects, const CArray<CLight*>* pLights) = 0;
};

typedef Ptr<CPass> PPass;

}

#endif
