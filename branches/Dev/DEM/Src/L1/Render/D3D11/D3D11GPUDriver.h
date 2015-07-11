#pragma once
#ifndef __DEM_L1_RENDER_D3D11_GPU_DRIVER_H__
#define __DEM_L1_RENDER_D3D11_GPU_DRIVER_H__

#include <Render/GPUDriver.h>
#include <Render/D3D11/D3D11SwapChain.h>
#include <Data/FixedArray.h>
#include <Data/Buffer.h>

// Direct3D11 GPU device driver.

struct IDXGISwapChain;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct D3D11_VIEWPORT;
typedef enum D3D_DRIVER_TYPE D3D_DRIVER_TYPE;
enum D3D11_USAGE;
enum D3D11_MAP;
typedef struct tagRECT RECT;

namespace Render
{
typedef Ptr<class CD3D11VertexLayout> PD3D11VertexLayout;
typedef Ptr<class CD3D11VertexBuffer> PD3D11VertexBuffer;
typedef Ptr<class CD3D11IndexBuffer> PD3D11IndexBuffer;
typedef Ptr<class CD3D11RenderTarget> PD3D11RenderTarget;
typedef Ptr<class CD3D11DepthStencilBuffer> PD3D11DepthStencilBuffer;
typedef Ptr<class CD3D11RenderState> PD3D11RenderState;

class CD3D11GPUDriver: public CGPUDriver
{
	__DeclareClass(CD3D11GPUDriver);

public:

	enum
	{
		GPU_Dirty_VL = 0x0001,		// Vertex layout
		GPU_Dirty_VB = 0x0002,		// Vertex buffer(s)
		GPU_Dirty_IB = 0x0004,		// Index buffer
		GPU_Dirty_RT = 0x0010,		// Render target(s)
		GPU_Dirty_DS = 0x0020,		// Depth-stencil buffer
		GPU_Dirty_VP = 0x0040,		// Viewport(s)
		GPU_Dirty_SR = 0x0080,		// Scissor rect(s)

		GPU_Dirty_All = 0xffffffff	// All bits set, for convenience in ApplyChanges() call
	};

protected:

	Data::CFlags						CurrDirtyFlags;
	PD3D11VertexLayout					CurrVL;
	ID3D11InputLayout*					pCurrIL;
	CFixedArray<PD3D11VertexBuffer>		CurrVB;
	CFixedArray<DWORD>					CurrVBOffset;
	PD3D11IndexBuffer					CurrIB;
	EPrimitiveTopology					CurrPT;
	CFixedArray<PD3D11RenderTarget>		CurrRT;
	PD3D11DepthStencilBuffer			CurrDS;
	D3D11_VIEWPORT*						CurrVP;
	RECT*								CurrSR;				//???SR corresp to VP, mb set in pairs and use all 32 bits each for a pair?
	DWORD								MaxViewportCount;
	Data::CFlags						VPSRSetFlags;		// 16 low bits indicate whether VP is set or not, same for SR in 16 high bits
	static const DWORD					VP_OR_SR_SET_FLAG_COUNT = 16;

	CArray<CD3D11SwapChain>				SwapChains;
	CDict<CStrID, PD3D11VertexLayout>	VertexLayouts;
	CDict<CStrID, Data::CBuffer>		ShaderInputSignatures;
	CArray<PD3D11RenderState>			RenderStates;
	//bool								IsInsideFrame;
	//bool								Wireframe;

	ID3D11Device*						pD3DDevice;
	ID3D11DeviceContext*				pD3DImmContext;
	//???store also D3D11.1 interfaces? and use for 11.1 methods only.

	CD3D11GPUDriver();

	bool			OnOSWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnOSWindowSizeChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnOSWindowToggleFullscreen(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	//bool			OnOSWindowPaint(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

	bool			InitSwapChainRenderTarget(CD3D11SwapChain& SC);
	void			Release();

	static D3D_DRIVER_TYPE	GetD3DDriverType(EGPUDriverType DriverType);
	static EGPUDriverType	GetDEMDriverType(D3D_DRIVER_TYPE DriverType);
	static void				GetUsageAccess(DWORD InAccessFlags, bool InitDataProvided, D3D11_USAGE& OutUsage, UINT& OutCPUAccess);
	static void				GetD3DMapTypeAndFlags(EResourceMapMode MapMode, D3D11_MAP& OutMapType, UINT& OutMapFlags);

	ID3D11InputLayout*		GetD3DInputLayout(CD3D11VertexLayout& VertexLayout, CStrID ShaderInputSignatureID, const Data::CBuffer* pSignature = NULL);
	bool					ReadFromD3DBuffer(void* pDest, ID3D11Buffer* pBuf, D3D11_USAGE Usage, DWORD BufferSize, DWORD Size, DWORD Offset);
	bool					WriteToD3DBuffer(ID3D11Buffer* pBuf, D3D11_USAGE Usage, DWORD BufferSize, const void* pData, DWORD Size, DWORD Offset);

	friend class CD3D11DriverFactory;

public:

	virtual ~CD3D11GPUDriver() { Release(); }

	virtual bool				Init(DWORD AdapterNumber, EGPUDriverType DriverType);
	virtual bool				CheckCaps(ECaps Cap);
	virtual DWORD				GetMaxVertexStreams();
	virtual DWORD				GetMaxTextureSize(ETextureType Type);
	virtual DWORD				GetMaxMultipleRenderTargetCount() { return CurrRT.GetCount(); }

	virtual int					CreateSwapChain(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, Sys::COSWindow* pWindow);
	virtual bool				DestroySwapChain(DWORD SwapChainID);
	virtual bool				SwapChainExists(DWORD SwapChainID) const;
	virtual bool				ResizeSwapChain(DWORD SwapChainID, unsigned int Width, unsigned int Height);
	virtual bool				SwitchToFullscreen(DWORD SwapChainID, CDisplayDriver* pDisplay = NULL, const CDisplayMode* pMode = NULL);
	virtual bool				SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect = NULL);
	virtual bool				IsFullscreen(DWORD SwapChainID) const;
	virtual PRenderTarget		GetSwapChainRenderTarget(DWORD SwapChainID) const;
	virtual bool				Present(DWORD SwapChainID);
	virtual bool				WriteScreenshot(DWORD SwapChainID, IO::CStream& OutStream) const;

	virtual bool				SetViewport(DWORD Index, const CViewport* pViewport); // NULL to reset
	virtual bool				GetViewport(DWORD Index, CViewport& OutViewport);
	virtual bool				SetScissorRect(DWORD Index, const Data::CRect* pScissorRect); // NULL to reset
	virtual bool				GetScissorRect(DWORD Index, Data::CRect& OutScissorRect);

	virtual bool				BeginFrame();
	virtual void				EndFrame();
	virtual bool				SetVertexLayout(CVertexLayout* pVLayout);
	virtual bool				SetVertexBuffer(DWORD Index, CVertexBuffer* pVB, DWORD OffsetVertex = 0);
	virtual bool				SetIndexBuffer(CIndexBuffer* pIB);
	//virtual bool				SetInstanceBuffer(DWORD Index, CVertexBuffer* pVB, DWORD Instances, DWORD OffsetVertex = 0);
	virtual bool				SetRenderTarget(DWORD Index, CRenderTarget* pRT);
	virtual bool				SetDepthStencilBuffer(CDepthStencilBuffer* pDS);
	virtual void				Clear(DWORD Flags, const vector4& ColorRGBA, float Depth, uchar Stencil);
	virtual void				ClearRenderTarget(CRenderTarget& RT, const vector4& ColorRGBA);
	virtual bool				Draw(const CPrimitiveGroup& PrimGroup);

	//???virtual to unify interface? no-op where is not applicable. or only apply on draw etc here?
	DWORD						ApplyChanges(DWORD ChangesToUpdate = GPU_Dirty_All); // returns a combination of dirty flags where errors occured

	//!!!D3D11 vertex layout must be a key to a list of different layout interfaces, each for a particular shader input signature!
	//it will be resolved at a draw call, when both shader and VB are bound
	//???!!!support multichannel VBs?!
	virtual PVertexLayout		CreateVertexLayout(const CVertexComponent* pComponents, DWORD Count);
	virtual PVertexBuffer		CreateVertexBuffer(CVertexLayout& VertexLayout, DWORD VertexCount, DWORD AccessFlags, const void* pData = NULL);
	virtual PIndexBuffer		CreateIndexBuffer(EIndexType IndexType, DWORD IndexCount, DWORD AccessFlags, const void* pData = NULL);
	virtual PTexture			CreateTexture(const CTextureDesc& Desc, DWORD AccessFlags, const void* pData = NULL, bool MipDataProvided = false);
	virtual PRenderTarget		CreateRenderTarget(const CRenderTargetDesc& Desc);
	virtual PDepthStencilBuffer	CreateDepthStencilBuffer(const CRenderTargetDesc& Desc);
	virtual PRenderState		CreateRenderState(const Data::CParams& Desc);

	virtual bool				MapResource(void** ppOutData, const CVertexBuffer& Resource, EResourceMapMode Mode);
	virtual bool				MapResource(void** ppOutData, const CIndexBuffer& Resource, EResourceMapMode Mode);
	virtual bool				MapResource(CImageData& OutData, const CTexture& Resource, EResourceMapMode Mode, DWORD ArraySlice = 0, DWORD MipLevel = 0);
	virtual bool				UnmapResource(const CVertexBuffer& Resource);
	virtual bool				UnmapResource(const CIndexBuffer& Resource);
	virtual bool				UnmapResource(const CTexture& Resource, DWORD ArraySlice = 0, DWORD MipLevel = 0);
	virtual bool				ReadFromResource(void* pDest, const CVertexBuffer& Resource, DWORD Size = 0, DWORD Offset = 0);
	virtual bool				ReadFromResource(void* pDest, const CIndexBuffer& Resource, DWORD Size = 0, DWORD Offset = 0);
	virtual bool				ReadFromResource(const CImageData& Dest, const CTexture& Resource, DWORD ArraySlice = 0, DWORD MipLevel = 0, const Data::CBox* pRegion = NULL);
	virtual bool				WriteToResource(CVertexBuffer& Resource, const void* pData, DWORD Size = 0, DWORD Offset = 0);
	virtual bool				WriteToResource(CIndexBuffer& Resource, const void* pData, DWORD Size = 0, DWORD Offset = 0);
	virtual bool				WriteToResource(CTexture& Resource, const CImageData& SrcData, DWORD ArraySlice = 0, DWORD MipLevel = 0, const Data::CBox* pRegion = NULL);

	//void					SetWireframe(bool Wire);
	//bool					IsWireframe() const { return Wireframe; }

	//IDirect3DDevice9*		GetD3DDevice() const { return pD3DDevice; }
};

typedef Ptr<CD3D11GPUDriver> PD3D11GPUDriver;

inline CD3D11GPUDriver::CD3D11GPUDriver():
	SwapChains(1, 1),
	pD3DDevice(NULL),
	pD3DImmContext(NULL),
	pCurrIL(NULL),
	CurrPT(Prim_Invalid),
	CurrVP(NULL),
	CurrSR(NULL),
	MaxViewportCount(0) /*, IsInsideFrame(false)*/
{
}
//---------------------------------------------------------------------

}

#endif
