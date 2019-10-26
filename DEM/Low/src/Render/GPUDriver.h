#pragma once
#include <Core/Object.h>
#include <Render/RenderFwd.h>
#include <Data/Regions.h>

// GPU device driver manages VRAM resources and provides an interface for rendering on a video card.
// Create GPU device drivers with CVideoDriverFactory.
// You can implement this class via some graphics API, like D3D or OpenGL.
// Each GPU device can manage zero or more swap chains, either windowed or fullscreen. Each fullscreen
// swap chain must be connected to a corresponding display output, represented by a CDisplayDriver instance.
// Windowed swap chains are present to the desktop. Each swap chain requires a viewport, and now
// I use COSWindow for it.
// Objects created by GPU are specific to this GPU, so each GPU manages its own textures, shaders etc.
// They can't be shared through a CResourceManager, but it may manage templates, like RAM texture data
// or material template with IDs instead of real GPU objects. Template can be instantiated with a GPU.

//PERF:
//!!!GCN says load shaders before textures, driver compiles to its ASM in the background!
//then warmup shader cache - bind all shaders and perform offscreen rendering

namespace IO
{
	class CStream;
}

namespace Data
{
	class CParams;
}

namespace DEM::Sys
{
	typedef Ptr<class COSWindow> POSWindow;
}

namespace Render
{

class CGPUDriver: public Core::CObject
{
protected:

	UPTR				AdapterID = Adapter_None;
	EGPUDriverType		Type;
	EGPUFeatureLevel	FeatureLevel;	// Must be filled on init

#ifdef DEM_STATS
	UPTR				PrimitivesRendered = 0;
	UPTR				DrawsRendered = 0;
#endif

	static void					PrepareWindowAndBackBufferSize(DEM::Sys::COSWindow& Window, U32& Width, U32& Height);

public:

	CGPUDriver();
	virtual ~CGPUDriver();

	EGPUDriverType				GetType() const { return Type; }
	EGPUFeatureLevel			GetFeatureLevel() const { return FeatureLevel; }

	virtual bool				Init(UPTR AdapterNumber, EGPUDriverType DriverType) { AdapterID = AdapterNumber; OK; }
	virtual bool				CheckCaps(ECaps Cap) const = 0;
	virtual bool				SupportsShaderFormat(U32 ShaderFormatCode) const = 0;
	virtual UPTR				GetMaxVertexStreams() const = 0;
	virtual UPTR				GetMaxTextureSize(ETextureType Type) const = 0;
	virtual UPTR				GetMaxMultipleRenderTargetCount() const = 0;

	virtual int					CreateSwapChain(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, DEM::Sys::COSWindow* pWindow) = 0;
	virtual bool				DestroySwapChain(UPTR SwapChainID) = 0;
	virtual bool				SwapChainExists(UPTR SwapChainID) const = 0;
	virtual bool				SwitchToFullscreen(UPTR SwapChainID, CDisplayDriver* pDisplay = nullptr, const CDisplayMode* pMode = nullptr) = 0;
	virtual bool				SwitchToWindowed(UPTR SwapChainID, const Data::CRect* pWindowRect = nullptr) = 0;
	virtual bool				ResizeSwapChain(UPTR SwapChainID, unsigned int Width, unsigned int Height) = 0;
	virtual bool				IsFullscreen(UPTR SwapChainID) const = 0;
	virtual PRenderTarget		GetSwapChainRenderTarget(UPTR SwapChainID) const = 0;
	virtual DEM::Sys::COSWindow*	GetSwapChainWindow(UPTR SwapChainID) const = 0;
	virtual PDisplayDriver		GetSwapChainDisplay(UPTR SwapChainID) const = 0;
	virtual bool				Present(UPTR SwapChainID) = 0;
	bool						PresentBlankScreen(UPTR SwapChainID, const vector4& ColorRGBA);
	virtual bool				CaptureScreenshot(UPTR SwapChainID, IO::CStream& OutStream) const = 0;

	virtual PVertexLayout		CreateVertexLayout(const CVertexComponent* pComponents, UPTR Count) = 0;
	virtual PVertexBuffer		CreateVertexBuffer(CVertexLayout& VertexLayout, UPTR VertexCount, UPTR AccessFlags, const void* pData = nullptr) = 0;
	virtual PIndexBuffer		CreateIndexBuffer(EIndexType IndexType, UPTR IndexCount, UPTR AccessFlags, const void* pData = nullptr) = 0;
	virtual PRenderState		CreateRenderState(const CRenderStateDesc& Desc) = 0;
	virtual PShader				CreateShader(IO::CStream& Stream, CShaderLibrary* pLibrary = nullptr, bool LoadParamTable = true) = 0;
	virtual PShaderParamTable   LoadShaderParamTable(uint32_t ShaderFormatCode, IO::CStream& Stream) = 0;
	virtual PConstantBuffer		CreateConstantBuffer(HConstantBuffer hBuffer, UPTR AccessFlags, const CConstantBuffer* pData = nullptr) = 0;
	virtual PConstantBuffer		CreateTemporaryConstantBuffer(HConstantBuffer hBuffer) = 0;
	virtual void				FreeTemporaryConstantBuffer(CConstantBuffer& CBuffer) = 0;
	virtual PTexture			CreateTexture(PTextureData Data, UPTR AccessFlags) = 0;
	virtual PSampler			CreateSampler(const CSamplerDesc& Desc) = 0;
	virtual PRenderTarget		CreateRenderTarget(const CRenderTargetDesc& Desc) = 0;
	virtual PDepthStencilBuffer	CreateDepthStencilBuffer(const CRenderTargetDesc& Desc) = 0;

	virtual bool				SetViewport(UPTR Index, const CViewport* pViewport) = 0; // nullptr to reset
	virtual bool				GetViewport(UPTR Index, CViewport& OutViewport) = 0;
	virtual bool				SetScissorRect(UPTR Index, const Data::CRect* pScissorRect) = 0; // nullptr to reset
	virtual bool				GetScissorRect(UPTR Index, Data::CRect& OutScissorRect) = 0;

	virtual bool				SetVertexLayout(CVertexLayout* pVLayout) = 0;
	virtual bool				SetVertexBuffer(UPTR Index, CVertexBuffer* pVB, UPTR OffsetVertex = 0) = 0;
	virtual bool				SetIndexBuffer(CIndexBuffer* pIB) = 0;
	//virtual bool				SetInstanceBuffer(UPTR Index, CVertexBuffer* pVB, UPTR Instances, UPTR OffsetVertex = 0) = 0;
	virtual bool				SetRenderState(CRenderState* pState) = 0;
	virtual bool				SetRenderTarget(UPTR Index, CRenderTarget* pRT) = 0;
	virtual bool				SetDepthStencilBuffer(CDepthStencilBuffer* pDS) = 0;

	virtual bool				BindConstantBuffer(EShaderType ShaderType, HConstantBuffer Handle, CConstantBuffer* pCBuffer) = 0;
	virtual bool				BindResource(EShaderType ShaderType, HResource Handle, CTexture* pResource) = 0;
	virtual bool				BindSampler(EShaderType ShaderType, HSampler Handle, CSampler* pSampler) = 0;

	virtual bool				BeginFrame() = 0;
	virtual void				EndFrame() = 0;
	virtual void				Clear(UPTR Flags, const vector4& ColorRGBA, float Depth, U8 Stencil) = 0;
	virtual void				ClearRenderTarget(CRenderTarget& RT, const vector4& ColorRGBA) = 0;
	virtual void				ClearDepthStencilBuffer(CDepthStencilBuffer& DS, UPTR Flags, float Depth, U8 Stencil) = 0;
	virtual bool				Draw(const CPrimitiveGroup& PrimGroup) = 0;
	virtual bool				DrawInstanced(const CPrimitiveGroup& PrimGroup, UPTR InstanceCount) = 0;

	//!!!???need copy subresource?! (has meaning for textures only! ArraySlice, MipLevel)
	//???allow copying subresource region? to CopySubresource as optional arg
	//!!!ArraySlice is valid for cubemap faces even in D3D9, with enum consts! need proper indexing!
	//MapSubresource -> OutMappedData (pointer and 2 pitches where applicable), buffers can only return a pointer!
	//UnmapResource returns is still mapped
	//???can make these calls async? copy, map etc.
	//!!!all sync for now until I run async job manager!
	// It is a good rule of thumb to supply at least 16-byte aligned pointers to WriteToResource and ReadFromResource
	virtual bool				MapResource(void** ppOutData, const CVertexBuffer& Resource, EResourceMapMode Mode) = 0;
	virtual bool				MapResource(void** ppOutData, const CIndexBuffer& Resource, EResourceMapMode Mode) = 0;
	virtual bool				MapResource(CImageData& OutData, const CTexture& Resource, EResourceMapMode Mode, UPTR ArraySlice = 0, UPTR MipLevel = 0) = 0;
	virtual bool				UnmapResource(const CVertexBuffer& Resource) = 0;
	virtual bool				UnmapResource(const CIndexBuffer& Resource) = 0;
	virtual bool				UnmapResource(const CTexture& Resource, UPTR ArraySlice = 0, UPTR MipLevel = 0) = 0;
	virtual bool				ReadFromResource(void* pDest, const CVertexBuffer& Resource, UPTR Size = 0, UPTR Offset = 0) = 0;
	virtual bool				ReadFromResource(void* pDest, const CIndexBuffer& Resource, UPTR Size = 0, UPTR Offset = 0) = 0;
	virtual bool				ReadFromResource(const CImageData& Dest, const CTexture& Resource, UPTR ArraySlice = 0, UPTR MipLevel = 0, const Data::CBox* pRegion = nullptr) = 0;
	virtual bool				WriteToResource(CVertexBuffer& Resource, const void* pData, UPTR Size = 0, UPTR Offset = 0) = 0;
	virtual bool				WriteToResource(CIndexBuffer& Resource, const void* pData, UPTR Size = 0, UPTR Offset = 0) = 0;
	virtual bool				WriteToResource(CTexture& Resource, const CImageData& SrcData, UPTR ArraySlice = 0, UPTR MipLevel = 0, const Data::CBox* pRegion = nullptr) = 0;
	virtual bool				WriteToResource(CConstantBuffer& Resource, const void* pData, UPTR Size = 0, UPTR Offset = 0) = 0;
	//virtual PVertexBuffer		CopyResource(const CVertexBuffer& Source, UPTR NewAccessFlags) = 0;
	//virtual PIndexBuffer		CopyResource(const CIndexBuffer& Source, UPTR NewAccessFlags) = 0;
	//virtual PTexture			CopyResource(const CTexture& Source, UPTR NewAccessFlags) = 0;

	//???or auto-begin if not?!
	virtual bool				BeginShaderConstants(CConstantBuffer& Buffer) = 0;
	virtual bool				SetShaderConstant(CConstantBuffer& Buffer, HConstant hConst, UPTR ElementIndex, const void* pData, UPTR Size) = 0;
	virtual bool				CommitShaderConstants(CConstantBuffer& Buffer) = 0;

	virtual CRenderTarget*		GetRenderTarget(UPTR Index) const = 0;
	virtual CDepthStencilBuffer* GetDepthStencilBuffer() const = 0;
};

typedef Ptr<CGPUDriver> PGPUDriver;

}
