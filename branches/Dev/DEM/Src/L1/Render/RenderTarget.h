#pragma once
#ifndef __DEM_L1_RENDER_TARGET_H__
#define __DEM_L1_RENDER_TARGET_H__

#include <Core/Object.h>
#include <Render/RenderFwd.h>

// A surface in a video memory on which rendering is performed. It can be
// a back buffer or a texture. It optionally can be used as a shader input.

namespace Render
{
typedef Ptr<class CTexture> PTexture;

struct CRenderTargetDesc
{
	ushort			Width;
	ushort			Height;
	EPixelFormat	Format;
	EMSAAQuality	MSAAQuality;
	bool			UseAsShaderInput;
};

class CRenderTarget: public Core::CObject
{
protected:

	CRenderTargetDesc	Desc;

public:

	virtual CTexture*			GetShaderResource() const = 0; //???or store PTexture inside always and avoid virtualization here?
	//virtual bool CopyToTexture/ResolveToTexture(CTexture& Dest) const = 0; // copy to another size and/or MSAA
	const CRenderTargetDesc&	GetDesc() const { return Desc; }
};

typedef Ptr<CRenderTarget> PRenderTarget;

}

#endif
