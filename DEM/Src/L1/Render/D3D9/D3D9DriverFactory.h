#pragma once
#ifndef __DEM_L1_RENDER_D3D9_DRIVER_FACTORY_H__
#define __DEM_L1_RENDER_D3D9_DRIVER_FACTORY_H__

#include <Render/VideoDriverFactory.h>

// Central object to enumerate video hardware and obtain driver objects to control it.
// This class should be subclassed and implemented via some video subsystem API like D3D9 or DXGI.

struct IDirect3D9;

namespace Render
{

class CD3D9DriverFactory: public CVideoDriverFactory
{
	__DeclareClassNoFactory;

protected:

	IDirect3D9* pD3D9;
	DWORD		AdapterCount;	// Valid during a lifetime of the D3D9 object

	//???in base class?
	//store info about each adapter

public:

	CD3D9DriverFactory(): pD3D9(NULL), AdapterCount(0) {}
	virtual ~CD3D9DriverFactory() { if (IsOpened()) Close(); }

	bool					Open();
	void					Close();
	bool					IsOpened() const { return !!pD3D9; }

	virtual DWORD			GetAdapterCount() const { return AdapterCount; }
	virtual bool			AdapterExists(DWORD Adapter) const { return Adapter < AdapterCount; }
	virtual bool			GetAdapterInfo(DWORD Adapter, CAdapterInfo& OutInfo) const;
	virtual DWORD			GetAdapterOutputCount(DWORD Adapter) const { return 1; }
	virtual PDisplayDriver	CreateDisplayDriver(DWORD Adapter = 0, DWORD Output = 0) = 0;
	//virtual CGPUDriver*		CreateGPUDriver(CDisplayDriver* pDisplay) = 0;
};

}

#endif
