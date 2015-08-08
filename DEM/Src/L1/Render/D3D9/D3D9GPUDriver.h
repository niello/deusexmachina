#pragma once
#ifndef __DEM_L1_RENDER_D3D9_GPU_DRIVER_H__
#define __DEM_L1_RENDER_D3D9_GPU_DRIVER_H__

#include <Render/GPUDriver.h>
#include <Render/D3D9/D3D9SwapChain.h>
#include <Data/FixedArray.h>

#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h> // At least for a CAPS structure

// Direct3D9 GPU device driver.
// Multihead (multimonitor) feature is not implemented. You may do it by yourself.
// NB: D3D9 device can't be created without a swap chain, so you MUST call CreateSwapChain()
// before using any device-dependent methods.

//!!!lost and reset devices!

namespace Render
{
typedef Ptr<class CD3D9VertexLayout> PD3D9VertexLayout;
typedef Ptr<class CD3D9VertexBuffer> PD3D9VertexBuffer;
typedef Ptr<class CD3D9IndexBuffer> PD3D9IndexBuffer;
typedef Ptr<class CD3D9RenderTarget> PD3D9RenderTarget;
typedef Ptr<class CD3D9DepthStencilBuffer> PD3D9DepthStencilBuffer;
typedef Ptr<class CD3D9RenderState> PD3D9RenderState;
typedef Ptr<class CD3D9Sampler> PD3D9Sampler;
typedef Ptr<class CD3D9Texture> PD3D9Texture;

class CD3D9GPUDriver: public CGPUDriver
{
	__DeclareClass(CD3D9GPUDriver);

protected:

	static const DWORD SM30_PS_SamplerCount = 16;
	static const DWORD SM30_VS_SamplerCount = 4;

	PD3D9VertexLayout					CurrVL;
	CFixedArray<PD3D9VertexBuffer>		CurrVB;
	CFixedArray<DWORD>					CurrVBOffset;
	PD3D9IndexBuffer					CurrIB;
	CFixedArray<PD3D9RenderTarget>		CurrRT;
	PD3D9DepthStencilBuffer				CurrDS;
	PD3D9RenderState					CurrRS;
	CFixedArray<PD3D9Sampler>			CurrSS; // Pixel, then vertex
	CFixedArray<PD3D9Texture>			CurrTex; // Pixel, then vertex

	CArray<CD3D9SwapChain>				SwapChains;
	CDict<CStrID, PD3D9VertexLayout>	VertexLayouts;
	CArray<PD3D9RenderState>			RenderStates;
	PD3D9RenderState					DefaultRenderState; //!!!destroy along with RenderStates[]!
	CArray<PD3D9Sampler>				Samplers;
	PD3D9Sampler						DefaultSampler; //!!!destroy along with Samplers[]!
	bool								IsInsideFrame;
	//bool								Wireframe;

	D3DCAPS9							D3DCaps;
	IDirect3DDevice9*					pD3DDevice;

	Events::PSub						Sub_OnPaint; // Fullscreen-only, so only one swap chain will be subscribed

	CD3D9GPUDriver();

	// Events are received from swap chain windows, so subscriptions are in swap chains
	bool						OnOSWindowToggleFullscreen(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool						OnOSWindowSizeChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool						OnOSWindowPaint(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool						OnOSWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

	bool						CreateD3DDevice(DWORD CurrAdapterID, EGPUDriverType CurrDriverType, D3DPRESENT_PARAMETERS D3DPresentParams);
	void						SetDefaultRenderState();
	void						SetDefaultSamplers();
	bool						InitSwapChainRenderTarget(CD3D9SwapChain& SC);
	bool						Reset(D3DPRESENT_PARAMETERS& D3DPresentParams, DWORD TargetSwapChainID);
	void						Release();

	void						FillD3DPresentParams(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, const Sys::COSWindow* pWindow, D3DPRESENT_PARAMETERS& D3DPresentParams) const;
	bool						GetCurrD3DPresentParams(const CD3D9SwapChain& SC, D3DPRESENT_PARAMETERS& D3DPresentParams) const;
	static D3DDEVTYPE			GetD3DDriverType(EGPUDriverType DriverType);
	static void					GetUsagePool(DWORD InAccessFlags, DWORD& OutUsage, D3DPOOL& OutPool);
	static UINT					GetD3DLockFlags(EResourceMapMode MapMode);
	static D3DCUBEMAP_FACES		GetD3DCubeMapFace(ECubeMapFace Face);
	static D3DCMPFUNC			GetD3DCmpFunc(ECmpFunc Func);
	static D3DSTENCILOP			GetD3DStencilOp(EStencilOp Operation);
	static D3DBLEND				GetD3DBlendArg(EBlendArg Arg);
	static D3DBLENDOP			GetD3DBlendOp(EBlendOp Operation);
	static D3DTEXTUREADDRESS	GetD3DTexAddressMode(ETexAddressMode Mode);
	static void					GetD3DTexFilter(ETexFilter Filter, D3DTEXTUREFILTERTYPE& OutMin, D3DTEXTUREFILTERTYPE& OutMag, D3DTEXTUREFILTERTYPE& OutMip);

	friend class CD3D9DriverFactory;

public:

	virtual ~CD3D9GPUDriver() { Release(); }

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
	virtual bool				CaptureScreenshot(DWORD SwapChainID, IO::CStream& OutStream) const;

	virtual PVertexLayout		CreateVertexLayout(const CVertexComponent* pComponents, DWORD Count);
	virtual PVertexBuffer		CreateVertexBuffer(CVertexLayout& VertexLayout, DWORD VertexCount, DWORD AccessFlags, const void* pData = NULL);
	virtual PIndexBuffer		CreateIndexBuffer(EIndexType IndexType, DWORD IndexCount, DWORD AccessFlags, const void* pData = NULL);
	virtual PRenderState		CreateRenderState(const CRenderStateDesc& Desc);
	//???CreateShader? instead of D3D9 API code in loaders?
	virtual PConstantBuffer		CreateConstantBuffer(const CShader& Shader, CStrID ID, DWORD AccessFlags, const void* pData = NULL);
	virtual PTexture			CreateTexture(const CTextureDesc& Desc, DWORD AccessFlags, const void* pData = NULL, bool MipDataProvided = false);
	virtual PSampler			CreateSampler(const CSamplerDesc& Desc);
	virtual PRenderTarget		CreateRenderTarget(const CRenderTargetDesc& Desc);
	virtual PDepthStencilBuffer	CreateDepthStencilBuffer(const CRenderTargetDesc& Desc);

	virtual bool				SetViewport(DWORD Index, const CViewport* pViewport); // NULL to reset
	virtual bool				GetViewport(DWORD Index, CViewport& OutViewport);
	virtual bool				SetScissorRect(DWORD Index, const Data::CRect* pScissorRect); // NULL to reset
	virtual bool				GetScissorRect(DWORD Index, Data::CRect& OutScissorRect);

	virtual bool				SetVertexLayout(CVertexLayout* pVLayout);
	virtual bool				SetVertexBuffer(DWORD Index, CVertexBuffer* pVB, DWORD OffsetVertex = 0);
	virtual bool				SetIndexBuffer(CIndexBuffer* pIB);
	//virtual bool				SetInstanceBuffer(DWORD Index, CVertexBuffer* pVB, DWORD Instances, DWORD OffsetVertex = 0);
	virtual bool				SetRenderState(CRenderState* pState);
	virtual bool				SetRenderTarget(DWORD Index, CRenderTarget* pRT);
	virtual bool				SetDepthStencilBuffer(CDepthStencilBuffer* pDS);

	virtual bool				BindConstantBuffer(EShaderType ShaderType, HConstBuffer Handle, CConstantBuffer* pCBuffer);
	virtual bool				BindResource(EShaderType ShaderType, HResource Handle, CTexture* pResource);
	virtual bool				BindSampler(EShaderType ShaderType, HSampler Handle, CSampler* pSampler);

	virtual bool				BeginFrame();
	virtual void				EndFrame();
	virtual void				Clear(DWORD Flags, const vector4& ColorRGBA, float Depth, uchar Stencil);
	virtual void				ClearRenderTarget(CRenderTarget& RT, const vector4& ColorRGBA);
	virtual bool				Draw(const CPrimitiveGroup& PrimGroup);

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

	//void						SetWireframe(bool Wire);
	//bool						IsWireframe() const { return Wireframe; }

	bool						GetD3DMSAAParams(EMSAAQuality MSAA, D3DFORMAT Format, D3DMULTISAMPLE_TYPE& OutType, DWORD& OutQuality) const;
	IDirect3DDevice9*			GetD3DDevice() const { return pD3DDevice; }
};

typedef Ptr<CD3D9GPUDriver> PD3D9GPUDriver;

inline CD3D9GPUDriver::CD3D9GPUDriver(): SwapChains(1, 1), pD3DDevice(NULL), IsInsideFrame(false)
{
	//???CurrVBOffset.SetSize()?
	//memset(CurrVBOffset.GetPtr(), 0, sizeof(DWORD) * CurrVBOffset.GetCount());
}
//---------------------------------------------------------------------

}

#endif
