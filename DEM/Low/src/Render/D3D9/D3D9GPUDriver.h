#pragma once
#include <Render/GPUDriver.h>
#include <Render/D3D9/D3D9SwapChain.h>
#include <Data/FixedArray.h>
#include <Data/HashTable.h>
#include <System/Allocators/PoolAllocator.h>
#include "DEMD3D9.h" // At least for a CAPS structure

#undef min
#undef max

// Direct3D9 GPU device driver.
// Multihead (multimonitor) feature is not implemented. You may do it by yourself.
// NB: D3D9 device can't be created without a swap chain, so you MUST call CreateSwapChain()
// before using any device-dependent methods.

//!!!lost and reset devices!

namespace Render
{
typedef Ptr<class CD3D9DriverFactory> PD3D9DriverFactory;
typedef Ptr<class CD3D9VertexLayout> PD3D9VertexLayout;
typedef Ptr<class CD3D9VertexBuffer> PD3D9VertexBuffer;
typedef Ptr<class CD3D9IndexBuffer> PD3D9IndexBuffer;
typedef Ptr<class CD3D9RenderTarget> PD3D9RenderTarget;
typedef Ptr<class CD3D9DepthStencilBuffer> PD3D9DepthStencilBuffer;
typedef Ptr<class CD3D9RenderState> PD3D9RenderState;
typedef Ptr<class CD3D9ConstantBuffer> PD3D9ConstantBuffer;
typedef Ptr<class CD3D9Sampler> PD3D9Sampler;
typedef Ptr<class CD3D9Texture> PD3D9Texture;
class CSM30ConstantBufferParam;

class CD3D9GPUDriver: public CGPUDriver
{
	__DeclareClassNoFactory;

protected:

	friend class CD3D9DriverFactory;

	static const UPTR CB_Slot_Count = 4;				// Pseudoregisters for CB binding
	static const UPTR SM30_VS_Int4Count = 16;
	static const UPTR SM30_VS_BoolCount = 16;
	static const UPTR SM30_VS_SamplerCount = 4;
	static const UPTR SM30_PS_Float4Count = 224;
	static const UPTR SM30_PS_Int4Count = 16;
	static const UPTR SM30_PS_BoolCount = 16;
	static const UPTR SM30_PS_SamplerCount = 16;

	enum
	{
		CB_ApplyFloat4	= 0x01,
		CB_ApplyInt4	= 0x02,
		CB_ApplyBool	= 0x04,
		CB_ApplyAll		= (CB_ApplyFloat4 | CB_ApplyInt4 | CB_ApplyBool)
	};

	struct CVBRec
	{
		PD3D9VertexBuffer				VB;
		UPTR							Offset;
		UINT							Frequency;
	};

	struct CCBRec
	{
		PD3D9ConstantBuffer				CB;
		Data::CFlags					ApplyFlags;
		UPTR							NextRange;
		UPTR							CurrRangeOffset;
	};

	PD3D9VertexLayout					CurrVL;
	CFixedArray<CVBRec>					CurrVB;
	PD3D9IndexBuffer					CurrIB;
	CFixedArray<PD3D9RenderTarget>		CurrRT;
	PD3D9DepthStencilBuffer				CurrDS;
	PD3D9RenderState					CurrRS;
	CFixedArray<CCBRec>					CurrCB;		// CB_Slot_Count vertex, then CB_Slot_Count pixel
	CFixedArray<PD3D9Sampler>			CurrSS;		// Pixel, then vertex
	CFixedArray<PD3D9Texture>			CurrTex;	// Pixel, then vertex

	char*								pCurrShaderConsts = nullptr;	// Main aligned pointer, for deallocation, next ones are offset pointers
	float*								pCurrVSFloat4 = nullptr;		// D3DCaps.MaxVertexShaderConst * float4
	int*								pCurrVSInt4 = nullptr;			// 16 * int4
	BOOL*								pCurrVSBool = nullptr;			// 16 * BOOL
	float*								pCurrPSFloat4 = nullptr;		// 224 * float4
	int*								pCurrPSInt4 = nullptr;			// 16 * int4
	BOOL*								pCurrPSBool = nullptr;			// 16 * BOOL

	CArray<CD3D9SwapChain>				SwapChains;
	CDict<CStrID, PD3D9VertexLayout>	VertexLayouts;
	CArray<PD3D9RenderState>			RenderStates;
	PD3D9RenderState					DefaultRenderState;
	CArray<PD3D9Sampler>				Samplers;
	PD3D9Sampler						DefaultSampler;
	bool								IsInsideFrame = false;

	PD3D9DriverFactory                  _DriverFactory;
	D3DCAPS9							D3DCaps;
	IDirect3DDevice9*					pD3DDevice = nullptr;

	struct CTmpCB
	{
		PD3D9ConstantBuffer	CB;
		CTmpCB*				pNext;
	};

	CPoolAllocator<CTmpCB>				TmpCBPool;
	CHashTable<const CSM30ConstantBufferParam*, CTmpCB*>	TmpConstantBuffers;
	CTmpCB*								pPendingCBHead = nullptr;

	Events::PSub						Sub_OnPaint; // Fullscreen-only, so only one swap chain will be subscribed

	CD3D9GPUDriver(CD3D9DriverFactory& DriverFactory);

	// Events are received from swap chain windows, so subscriptions are in swap chains
	bool						OnOSWindowToggleFullscreen(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool						OnOSWindowResized(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool						OnOSWindowPaint(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool						OnOSWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

	bool						CreateD3DDevice(UPTR CurrAdapterID, EGPUDriverType CurrDriverType, D3DPRESENT_PARAMETERS D3DPresentParams);
	void						SetDefaultRenderState();
	void						SetDefaultSamplers();
	bool						InitSwapChainRenderTarget(CD3D9SwapChain& SC);
	bool						Reset(D3DPRESENT_PARAMETERS& D3DPresentParams, UPTR TargetSwapChainID);
	void						Release();
	bool						FindNextShaderConstRegion(UPTR BufStart, UPTR BufEnd, UPTR CurrConst, UPTR ApplyFlag, UPTR& FoundRangeStart, UPTR& FoundRangeEnd, CCBRec*& pFoundRec);
	void						ApplyShaderConstChanges();
	UPTR						InternalDraw(const CPrimitiveGroup& PrimGroup);

	void						FillD3DPresentParams(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, HWND hWnd, D3DPRESENT_PARAMETERS& D3DPresentParams) const;
	bool						GetCurrD3DPresentParams(const CD3D9SwapChain& SC, D3DPRESENT_PARAMETERS& D3DPresentParams) const;
	static D3DDEVTYPE			GetD3DDriverType(EGPUDriverType DriverType);
	static bool					GetUsagePool(UPTR InAccessFlags, DWORD& OutUsage, D3DPOOL& OutPool);
	static UINT					GetD3DLockFlags(EResourceMapMode MapMode);
	static D3DCUBEMAP_FACES		GetD3DCubeMapFace(ECubeMapFace Face);
	static D3DCMPFUNC			GetD3DCmpFunc(ECmpFunc Func);
	static D3DSTENCILOP			GetD3DStencilOp(EStencilOp Operation);
	static D3DBLEND				GetD3DBlendArg(EBlendArg Arg);
	static D3DBLENDOP			GetD3DBlendOp(EBlendOp Operation);
	static D3DTEXTUREADDRESS	GetD3DTexAddressMode(ETexAddressMode Mode);
	static void					GetD3DTexFilter(ETexFilter Filter, D3DTEXTUREFILTERTYPE& OutMin, D3DTEXTUREFILTERTYPE& OutMag, D3DTEXTUREFILTERTYPE& OutMip);

public:

	virtual ~CD3D9GPUDriver();

	virtual bool				Init(UPTR AdapterNumber, EGPUDriverType DriverType);
	virtual bool				CheckCaps(ECaps Cap) const;
	virtual bool				SupportsShaderFormat(U32 ShaderFormatCode) const { return ShaderFormatCode == 'DX9C'; }
	virtual UPTR				GetMaxVertexStreams() const;
	virtual UPTR				GetMaxTextureSize(ETextureType Type) const;
	virtual UPTR				GetMaxMultipleRenderTargetCount() const { return CurrRT.GetCount(); }

	virtual int					CreateSwapChain(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, DEM::Sys::COSWindow* pWindow);
	virtual bool				DestroySwapChain(UPTR SwapChainID);
	virtual bool				SwapChainExists(UPTR SwapChainID) const;
	virtual bool				ResizeSwapChain(UPTR SwapChainID, unsigned int Width, unsigned int Height);
	virtual bool				SwitchToFullscreen(UPTR SwapChainID, CDisplayDriver* pDisplay = nullptr, const CDisplayMode* pMode = nullptr);
	virtual bool				SwitchToWindowed(UPTR SwapChainID, const Data::CRect* pWindowRect = nullptr);
	virtual bool				IsFullscreen(UPTR SwapChainID) const;
	virtual PRenderTarget		GetSwapChainRenderTarget(UPTR SwapChainID) const;
	virtual DEM::Sys::COSWindow*	GetSwapChainWindow(UPTR SwapChainID) const override;
	virtual PDisplayDriver		GetSwapChainDisplay(UPTR SwapChainID) const override;
	virtual bool				Present(UPTR SwapChainID);
	virtual bool				CaptureScreenshot(UPTR SwapChainID, IO::CStream& OutStream) const;

	virtual PVertexLayout		CreateVertexLayout(const CVertexComponent* pComponents, UPTR Count);
	virtual PVertexBuffer		CreateVertexBuffer(CVertexLayout& VertexLayout, UPTR VertexCount, UPTR AccessFlags, const void* pData = nullptr);
	virtual PIndexBuffer		CreateIndexBuffer(EIndexType IndexType, UPTR IndexCount, UPTR AccessFlags, const void* pData = nullptr);
	virtual PRenderState		CreateRenderState(const CRenderStateDesc& Desc);
	virtual PShader				CreateShader(IO::CStream& Stream, CShaderLibrary* pLibrary = nullptr, bool LoadParamTable = true) override;
	virtual PShaderParamTable   LoadShaderParamTable(uint32_t ShaderFormatCode, IO::CStream& Stream) override;
	virtual PConstantBuffer		CreateConstantBuffer(IConstantBufferParam& Param, UPTR AccessFlags, const CConstantBuffer* pData = nullptr);
	virtual PConstantBuffer		CreateTemporaryConstantBuffer(IConstantBufferParam& Param);
	virtual void				FreeTemporaryConstantBuffer(CConstantBuffer& CBuffer);
	virtual PTexture			CreateTexture(PTextureData Data, UPTR AccessFlags);
	virtual PSampler			CreateSampler(const CSamplerDesc& Desc);
	virtual PRenderTarget		CreateRenderTarget(const CRenderTargetDesc& Desc);
	virtual PDepthStencilBuffer	CreateDepthStencilBuffer(const CRenderTargetDesc& Desc);

	virtual bool				SetViewport(UPTR Index, const CViewport* pViewport); // nullptr to reset
	virtual bool				GetViewport(UPTR Index, CViewport& OutViewport);
	virtual bool				SetScissorRect(UPTR Index, const Data::CRect* pScissorRect); // nullptr to reset
	virtual bool				GetScissorRect(UPTR Index, Data::CRect& OutScissorRect);

	virtual bool				SetVertexLayout(CVertexLayout* pVLayout);
	virtual bool				SetVertexBuffer(UPTR Index, CVertexBuffer* pVB, UPTR OffsetVertex = 0);
	virtual bool				SetIndexBuffer(CIndexBuffer* pIB);
	virtual bool				SetRenderState(CRenderState* pState);
	virtual bool				SetRenderTarget(UPTR Index, CRenderTarget* pRT);
	virtual bool				SetDepthStencilBuffer(CDepthStencilBuffer* pDS);
	virtual CRenderTarget*		GetRenderTarget(UPTR Index) const;
	virtual CDepthStencilBuffer* GetDepthStencilBuffer() const;

	virtual bool				BeginFrame();
	virtual void				EndFrame();
	virtual void				Clear(UPTR Flags, const vector4& ColorRGBA, float Depth, U8 Stencil);
	virtual void				ClearRenderTarget(CRenderTarget& RT, const vector4& ColorRGBA);
	virtual void				ClearDepthStencilBuffer(CDepthStencilBuffer& DS, UPTR Flags, float Depth, U8 Stencil);
	virtual bool				Draw(const CPrimitiveGroup& PrimGroup);
	virtual bool				DrawInstanced(const CPrimitiveGroup& PrimGroup, UPTR InstanceCount);

	virtual bool				MapResource(void** ppOutData, const CVertexBuffer& Resource, EResourceMapMode Mode);
	virtual bool				MapResource(void** ppOutData, const CIndexBuffer& Resource, EResourceMapMode Mode);
	virtual bool				MapResource(CImageData& OutData, const CTexture& Resource, EResourceMapMode Mode, UPTR ArraySlice = 0, UPTR MipLevel = 0);
	virtual bool				UnmapResource(const CVertexBuffer& Resource);
	virtual bool				UnmapResource(const CIndexBuffer& Resource);
	virtual bool				UnmapResource(const CTexture& Resource, UPTR ArraySlice = 0, UPTR MipLevel = 0);
	virtual bool				ReadFromResource(void* pDest, const CVertexBuffer& Resource, UPTR Size = 0, UPTR Offset = 0);
	virtual bool				ReadFromResource(void* pDest, const CIndexBuffer& Resource, UPTR Size = 0, UPTR Offset = 0);
	virtual bool				ReadFromResource(const CImageData& Dest, const CTexture& Resource, UPTR ArraySlice = 0, UPTR MipLevel = 0, const Data::CBox* pRegion = nullptr);
	virtual bool				WriteToResource(CVertexBuffer& Resource, const void* pData, UPTR Size = 0, UPTR Offset = 0);
	virtual bool				WriteToResource(CIndexBuffer& Resource, const void* pData, UPTR Size = 0, UPTR Offset = 0);
	virtual bool				WriteToResource(CTexture& Resource, const CImageData& SrcData, UPTR ArraySlice = 0, UPTR MipLevel = 0, const Data::CBox* pRegion = nullptr);
	virtual bool				WriteToResource(CConstantBuffer& Resource, const void* pData, UPTR Size = 0, UPTR Offset = 0) { FAIL; }

	virtual bool				BeginShaderConstants(CConstantBuffer& Buffer);
	virtual bool				CommitShaderConstants(CConstantBuffer& Buffer);

	//void						SetWireframe(bool Wire);
	//bool						IsWireframe() const { return Wireframe; }

	bool                        BindConstantBuffer(EShaderType ShaderType, U32 SlotIndex, CD3D9ConstantBuffer* pCBuffer);
	bool                        BindResource(EShaderType ShaderType, U32 Register, CD3D9Texture* pResource);
	bool                        BindSampler(EShaderType ShaderType, U32 RegisterStart, U32 RegisterCount, CD3D9Sampler* pSampler);
	void                        UnbindConstantBuffer(EShaderType ShaderType, U32 SlotIndex, CD3D9ConstantBuffer& CBuffer);
	void                        UnbindResource(EShaderType ShaderType, U32 Register, CD3D9Texture& Resource);
	void                        UnbindSampler(EShaderType ShaderType, U32 RegisterStart, U32 RegisterCount, CD3D9Sampler& Sampler);

	bool						GetD3DMSAAParams(EMSAAQuality MSAA, D3DFORMAT Format, D3DMULTISAMPLE_TYPE& OutType, DWORD& OutQuality) const;
	IDirect3DDevice9*			GetD3DDevice() const { return pD3DDevice; }
};

typedef Ptr<CD3D9GPUDriver> PD3D9GPUDriver;

}
