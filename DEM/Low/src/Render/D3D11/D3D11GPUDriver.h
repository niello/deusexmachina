#pragma once
#include <Render/GPUDriver.h>
#include <Render/D3D11/D3D11SwapChain.h>
#include <Data/FixedArray.h>
#include <Data/Buffer.h>
#include <System/Allocators/PoolAllocator.h>

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
typedef Ptr<class CD3D11DriverFactory> PD3D11DriverFactory;
typedef Ptr<class CD3D11VertexLayout> PD3D11VertexLayout;
typedef Ptr<class CD3D11VertexBuffer> PD3D11VertexBuffer;
typedef Ptr<class CD3D11IndexBuffer> PD3D11IndexBuffer;
typedef Ptr<class CD3D11RenderTarget> PD3D11RenderTarget;
typedef Ptr<class CD3D11DepthStencilBuffer> PD3D11DepthStencilBuffer;
typedef Ptr<class CD3D11RenderState> PD3D11RenderState;
typedef Ptr<class CD3D11ConstantBuffer> PD3D11ConstantBuffer;
typedef Ptr<class CD3D11Sampler> PD3D11Sampler;
typedef Ptr<class CD3D11Texture> PD3D11Texture;
enum EUSMBufferType;

class CD3D11GPUDriver: public CGPUDriver
{
	RTTI_CLASS_DECL;

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

	friend class CD3D11DriverFactory;

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
	ID3D11InputLayout*					pCurrIL = nullptr;
	CFixedArray<PD3D11VertexBuffer>		CurrVB;
	CFixedArray<UPTR>					CurrVBOffset;
	PD3D11IndexBuffer					CurrIB;
	EPrimitiveTopology					CurrPT = Prim_Invalid;
	CFixedArray<PD3D11RenderTarget>		CurrRT;
	PD3D11DepthStencilBuffer			CurrDS;
	PD3D11RenderState					CurrRS;
	PD3D11RenderState					NewRS;
	D3D11_VIEWPORT*						CurrVP = nullptr;
	RECT*								CurrSR = nullptr;	//???SR corresp to VP, mb set in pairs and use all 32 bits each for a pair?
	UPTR								MaxViewportCount = 0;
	Data::CFlags						VPSRSetFlags;		// 16 low bits indicate whether VP is set or not, same for SR in 16 high bits
	static const UPTR					VP_OR_SR_SET_FLAG_COUNT = 16;
	CFixedArray<PD3D11ConstantBuffer>	CurrCB;
	CFixedArray<PD3D11Sampler>			CurrSS;

	struct CSRVRecord
	{
		ID3D11ShaderResourceView*	pSRV;
		PD3D11ConstantBuffer		CB;		// nullptr if SRV is not a buffer //???or store PObject - SRV source?
	};

	CDict<UPTR, CSRVRecord>				CurrSRV; // ShaderType|Register to SRV mapping, not to store all 128 possible SRV values per shader type

	UPTR								MaxSRVSlotIndex = 0;

	CArray<CD3D11SwapChain>				SwapChains;
	CDict<CStrID, PD3D11VertexLayout>	VertexLayouts;
	CArray<PD3D11RenderState>			RenderStates;
	CArray<PD3D11Sampler>				Samplers;
	//bool								IsInsideFrame;
	//bool								Wireframe;

	PD3D11DriverFactory                 _DriverFactory;
	ID3D11Device*						pD3DDevice = nullptr;
	ID3D11DeviceContext*				pD3DImmContext = nullptr;
	//???store also D3D11.1 interfaces? and use for 11.1 methods only.

	struct CTmpCB
	{
		PD3D11ConstantBuffer	CB;
		CTmpCB*					pNext;
	};

	CPool<CTmpCB>				        TmpCBPool;
	CDict<UPTR, CTmpCB*>				TmpConstantBuffers;	// Key is a size (pow2), value is a linked list
	CDict<UPTR, CTmpCB*>				TmpTextureBuffers;	// Key is a size (pow2), value is a linked list
	CDict<UPTR, CTmpCB*>				TmpStructuredBuffers;	// Key is a size (pow2), value is a linked list
	CTmpCB*								pPendingCBHead = nullptr;

	CD3D11GPUDriver(CD3D11DriverFactory& DriverFactory);

	bool								OnOSWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool								OnOSWindowToggleFullscreen(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	//bool								OnOSWindowPaint(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

	bool								InitSwapChainRenderTarget(CD3D11SwapChain& SC);
	void								Release();
	PD3D11ConstantBuffer				InternalCreateConstantBuffer(EUSMBufferType Type, U32 Size, UPTR AccessFlags, const CConstantBuffer* pData, bool Temporary);
	bool								InternalDraw(const CPrimitiveGroup& PrimGroup, bool Instanced, UPTR InstanceCount);

	static D3D_DRIVER_TYPE				GetD3DDriverType(EGPUDriverType DriverType);
	static EGPUDriverType				GetDEMDriverType(D3D_DRIVER_TYPE DriverType);
	static bool							GetUsageAccess(UPTR InAccessFlags, bool InitDataProvided, D3D11_USAGE& OutUsage, UINT& OutCPUAccess);
	static void							GetD3DMapTypeAndFlags(EResourceMapMode MapMode, D3D11_MAP& OutMapType, UPTR& OutMapFlags);
	static D3D11_COMPARISON_FUNC		GetD3DCmpFunc(ECmpFunc Func);
	static D3D11_STENCIL_OP				GetD3DStencilOp(EStencilOp Operation);
	static D3D11_BLEND					GetD3DBlendArg(EBlendArg Arg);
	static D3D11_BLEND_OP				GetD3DBlendOp(EBlendOp Operation);
	static D3D11_TEXTURE_ADDRESS_MODE	GetD3DTexAddressMode(ETexAddressMode Mode);
	static D3D11_FILTER					GetD3DTexFilter(ETexFilter Filter, bool Comparison);

	ID3D11InputLayout*					GetD3DInputLayout(CD3D11VertexLayout& VertexLayout, UPTR ShaderInputSignatureID, const Data::CDataBuffer* pSignature = nullptr);
	bool								ReadFromD3DBuffer(void* pDest, ID3D11Buffer* pBuf, D3D11_USAGE Usage, UPTR BufferSize, UPTR Size, UPTR Offset);
	bool								WriteToD3DBuffer(ID3D11Buffer* pBuf, D3D11_USAGE Usage, UPTR BufferSize, const void* pData, UPTR Size, UPTR Offset);
	bool								BindSRV(EShaderType ShaderType, UPTR SlotIndex, ID3D11ShaderResourceView* pSRV, CD3D11ConstantBuffer* pCB);
	void								UnbindSRV(EShaderType ShaderType, UPTR SlotIndex, ID3D11ShaderResourceView* pSRV);
	bool								IsConstantBufferBound(const CD3D11ConstantBuffer* pBuffer, EShaderType ExceptStage = ShaderType_Invalid, UPTR ExceptSlot = 0);
	void								FreePendingTemporaryBuffer(const CD3D11ConstantBuffer* pBuffer, EShaderType Stage, UPTR Slot);

public:

	virtual ~CD3D11GPUDriver() override;

	virtual bool				Init(UPTR AdapterNumber, EGPUDriverType DriverType) override;
	virtual bool				CheckCaps(ECaps Cap) const override;
	virtual bool				SupportsShaderFormat(U32 ShaderFormatCode) const override { return ShaderFormatCode == 'DXBC'; }
	virtual UPTR				GetMaxVertexStreams() const override;
	virtual UPTR				GetMaxTextureSize(ETextureType Type) const override;
	virtual UPTR				GetMaxMultipleRenderTargetCount() const override { return CurrRT.GetCount(); }

	virtual int					CreateSwapChain(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, DEM::Sys::COSWindow* pWindow) override;
	virtual bool				DestroySwapChain(UPTR SwapChainID) override;
	virtual bool				SwapChainExists(UPTR SwapChainID) const override;
	virtual bool				ResizeSwapChain(UPTR SwapChainID, unsigned int Width, unsigned int Height) override;
	virtual bool				SwitchToFullscreen(UPTR SwapChainID, CDisplayDriver* pDisplay = nullptr, const CDisplayMode* pMode = nullptr) override;
	virtual bool				SwitchToWindowed(UPTR SwapChainID, const Data::CRect* pWindowRect = nullptr) override;
	virtual bool				IsFullscreen(UPTR SwapChainID) const override;
	virtual PRenderTarget		GetSwapChainRenderTarget(UPTR SwapChainID) const override;
	virtual DEM::Sys::COSWindow*	GetSwapChainWindow(UPTR SwapChainID) const override;
	virtual PDisplayDriver		GetSwapChainDisplay(UPTR SwapChainID) const override;
	virtual bool				Present(UPTR SwapChainID) override;
	virtual bool				CaptureScreenshot(UPTR SwapChainID, IO::IStream& OutStream) const override;

	virtual PVertexLayout		CreateVertexLayout(const CVertexComponent* pComponents, UPTR Count) override;
	virtual PVertexBuffer		CreateVertexBuffer(CVertexLayout& VertexLayout, UPTR VertexCount, UPTR AccessFlags, const void* pData = nullptr) override;
	virtual PIndexBuffer		CreateIndexBuffer(EIndexType IndexType, UPTR IndexCount, UPTR AccessFlags, const void* pData = nullptr) override;
	virtual PRenderState		CreateRenderState(const CRenderStateDesc& Desc) override;
	virtual PShader				CreateShader(IO::IStream& Stream, bool LoadParamTable = true) override;
	virtual PShaderParamTable   LoadShaderParamTable(uint32_t ShaderFormatCode, IO::IStream& Stream) override;
	virtual PConstantBuffer		CreateConstantBuffer(IConstantBufferParam& Param, UPTR AccessFlags, const CConstantBuffer* pData = nullptr) override;
	virtual PConstantBuffer		CreateTemporaryConstantBuffer(IConstantBufferParam& Param) override;
	virtual void				FreeTemporaryConstantBuffer(CConstantBuffer& Buffer) override;
	virtual PTexture			CreateTexture(PTextureData Data, UPTR AccessFlags) override;
	virtual PSampler			CreateSampler(const CSamplerDesc& Desc) override;
	virtual PRenderTarget		CreateRenderTarget(const CRenderTargetDesc& Desc) override;
	virtual PDepthStencilBuffer	CreateDepthStencilBuffer(const CRenderTargetDesc& Desc) override;

	virtual bool				SetViewport(UPTR Index, const CViewport* pViewport) override; // nullptr to reset
	virtual bool				GetViewport(UPTR Index, CViewport& OutViewport) override;
	virtual bool				SetScissorRect(UPTR Index, const Data::CRect* pScissorRect) override; // nullptr to reset
	virtual bool				GetScissorRect(UPTR Index, Data::CRect& OutScissorRect) override;

	virtual bool				SetVertexLayout(CVertexLayout* pVLayout) override;
	virtual bool				SetVertexBuffer(UPTR Index, CVertexBuffer* pVB, UPTR OffsetVertex = 0) override;
	virtual bool				SetIndexBuffer(CIndexBuffer* pIB) override;
	//virtual bool				SetInstanceBuffer(UPTR Index, CVertexBuffer* pVB, UPTR Instances, UPTR OffsetVertex = 0) override;
	virtual bool				SetRenderState(CRenderState* pState) override;
	virtual bool				SetRenderTarget(UPTR Index, CRenderTarget* pRT) override;
	virtual bool				SetDepthStencilBuffer(CDepthStencilBuffer* pDS) override;
	virtual CRenderTarget*		GetRenderTarget(UPTR Index) const override;
	virtual CDepthStencilBuffer* GetDepthStencilBuffer() const override;

	virtual bool				BeginFrame() override;
	virtual void				EndFrame() override;
	virtual void				Clear(UPTR Flags, const vector4& ColorRGBA, float Depth, U8 Stencil) override;
	virtual void				ClearRenderTarget(CRenderTarget& RT, const vector4& ColorRGBA) override;
	virtual void				ClearDepthStencilBuffer(CDepthStencilBuffer& DS, UPTR Flags, float Depth, U8 Stencil) override;
	virtual bool				Draw(const CPrimitiveGroup& PrimGroup) override { return InternalDraw(PrimGroup, false, 1); }
	virtual bool				DrawInstanced(const CPrimitiveGroup& PrimGroup, UPTR InstanceCount) override { return InternalDraw(PrimGroup, true, InstanceCount); }

	//can set current values and call CreateRenderCache for the current set, which will generate layouts etc and even return cache object
	//then Draw(CRenderCache&). D3D12 bundles may perfectly fit into this architecture.
	UPTR						ApplyChanges(UPTR ChangesToUpdate = GPU_Dirty_All); // returns a combination of dirty flags where errors occurred

	virtual bool				MapResource(void** ppOutData, const CVertexBuffer& Resource, EResourceMapMode Mode) override;
	virtual bool				MapResource(void** ppOutData, const CIndexBuffer& Resource, EResourceMapMode Mode) override;
	virtual bool				MapResource(CImageData& OutData, const CTexture& Resource, EResourceMapMode Mode, UPTR ArraySlice = 0, UPTR MipLevel = 0) override;
	virtual bool				UnmapResource(const CVertexBuffer& Resource) override;
	virtual bool				UnmapResource(const CIndexBuffer& Resource) override;
	virtual bool				UnmapResource(const CTexture& Resource, UPTR ArraySlice = 0, UPTR MipLevel = 0) override;
	virtual bool				ReadFromResource(void* pDest, const CVertexBuffer& Resource, UPTR Size = 0, UPTR Offset = 0) override;
	virtual bool				ReadFromResource(void* pDest, const CIndexBuffer& Resource, UPTR Size = 0, UPTR Offset = 0) override;
	virtual bool				ReadFromResource(const CImageData& Dest, const CTexture& Resource, UPTR ArraySlice = 0, UPTR MipLevel = 0, const Data::CBox* pRegion = nullptr) override;
	virtual bool				WriteToResource(CVertexBuffer& Resource, const void* pData, UPTR Size = 0, UPTR Offset = 0) override;
	virtual bool				WriteToResource(CIndexBuffer& Resource, const void* pData, UPTR Size = 0, UPTR Offset = 0) override;
	virtual bool				WriteToResource(CTexture& Resource, const CImageData& SrcData, UPTR ArraySlice = 0, UPTR MipLevel = 0, const Data::CBox* pRegion = nullptr) override;
	virtual bool				WriteToResource(CConstantBuffer& Resource, const void* pData, UPTR Size = 0, UPTR Offset = 0) override;

	virtual bool				BeginShaderConstants(CConstantBuffer& Buffer) override;
	virtual bool				CommitShaderConstants(CConstantBuffer& Buffer) override;

	bool                        BindConstantBuffer(EShaderType ShaderType, EUSMBufferType Type, U32 Register, CD3D11ConstantBuffer* pBuffer);
	bool                        BindResource(EShaderType ShaderType, U32 Register, CD3D11Texture* pResource);
	bool                        BindSampler(EShaderType ShaderType, U32 Register, CD3D11Sampler* pSampler);
	void                        UnbindConstantBuffer(EShaderType ShaderType, EUSMBufferType Type, U32 Register, CD3D11ConstantBuffer& Buffer);
	void                        UnbindResource(EShaderType ShaderType, U32 Register, CD3D11Texture& Resource);
	void                        UnbindSampler(EShaderType ShaderType, U32 Register, CD3D11Sampler& Sampler);

	//void					SetWireframe(bool Wire);
	//bool					IsWireframe() const { return Wireframe; }

	ID3D11Device*				GetD3DDevice() const { return pD3DDevice; }
};

typedef Ptr<CD3D11GPUDriver> PD3D11GPUDriver;

}
