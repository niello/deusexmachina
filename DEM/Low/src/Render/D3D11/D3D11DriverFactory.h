#pragma once
#include <Render/VideoDriverFactory.h>
#include <Data/HashTable.h>
#include <Data/Buffer.h>
#include <vector>

// Direct3D 11 and DXGI 1.1 implementation of CVideoDriverFactory

//???rename to DXGI[version]DriverFactory?

struct IDXGIFactory1;
struct IDXGIOutput;
#if DEM_RENDER_DEBUG
struct IDXGIDebug;
#endif
struct DXGI_SAMPLE_DESC;
enum DXGI_FORMAT;

namespace Data
{
	typedef std::unique_ptr<class CDataBuffer> PDataBuffer;
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

class CD3D11DriverFactory: public CVideoDriverFactory
{
	RTTI_CLASS_DECL;

protected:

	IDXGIFactory1*				pDXGIFactory = nullptr;
#if DEM_RENDER_DEBUG
	IDXGIDebug*					pDXGIDebug = nullptr;
#endif
	UPTR						AdapterCount = 0;		// Valid during a lifetime of the DXGI factory object
	std::vector<Data::CDataBuffer>	ShaderSignatures;
	CHashTable<U32, UPTR>		ShaderSigIDToIndex;

public:

	virtual ~CD3D11DriverFactory() override { if (IsOpened()) Release(); }

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

	bool					RegisterShaderInputSignature(U32 ID, Data::CDataBuffer&& Data);
	const Data::CDataBuffer*	FindShaderInputSignature(U32 ID) const;

	IDXGIFactory1*			GetDXGIFactory() const { return pDXGIFactory; }
};

typedef Ptr<CD3D11DriverFactory> PD3D11DriverFactory;

}
