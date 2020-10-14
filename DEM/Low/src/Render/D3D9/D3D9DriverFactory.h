#pragma once
#include <Render/VideoDriverFactory.h>

// D3D9 implementation of CVideoDriverFactory

struct IDirect3D9;
typedef enum _D3DFORMAT D3DFORMAT;
typedef enum _D3DMULTISAMPLE_TYPE D3DMULTISAMPLE_TYPE;

namespace Render
{

class CD3D9DriverFactory: public CVideoDriverFactory
{
	RTTI_CLASS_DECL(Render::CD3D9DriverFactory, Render::CVideoDriverFactory);

protected:

	IDirect3D9* pD3D9 = nullptr;
	UPTR        AdapterCount = 0;	// Valid during a lifetime of the D3D9 object

public:

	virtual ~CD3D9DriverFactory() override { if (IsOpened()) Release(); }

	static D3DFORMAT		PixelFormatToD3DFormat(EPixelFormat Format);
	static EPixelFormat		D3DFormatToPixelFormat(D3DFORMAT D3DFormat);
	static UPTR				D3DFormatBitsPerPixel(D3DFORMAT D3DFormat);
	static UPTR				D3DFormatBlockSize(D3DFORMAT D3DFormat);
	static UPTR				D3DFormatDepthBits(D3DFORMAT D3DFormat);
	static UPTR				D3DFormatStencilBits(D3DFORMAT D3DFormat);
	static EMSAAQuality		D3DMSAAParamsToMSAAQuality(D3DMULTISAMPLE_TYPE MultiSampleType, UPTR MultiSampleQuality);

	virtual bool			Create() override;
	virtual void			Release() override;
	bool					IsOpened() const { return !!pD3D9; }

	virtual bool			AdapterExists(UPTR Adapter) const { return Adapter < AdapterCount; }
	virtual UPTR			GetAdapterCount() const { return AdapterCount; }
	virtual bool			GetAdapterInfo(UPTR Adapter, CAdapterInfo& OutInfo) const;
	virtual UPTR			GetAdapterOutputCount(UPTR Adapter) const { return 1; }
	virtual PDisplayDriver	CreateDisplayDriver(UPTR Adapter = 0, UPTR Output = 0);
	//!!!need swap chain params!
	virtual PGPUDriver		CreateGPUDriver(UPTR Adapter = Adapter_AutoSelect, EGPUDriverType DriverType = GPU_AutoSelect);

	IDirect3D9*				GetDirect3D9() const { return pD3D9; }
};

typedef Ptr<CD3D9DriverFactory> PD3D9DriverFactory;

}
