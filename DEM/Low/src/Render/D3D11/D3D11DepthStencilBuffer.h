#pragma once
#include <Render/DepthStencilBuffer.h>

// D3D11 implementation of depth-stencil buffer

struct ID3D11DepthStencilView;
struct ID3D11ShaderResourceView;

namespace Render
{
typedef Ptr<class CD3D11Texture> PD3D11Texture;

class CD3D11DepthStencilBuffer: public CDepthStencilBuffer
{
	FACTORY_CLASS_DECL;

protected:

	ID3D11DepthStencilView* pDSView;
	PD3D11Texture			Texture;

	void					InternalDestroy();

public:

	CD3D11DepthStencilBuffer(): pDSView(nullptr) {}
	virtual ~CD3D11DepthStencilBuffer() { InternalDestroy(); }

	bool					Create(ID3D11DepthStencilView* pDSV, ID3D11ShaderResourceView* pSRV); // For internal use
	virtual void			Destroy() { InternalDestroy(); }
	virtual bool			IsValid() const { return !!pDSView; }
	virtual CTexture*		GetShaderResource() const;
	virtual void            SetDebugName(std::string_view Name) override;
	ID3D11DepthStencilView*	GetD3DDSView() const { return pDSView; }
};

typedef Ptr<CD3D11DepthStencilBuffer> PD3D11DepthStencilBuffer;

}
