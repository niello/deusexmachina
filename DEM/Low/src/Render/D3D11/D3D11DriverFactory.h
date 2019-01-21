#pragma once
#ifndef __DEM_L1_RENDER_D3D11_DRIVER_FACTORY_H__
#define __DEM_L1_RENDER_D3D11_DRIVER_FACTORY_H__

#include <Render/VideoDriverFactory.h>
#include <Data/Singleton.h>
#include <Data/HandleManager.h>
#include <Data/HashTable.h>
#include <Data/Buffer.h>
#include <vector>

// Direct3D 11 and DXGI 1.1 implementation of CVideoDriverFactory

//???rename to DXGI[version]DriverFactory?

struct IDXGIFactory1;
struct IDXGIOutput;
struct DXGI_SAMPLE_DESC;
enum DXGI_FORMAT;

namespace Data
{
	typedef std::unique_ptr<class CBuffer> PBuffer;
}

namespace Render
{

enum EFormatType
{
	FmtType_Typeless,
	FmtType_DefaultTyped,
	FmtType_UInt,
	FmtType_UNorm,
	FmtType_SInt,
	FmtType_SNorm,
	FmtType_Float
};

#define D3D11DrvFactory Render::CD3D11DriverFactory::Instance()

class CD3D11DriverFactory: public CVideoDriverFactory
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CD3D11DriverFactory);

protected:

	IDXGIFactory1*				pDXGIFactory = nullptr;
	UPTR						AdapterCount = 0;		// Valid during a lifetime of the DXGI factory object
	std::vector<Data::CBuffer>	ShaderSignatures;
	CHashTable<U32, UPTR>		ShaderSigIDToIndex;

public:

	CD3D11DriverFactory() { __ConstructSingleton; }
	virtual ~CD3D11DriverFactory() { if (IsOpened()) Release(); __DestructSingleton; }

	static DXGI_FORMAT		PixelFormatToDXGIFormat(EPixelFormat Format);
	static EPixelFormat		DXGIFormatToPixelFormat(DXGI_FORMAT D3DFormat);
	static UPTR				DXGIFormatBitsPerPixel(DXGI_FORMAT D3DFormat);
	static UPTR				DXGIFormatDepthBits(DXGI_FORMAT D3DFormat);
	static UPTR				DXGIFormatStencilBits(DXGI_FORMAT D3DFormat);
	static UPTR				DXGIFormatBlockSize(DXGI_FORMAT D3DFormat);
	static DXGI_FORMAT		GetCorrespondingFormat(DXGI_FORMAT D3DFormat, EFormatType Type, bool PreferSRGB = false);
	static EMSAAQuality		D3DMSAAParamsToMSAAQuality(DXGI_SAMPLE_DESC SampleDesc);

	virtual bool			Create() override;
	virtual void			Release() override;
	bool					IsOpened() const { return !!pDXGIFactory; }

	virtual bool			AdapterExists(UPTR Adapter) const { return Adapter < AdapterCount; }
	virtual UPTR			GetAdapterCount() const { return AdapterCount; }
	virtual bool			GetAdapterInfo(UPTR Adapter, CAdapterInfo& OutInfo) const;
	virtual UPTR			GetAdapterOutputCount(UPTR Adapter) const;
	virtual PDisplayDriver	CreateDisplayDriver(UPTR Adapter = 0, UPTR Output = 0);
	PDisplayDriver			CreateDisplayDriver(IDXGIOutput* pOutput);
	virtual PGPUDriver		CreateGPUDriver(UPTR Adapter = Adapter_AutoSelect, EGPUDriverType DriverType = GPU_AutoSelect);

	bool					RegisterShaderInputSignature(U32 ID, Data::CBuffer&& Data);
	const Data::CBuffer*	FindShaderInputSignature(U32 ID) const;

	IDXGIFactory1*			GetDXGIFactory() const { return pDXGIFactory; }
};

typedef Ptr<CD3D11DriverFactory> PD3D11DriverFactory;

}

#endif
