#pragma once
#ifndef __DEM_L1_RENDER_D3D9_DRIVER_FACTORY_H__
#define __DEM_L1_RENDER_D3D9_DRIVER_FACTORY_H__

#include <Render/VideoDriverFactory.h>
#include <Data/Singleton.h>
#include <Data/HandleManager.h>

// Central object to enumerate video hardware and obtain driver objects to control it.
// This class should be subclassed and implemented via some video subsystem API like D3D9 or DXGI.

struct IDirect3D9;
typedef enum _D3DFORMAT D3DFORMAT;
typedef enum _D3DMULTISAMPLE_TYPE D3DMULTISAMPLE_TYPE;

namespace Render
{
#define D3D9DrvFactory Render::CD3D9DriverFactory::Instance()

class CD3D9DriverFactory: public CVideoDriverFactory
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CD3D9DriverFactory);

protected:

	IDirect3D9*		pD3D9;
	Sys::COSWindow*	pFocusWindow;	// Focus window for fullscreen responses, see device creation
	DWORD			AdapterCount;	// Valid during a lifetime of the D3D9 object

public:

	Data::CHandleManager	HandleMgr;			// Primarily for shader metadata handles

	CD3D9DriverFactory(): pD3D9(NULL), AdapterCount(0) { __ConstructSingleton; }
	virtual ~CD3D9DriverFactory() { if (IsOpened()) Close(); __DestructSingleton; }

	static D3DFORMAT		PixelFormatToD3DFormat(EPixelFormat Format);
	static EPixelFormat		D3DFormatToPixelFormat(D3DFORMAT D3DFormat);
	static DWORD			D3DFormatBitsPerPixel(D3DFORMAT D3DFormat);
	static DWORD			D3DFormatBlockSize(D3DFORMAT D3DFormat);
	static DWORD			D3DFormatDepthBits(D3DFORMAT D3DFormat);
	static DWORD			D3DFormatStencilBits(D3DFORMAT D3DFormat);
	static EMSAAQuality		D3DMSAAParamsToMSAAQuality(D3DMULTISAMPLE_TYPE MultiSampleType, DWORD MultiSampleQuality);

	bool					Open(Sys::COSWindow* pWindow);
	void					Close();
	bool					IsOpened() const { return !!pD3D9; }

	virtual bool			AdapterExists(DWORD Adapter) const { return Adapter < AdapterCount; }
	virtual DWORD			GetAdapterCount() const { return AdapterCount; }
	virtual bool			GetAdapterInfo(DWORD Adapter, CAdapterInfo& OutInfo) const;
	virtual DWORD			GetAdapterOutputCount(DWORD Adapter) const { return 1; }
	virtual PDisplayDriver	CreateDisplayDriver(DWORD Adapter = 0, DWORD Output = 0);
	//!!!need swap chain params!
	virtual PGPUDriver		CreateGPUDriver(DWORD Adapter = Adapter_AutoSelect, EGPUDriverType DriverType = GPU_AutoSelect);

	IDirect3D9*				GetDirect3D9() const { return pD3D9; }
	Sys::COSWindow*			GetFocusWindow() const { return pFocusWindow; }
};

typedef Ptr<CD3D9DriverFactory> PD3D9DriverFactory;

}

#endif
