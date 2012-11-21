#pragma once
#ifndef __DEM_L1_RENDER_PASS_H__
#define __DEM_L1_RENDER_PASS_H__

#include <Core/RefCounted.h>
#include <Data/StringID.h>

//!!!OLD!
#include <gfx2/nshaderparams.h>
#include <util/nfixedarray.h>
#include <variable/nvariablecontext.h>

// Pass encapsulates rendering to one RT or MRT

namespace Render
{
//!!!TMP!
class CFrameShader;

class CPass: public Core::CRefCounted
{
	//DeclareRTTI;

//protected:
public:

	CStrID Name;
	// Shader, RT/MRT, shader vars, batches
	// RT clear flags, color & values

	//bool Profile;

	//!!TMP!
	CFrameShader* pFrameShader;

//!!!OLD!
	struct ShaderParam
	{
		nShaderState::Type type;
		nString stateName;
		nString value;
	};
	nString shaderAlias;
	nString technique;
	int rpShaderIndex;
	nShaderParams shaderParams;
	nFixedArray<nString> renderTargetNames;
	int clearFlags;
	vector4 clearColor;
	float clearDepth;
	int clearStencil;
	nVariableContext varContext;

public:

	CPass():
		pFrameShader(NULL),
		rpShaderIndex(-1),
		clearFlags(0),
		clearColor(0.0f, 0.0f, 0.0f, 1.0f),
		clearDepth(1.0f),
		clearStencil(0),
		renderTargetNames(4 /*nGfxServer2::MaxRenderTargets*/) {}


	virtual ~CPass() {}

	virtual void Render() = 0;

	//!!!OLD!
	virtual void Validate();
};

typedef Ptr<CPass> PPass;

}

#endif
