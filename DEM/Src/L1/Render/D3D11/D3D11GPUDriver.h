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
struct ID3D11ShaderResourceView;
struct D3D11_VIEWPORT;
typedef enum D3D_DRIVER_TYPE D3D_DRIVER_TYPE;
enum D3D11_USAGE;
enum D3D11_MAP;
enum D3D11_COMPARISON_FUNC;
enum D3D11_STENCIL_OP;
enum D3D11_BLEND;
enum D3D11_BLEND_OP;
enum D3D11_TEXTURE_ADDRESS_MODE;
enum D3D11_FILTER;
typedef struct tagRECT RECT;
typedef unsigned int UINT;

namespace Render
{
typedef Ptr<class CD3D11VertexLayout> PD3D11VertexLayout;
typedef Ptr<class CD3D11VertexBuffer> PD3D11VertexBuffer;
typedef Ptr<class CD3D11IndexBuffer> PD3D11IndexBuffer;
typedef Ptr<class CD3D11RenderTarget> PD3D11RenderTarget;
typedef Ptr<class CD3D11DepthStencilBuffer> PD3D11DepthStencilBuffer;
typedef Ptr<class CD3D11RenderState> PD3D11RenderState;
typedef Ptr<class CD3D11ConstantBuffer> PD3D11ConstantBuffer;
typedef Ptr<class CD3D11Sampler> PD3D11Sampler;
typedef Ptr<class CD3D11Texture> PD3D11Texture;

class CD3D11GPUDriver: public CGPUDriver
{
	__DeclareClass(CD3D11GPUDriver);

public:

	enum
	{
		GPU_Dirty_VL	= 0x0001,		// Vertex layout
		GPU_Dirty_VB	= 0x0002,		// Vertex buffer(s)
		GPU_Dirty_IB	= 0x0004,		// Index buffer
		GPU_Dirty_RS	= 0x0008,		// Render state
		GPU_Dirty_RT	= 0x0010,		// Render target(s)
		GPU_Dirty_DS	= 0x0020,		// Depth-stencil buffer
		GPU_Dirty_VP	= 0x0040,		// Viewport(s)
		GPU_Dirty_SR	= 0x0080,		// Scissor rect(s)
		GPU_Dirty_CB	= 0x0100,		// Sampler state(s)
		GPU_Dirty_SS	= 0x0200,		// Sampler state(s)
		GPU_Dirty_SRV	= 0x0400,		// Shader resource view(s)

		GPU_Dirty_All = 0xffffffff	// All bits set, for convenience in ApplyChanges() call
	};

protected:

	enum
	{
		Shader_Dirty_Samplers		= 0,
		Shader_Dirty_Resources		= 8,
		Shader_Dirty_CBuffers		= 16,
		Shader_Dirty_VS_Samplers	= (1 << (Shader_Dirty_Samplers + ShaderType_Vertex)),
		Shader_Dirty_PS_Samplers	= (1 << (Shader_Dirty_Samplers + ShaderType_Pixel)),
		Shader_Dirty_GS_Samplers	= (1 << (Shader_Dirty_Samplers + ShaderType_Geometry)),
		Shader_Dirty_HS_Samplers	= (1 << (Shader_Dirty_Samplers + ShaderType_Hull)),
		Shader_Dirty_DS_Samplers	= (1 << (Shader_Dirty_Samplers + ShaderType_Domain)),
		Shader_Dirty_VS_Resources	= (1 << (Shader_Dirty_Resources + ShaderType_Vertex)),
		Shader_Dirty_PS_Resources	= (1 << (Shader_Dirty_Resources + ShaderType_Pixel)),
		Shader_Dirty_GS_Resources	= (1 << (Shader_Dirty_Resources + ShaderType_Geometry)),
		Shader_Dirty_HS_Resources	= (1 << (Shader_Dirty_Resources + ShaderType_Hull)),
		Shader_Dirty_DS_Resources	= (1 << (Shader_Dirty_Resources + ShaderType_Domain)),
		Shader_Dirty_VS_CBuffers	= (1 << (Shader_Dirty_CBuffers + ShaderType_Vertex)),
		Shader_Dirty_PS_CBuffers	= (1 << (Shader_Dirty_CBuffers + ShaderType_Pixel)),
		Shader_Dirty_GS_CBuffers	= (1 << (Shader_Dirty_CBuffers + ShaderType_Geometry)),
		Shader_Dirty_HS_CBuffers	= (1 << (Shader_Dirty_CBuffers + ShaderType_Hull)),
		Shader_Dirty_DS_CBuffers	= (1 << (Shader_Dirty_CBuffers + ShaderType_Domain))
	};

	Data::CFlags						CurrDirtyFlags;
	Data::CFlags						ShaderParamsDirtyFlags;
	PD3D11VertexLayout					CurrVL;
	ID3D11InputLayout*					pCurrIL;
	CFixedArray<PD3D11VertexBuffer>		CurrVB;
	CFixedArray<UPTR>					CurrVBOffset;
	PD3D11IndexBuffer					CurrIB;
	EPrimitiveTopology					CurrPT;
	CFixedArray<PD3D11RenderTarget>		CurrRT;
	PD3D11DepthStencilBuffer			CurrDS;
	PD3D11RenderState					CurrRS;
	PD3D11RenderState					NewRS;
	D3D11_VIEWPORT*						CurrVP;
	RECT*								CurrSR;				//???SR corresp to VP, mb set in pairs and use all 32 bits each for a pair?
	UPTR								MaxViewportCount;
	Data::CFlags						VPSRSetFlags;		// 16 low bits indicate whether VP is set or not, same for SR in 16 high bits
	static const UPTR					VP_OR_SR_SET_FLAG_COUNT = 16;
	CFixedArray<PD3D11ConstantBuffer>	CurrCB;
	CFixedArray<PD3D11Sampler>			CurrSS;
	CDict<UPTR, ID3D11ShaderResourceView*>	CurrSRV; // ShaderType|Register to SRV mapping, not to store all 128 possible SRV values per shader type
//!!!ID3D11ShaderResourceView* or PObject!
	UPTR								MaxSRVSlotIndex;

	CArray<CD3D11SwapChain>				SwapChains;
	CDict<CStrID, PD3D11VertexLayout>	VertexLayouts;
	CArray<PD3D11RenderState>			RenderStates;
	CArray<PD3D11Sampler>				Samplers;
	//bool								IsInsideFrame;
	//bool								Wireframe;

	ID3D11Device*						pD3DDevice;
	ID3D11DeviceContext*				pD3DImmContext;
	//???store also D3D11.1 interfaces? and use for 11.1 methods only.

	CD3D11GPUDriver();

	bool								OnOSWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool								OnOSWindowSizeChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool								OnOSWindowToggleFullscreen(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	//bool								OnOSWindowPaint(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

	bool								InitSwapChainRenderTarget(CD3D11SwapChain& SC);
	void								Release();

	static D3D_DRIVER_TYPE				GetD3DDriverType(EGPUDriverType DriverType);
	static EGPUDriverType				GetDEMDriverType(D3D_DRIVER_TYPE DriverType);
	static void							GetUsageAccess(UPTR InAccessFlags, bool InitDataProvided, D3D11_USAGE& OutUsage, UINT& OutCPUAccess);
	static void							GetD3DMapTypeAndFlags(EResourceMapMode MapMode, D3D11_MAP& OutMapType, UPTR& OutMapFlags);
	static D3D11_COMPARISON_FUNC		GetD3DCmpFunc(ECmpFunc Func);
	static D3D11_STENCIL_OP				GetD3DStencilOp(EStencilOp Operation);
	static D3D11_BLEND					GetD3DBlendArg(EBlendArg Arg);
	static D3D11_BLEND_OP				GetD3DBlendOp(EBlendOp Operation);
	static D3D11_TEXTURE_ADDRESS_MODE	GetD3DTexAddressMode(ETexAddressMode Mode);
	static D3D11_FILTER					GetD3DTexFilter(ETexFilter Filter, bool Comparison);

	ID3D11InputLayout*					GetD3DInputLayout(CD3D11VertexLayout& VertexLayout, UPTR ShaderInputSignatureID, const Data::CBuffer* pSignature = NULL);
	bool								ReadFromD3DBuffer(void* pDest, ID3D11Buffer* pBuf, D3D11_USAGE Usage, UPTR BufferSize, UPTR Size, UPTR Offset);
	bool								WriteToD3DBuffer(ID3D11Buffer* pBuf, D3D11_USAGE Usage, UPTR BufferSize, const void* pData, UPTR Size, UPTR Offset);
	bool								BindSRV(EShaderType ShaderType, UPTR SlotIndex, ID3D11ShaderResourceView* pSRV); //!!!ID3D11ShaderResourceView* or PObject!

	friend class CD3D11DriverFactory;

public:

	virtual ~CD3D11GPUDriver() { Release(); }

	virtual bool				Init(UPTR AdapterNumber, EGPUDriverType DriverType);
	virtual bool				CheckCaps(ECaps Cap) const;
	virtual bool				SupportsShaderModel(U32 ShaderModel) const { return ShaderModel >= 0x0400; }
	virtual UPTR				GetMaxVertexStreams() const;
	virtual UPTR				GetMaxTextureSize(ETextureType Type) const;
	virtual UPTR				GetMaxMultipleRenderTargetCount() const { return CurrRT.GetCount(); }

	virtual int					CreateSwapChain(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, Sys::COSWindow* pWindow);
	virtual bool				DestroySwapChain(UPTR SwapChainID);
	virtual bool				SwapChainExists(UPTR SwapChainID) const;
	virtual bool				ResizeSwapChain(UPTR SwapChainID, unsigned int Width, unsigned int Height);
	virtual bool				SwitchToFullscreen(UPTR SwapChainID, CDisplayDriver* pDisplay = NULL, const CDisplayMode* pMode = NULL);
	virtual bool				SwitchToWindowed(UPTR SwapChainID, const Data::CRect* pWindowRect = NULL);
	virtual bool				IsFullscreen(UPTR SwapChainID) const;
	virtual PRenderTarget		GetSwapChainRenderTarget(UPTR SwapChainID) const;
	virtual bool				Present(UPTR SwapChainID);
	virtual bool				CaptureScreenshot(UPTR SwapChainID, IO::CStream& OutStream) const;

	virtual PVertexLayout		CreateVertexLayout(const CVertexComponent* pComponents, UPTR Count);
	virtual PVertexBuffer		CreateVertexBuffer(CVertexLayout& VertexLayout, UPTR VertexCount, UPTR AccessFlags, const void* pData = NULL);
	virtual PIndexBuffer		CreateIndexBuffer(EIndexType IndexType, UPTR IndexCount, UPTR AccessFlags, const void* pData = NULL);
	virtual PRenderState		CreateRenderState(const CRenderStateDesc& Desc);
	virtual PShader				CreateShader(EShaderType ShaderType, const void* pData, UPTR Size);
	virtual PConstantBuffer		CreateConstantBuffer(HConstBuffer hBuffer, UPTR AccessFlags, const CConstantBuffer* pData = NULL);
	virtual PTexture			CreateTexture(const CTextureDesc& Desc, UPTR AccessFlags, const void* pData = NULL, bool MipDataProvided = false);
	virtual PSampler			CreateSampler(const CSamplerDesc& Desc);
	virtual PRenderTarget		CreateRenderTarget(const CRenderTargetDesc& Desc);
	virtual PDepthStencilBuffer	CreateDepthStencilBuffer(const CRenderTargetDesc& Desc);

	virtual bool				SetViewport(UPTR Index, const CViewport* pViewport); // NULL to reset
	virtual bool				GetViewport(UPTR Index, CViewport& OutViewport);
	virtual bool				SetScissorRect(UPTR Index, const Data::CRect* pScissorRect); // NULL to reset
	virtual bool				GetScissorRect(UPTR Index, Data::CRect& OutScissorRect);

	virtual bool				SetVertexLayout(CVertexLayout* pVLayout);
	virtual bool				SetVertexBuffer(UPTR Index, CVertexBuffer* pVB, UPTR OffsetVertex = 0);
	virtual bool				SetIndexBuffer(CIndexBuffer* pIB);
	//virtual bool				SetInstanceBuffer(UPTR Index, CVertexBuffer* pVB, UPTR Instances, UPTR OffsetVertex = 0);
	virtual bool				SetRenderState(CRenderState* pState);
	virtual bool				SetRenderTarget(UPTR Index, CRenderTarget* pRT);
	virtual bool				SetDepthStencilBuffer(CDepthStencilBuffer* pDS);

	virtual bool				BindConstantBuffer(EShaderType ShaderType, HConstBuffer Handle, CConstantBuffer* pCBuffer);
	virtual bool				BindResource(EShaderType ShaderType, HResource Handle, CTexture* pResource);
	virtual bool				BindSampler(EShaderType ShaderType, HSampler Handle, CSampler* pSampler);

	virtual bool				BeginFrame();
	virtual void				EndFrame();
	virtual void				Clear(UPTR Flags, const vector4& ColorRGBA, float Depth, U8 Stencil);
	virtual void				ClearRenderTarget(CRenderTarget& RT, const vector4& ColorRGBA);
	virtual bool				Draw(const CPrimitiveGroup& PrimGroup); //???instance count?

	//???virtual to unify interface? no-op where is not applicable. or only apply on draw etc here?
	//can also set current values and call CreateRenderCache for the current set, which will generate layouts etc and even return cache object
	//then Draw(CRenderCache&). D3D12 bundles may perfectly fit into this architecture.
	UPTR						ApplyChanges(UPTR ChangesToUpdate = GPU_Dirty_All); // returns a combination of dirty flags where errors occurred

	virtual bool				MapResource(void** ppOutData, const CVertexBuffer& Resource, EResourceMapMode Mode);
	virtual bool				MapResource(void** ppOutData, const CIndexBuffer& Resource, EResourceMapMode Mode);
	virtual bool				MapResource(CImageData& OutData, const CTexture& Resource, EResourceMapMode Mode, UPTR ArraySlice = 0, UPTR MipLevel = 0);
	virtual bool				UnmapResource(const CVertexBuffer& Resource);
	virtual bool				UnmapResource(const CIndexBuffer& Resource);
	virtual bool				UnmapResource(const CTexture& Resource, UPTR ArraySlice = 0, UPTR MipLevel = 0);
	virtual bool				ReadFromResource(void* pDest, const CVertexBuffer& Resource, UPTR Size = 0, UPTR Offset = 0);
	virtual bool				ReadFromResource(void* pDest, const CIndexBuffer& Resource, UPTR Size = 0, UPTR Offset = 0);
	virtual bool				ReadFromResource(const CImageData& Dest, const CTexture& Resource, UPTR ArraySlice = 0, UPTR MipLevel = 0, const Data::CBox* pRegion = NULL);
	virtual bool				WriteToResource(CVertexBuffer& Resource, const void* pData, UPTR Size = 0, UPTR Offset = 0);
	virtual bool				WriteToResource(CIndexBuffer& Resource, const void* pData, UPTR Size = 0, UPTR Offset = 0);
	virtual bool				WriteToResource(CTexture& Resource, const CImageData& SrcData, UPTR ArraySlice = 0, UPTR MipLevel = 0, const Data::CBox* pRegion = NULL);
	virtual bool				WriteToResource(CConstantBuffer& Resource, const void* pData, UPTR Size = 0, UPTR Offset = 0);

	virtual bool				BeginShaderConstants(CConstantBuffer& Buffer);
	virtual bool				SetShaderConstant(CConstantBuffer& Buffer, HConst hConst, UPTR ElementIndex, const void* pData, UPTR Size);
	virtual bool				CommitShaderConstants(CConstantBuffer& Buffer);

	//void					SetWireframe(bool Wire);
	//bool					IsWireframe() const { return Wireframe; }

	ID3D11Device*				GetD3DDevice() const { return pD3DDevice; }
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
	CurrSRV(16, 16),
	MaxSRVSlotIndex(0),
	RenderStates(32, 32),
	Samplers(16, 16),
	MaxViewportCount(0) /*, IsInsideFrame(false)*/
{
}
//---------------------------------------------------------------------

}

#endif
