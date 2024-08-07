#pragma once
#include <Render/DepthStencilBuffer.h>

// D3D9 implementation of a depth-stencil buffer.
// Reading as a texture is currently unsupported though it can be added through INTZ.

struct IDirect3DSurface9;

namespace Render
{

class CD3D9DepthStencilBuffer: public CDepthStencilBuffer
{
	FACTORY_CLASS_DECL;

protected:

	IDirect3DSurface9* pDSSurface = nullptr;

	// Requested clear operation parameters. For some strange reason in D3D9 we can't
	// clear RT, clear DS, then set RT and set DS. Setting a new RT results in breaking
	// of the depth buffer in a such way that it must be cleared again. Until reasons of
	// this behaviour remain unknown, I will cache ClearDepthStencilBuffer() request and
	// delay actual clear to the moment of setting DS surface.
	U32					D3D9ClearFlags = 0;
	float				ZClearValue = 1.f;
	U8					StencilClearValue = 0;

	void				InternalDestroy();

	friend class CD3D9GPUDriver; // For delayed clearing only, not to pollute an interface with setters and getters

public:

	virtual ~CD3D9DepthStencilBuffer() override { InternalDestroy(); }

	bool				Create(IDirect3DSurface9* pSurface); // For internal use
	virtual void		Destroy() { InternalDestroy(); }
	virtual bool		IsValid() const { return !!pDSSurface; }
	virtual CTexture*	GetShaderResource() const { return nullptr; }
	virtual void        SetDebugName(std::string_view Name) override;
	IDirect3DSurface9*	GetD3DSurface() const { return pDSSurface; }
};

typedef Ptr<CD3D9DepthStencilBuffer> PD3D9DepthStencilBuffer;

}
