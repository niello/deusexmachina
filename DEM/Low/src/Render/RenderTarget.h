#pragma once
#ifndef __DEM_L1_RENDER_TARGET_H__
#define __DEM_L1_RENDER_TARGET_H__

#include <Core/Object.h>
#include <Render/RenderFwd.h>

// A surface in a video memory on which rendering is performed. It can be
// a back buffer or a texture. It optionally can be used as a shader input.
// If you want to manage RT texture as a named resource you should get texture
// with GetShaderResource() and then manually register it in a resource manager.

//PERF:
//!!!always clear MSAARTs before rendering, at least AMD GCN benefits from it!

namespace Render
{
typedef Ptr<class CTexture> PTexture;

class CRenderTarget: public Core::CObject
{
	__DeclareClassNoFactory;

protected:

	CRenderTargetDesc Desc;

public:

	virtual void				Destroy() = 0;
	virtual bool				IsValid() const = 0;
	virtual bool				CopyResolveToTexture(PTexture Dest /*, region*/) const = 0; // Copy to texture of another size and/or MSAA
	virtual CTexture*			GetShaderResource() const = 0;
	const CRenderTargetDesc&	GetDesc() const { return Desc; }
};

typedef Ptr<CRenderTarget> PRenderTarget;

}

#endif
