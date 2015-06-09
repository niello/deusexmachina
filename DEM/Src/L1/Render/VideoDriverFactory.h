#pragma once
#ifndef __DEM_L1_RENDER_VIDEO_DRIVER_FACTORY_H__
#define __DEM_L1_RENDER_VIDEO_DRIVER_FACTORY_H__

#include <Core/Object.h>
#include <Render/RenderFwd.h>
#include <Data/SimpleString.h>

// Central object to enumerate video hardware and obtain driver objects to control it.
// This class should be subclassed and implemented via some video subsystem API like D3D9 or DXGI.
// Typical use case is:
// Create factory -> Enumerate video adapters -> Create display driver for some adapter output ->
// -> Set display mode and other parameters

//???is singleton? what if I want to have D3D9 and OpenGL driver factories simultaneously?
//???only D3D9 factory as a singleton? D3D9 must be only one.

namespace Sys
{
	class COSWindow;
}

namespace Render
{
typedef Ptr<class CDisplayDriver> PDisplayDriver;
typedef Ptr<class CGPUDriver> PGPUDriver;

class CVideoDriverFactory: public Core::CObject
{
	__DeclareClassNoFactory;

public:

	struct CAdapterInfo
	{
		Data::CSimpleString	Description;
		DWORD				VendorID;
		DWORD				DeviceID;
		DWORD				SubSysID;
		DWORD				Revision;
		QWORD				VideoMemBytes;
		QWORD				DedicatedSystemMemBytes;
		QWORD				SharedSystemMemBytes;
		bool				IsSoftware;
	};

protected:

	//???default/focus window? or D3D-only?

public:

	CVideoDriverFactory() { }
	virtual ~CVideoDriverFactory() { }

	virtual bool			AdapterExists(DWORD Adapter) const = 0;
	virtual DWORD			GetAdapterCount() const = 0;
	virtual bool			GetAdapterInfo(CAdapterInfo& OutInfo) const = 0;
	virtual DWORD			GetAdapterOutputCount(DWORD Adapter) const = 0;
	virtual PDisplayDriver	CreateDisplayDriver(DWORD Adapter = 0, DWORD Output = 0) = 0;
	virtual PGPUDriver		CreateGPUDriver(DWORD Adapter = 0) = 0;
};

}

#endif
