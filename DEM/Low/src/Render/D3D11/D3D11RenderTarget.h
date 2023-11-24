#pragma once
#include <Render/RenderTarget.h>

// D3D11 implementation of a render target

struct ID3D11RenderTargetView;
struct ID3D11ShaderResourceView;

namespace Render
{
typedef Ptr<class CD3D11Texture> PD3D11Texture;

class CD3D11RenderTarget: public CRenderTarget
{
	FACTORY_CLASS_DECL;

protected:

	ID3D11RenderTargetView* pRTView = nullptr;
	PD3D11Texture           Texture;

	void					InternalDestroy();

public:

	virtual ~CD3D11RenderTarget() override { InternalDestroy(); }

	bool					Create(ID3D11RenderTargetView* pRTV, ID3D11ShaderResourceView* pSRV); // For internal use
	virtual void			Destroy() { InternalDestroy(); }
	virtual bool			IsValid() const { return !!pRTView; }
	virtual bool			CopyResolveToTexture(PTexture Dest /*, region*/) const;
	virtual CTexture*		GetShaderResource() const;
	virtual void            SetDebugName(std::string_view Name) override;
	ID3D11RenderTargetView*	GetD3DRTView() const { return pRTView; }
};

typedef Ptr<CD3D11RenderTarget> PD3D11RenderTarget;

}
