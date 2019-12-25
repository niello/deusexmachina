#pragma once
#include <Core/Object.h>
#include <Render/RenderFwd.h>

// Central object to enumerate video hardware and obtain driver objects to control it.
// This class should be subclassed and implemented via some video subsystem API like D3D9 or DXGI.
// Typical use case is:
// Create factory -> Enumerate video adapters -> Create display driver for some adapter output ->
// -> Set display mode and other parameters
// When CreateGPUDriver() is called, both Adapter and DriverType may be specified or AutoSelect.
// Behaviour may vary for different implementations. Basic guidelines are:
// If both are specified, creation succeeds only if an Adapter supports requested DriverType.
// If DriverType only is auto, default DriverType of the Adapter is used.
// If Adapter only is auto, the first Adapter that supports requested DriverType is used.
// If both are auto, system will try to create hardware driver if possible, falling back to reference one.

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
public:

	struct CAdapterInfo
	{
		std::string Description;
		U32         VendorID;
		U32         DeviceID;
		U32         SubSysID;
		U32         Revision;
		U64         VideoMemBytes;
		U64         DedicatedSystemMemBytes;
		U64         SharedSystemMemBytes;
		bool        IsSoftware;
	};

	virtual bool			Create() = 0;
	virtual void			Release() = 0;
	virtual bool			AdapterExists(UPTR Adapter) const = 0;
	virtual UPTR			GetAdapterCount() const = 0;
	virtual bool			GetAdapterInfo(UPTR Adapter, CAdapterInfo& OutInfo) const = 0;
	virtual UPTR			GetAdapterOutputCount(UPTR Adapter) const = 0;
	virtual PDisplayDriver	CreateDisplayDriver(UPTR Adapter = 0, UPTR Output = 0) = 0;
	virtual PGPUDriver		CreateGPUDriver(UPTR Adapter = Adapter_AutoSelect, EGPUDriverType DriverType = GPU_AutoSelect) = 0;
};

typedef Ptr<CVideoDriverFactory> PVideoDriverFactory;

}
