#pragma once
#ifndef __DEM_L1_RENDER_GPU_DRIVER_H__
#define __DEM_L1_RENDER_GPU_DRIVER_H__

#include <Core/Object.h>
#include <Render/RenderFwd.h>
#include <Render/VertexComponent.h>
#include <Data/Dictionary.h>
#include <Data/StringID.h>
#include <Data/Regions.h>

// GPU device driver manages VRAM resources and provides an interface for rendering on a video card.
// Create GPU device drivers with CVideoDriverFactory.
// You can implement this class via some graphics API, like D3D or OpenGL.
// Each GPU device can manage zero or more swap chains, either windowed or fullscreen. Each fullscreen
// swap chain must be connected to a corresponding display output, represented by a CDisplayDriver instance.
// Windowed swap chains are present to the desktop. Each swap chain requires a viewport, and now
// I use COSWindow for it.

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

namespace Sys
{
	typedef Ptr<class COSWindow> POSWindow;
}

namespace Render
{

class CGPUDriver: public Core::CObject
{
protected:

	DWORD							AdapterID;
	EGPUDriverType					Type;

//	DWORD							InstanceCount;	// If 0, non-instanced rendering is active //???if 1? instanced shader + 1 inst data?

	//DWORD							PrimsRendered;
	//DWORD							DIPsRendered;

	static void					PrepareWindowAndBackBufferSize(Sys::COSWindow& Window, UINT& Width, UINT& Height);

public:

	CGPUDriver() {}
	virtual ~CGPUDriver() {}

	virtual bool				Init(DWORD AdapterNumber, EGPUDriverType DriverType) { AdapterID = AdapterNumber; OK; }
	virtual bool				CheckCaps(ECaps Cap) = 0;
	virtual DWORD				GetMaxVertexStreams() = 0;
	virtual DWORD				GetMaxTextureSize(ETextureType Type) = 0;
	virtual DWORD				GetMaxMultipleRenderTargetCount() = 0;

	virtual int					CreateSwapChain(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, Sys::COSWindow* pWindow) = 0;
	virtual bool				DestroySwapChain(DWORD SwapChainID) = 0;
	virtual bool				SwapChainExists(DWORD SwapChainID) const = 0;
	virtual bool				SwitchToFullscreen(DWORD SwapChainID, CDisplayDriver* pDisplay = NULL, const CDisplayMode* pMode = NULL) = 0;
	virtual bool				SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect = NULL) = 0;
	virtual bool				ResizeSwapChain(DWORD SwapChainID, unsigned int Width, unsigned int Height) = 0;
	virtual bool				IsFullscreen(DWORD SwapChainID) const = 0;
	virtual PRenderTarget		GetSwapChainRenderTarget(DWORD SwapChainID) const = 0;
	virtual bool				Present(DWORD SwapChainID) = 0;
	bool						PresentBlankScreen(DWORD SwapChainID, const vector4& ColorRGBA);
	virtual bool				CaptureScreenshot(DWORD SwapChainID, IO::CStream& OutStream) const = 0;

	virtual PVertexLayout		CreateVertexLayout(const CVertexComponent* pComponents, DWORD Count) = 0;
	virtual PVertexBuffer		CreateVertexBuffer(CVertexLayout& VertexLayout, DWORD VertexCount, DWORD AccessFlags, const void* pData = NULL) = 0;
	virtual PIndexBuffer		CreateIndexBuffer(EIndexType IndexType, DWORD IndexCount, DWORD AccessFlags, const void* pData = NULL) = 0;
	virtual PRenderState		CreateRenderState(const CRenderStateDesc& Desc) = 0;
	//???CreateShader? instead of D3D11 API code in loaders?
	virtual PConstantBuffer		CreateConstantBuffer(const CShader& Shader, CStrID ID, DWORD AccessFlags, const void* pData = NULL) = 0;
	virtual PTexture			CreateTexture(const CTextureDesc& Desc, DWORD AccessFlags, const void* pData = NULL, bool MipDataProvided = false) = 0;
	virtual PSampler			CreateSampler(const CSamplerDesc& Desc) = 0;
	virtual PRenderTarget		CreateRenderTarget(const CRenderTargetDesc& Desc) = 0;
	virtual PDepthStencilBuffer	CreateDepthStencilBuffer(const CRenderTargetDesc& Desc) = 0;

	virtual bool				SetViewport(DWORD Index, const CViewport* pViewport) = 0; // NULL to reset
	virtual bool				GetViewport(DWORD Index, CViewport& OutViewport) = 0;
	virtual bool				SetScissorRect(DWORD Index, const Data::CRect* pScissorRect) = 0; // NULL to reset
	virtual bool				GetScissorRect(DWORD Index, Data::CRect& OutScissorRect) = 0;

	virtual bool				SetVertexLayout(CVertexLayout* pVLayout) = 0;
	virtual bool				SetVertexBuffer(DWORD Index, CVertexBuffer* pVB, DWORD OffsetVertex = 0) = 0;
	virtual bool				SetIndexBuffer(CIndexBuffer* pIB) = 0;
	//virtual bool				SetInstanceBuffer(DWORD Index, CVertexBuffer* pVB, DWORD Instances, DWORD OffsetVertex = 0) = 0;
	virtual bool				SetRenderState(CRenderState* pState) = 0;
	virtual bool				SetRenderTarget(DWORD Index, CRenderTarget* pRT) = 0;
	virtual bool				SetDepthStencilBuffer(CDepthStencilBuffer* pDS) = 0;

	virtual bool				BeginShaderConstants(CConstantBuffer& CBuffer) = 0;
	virtual bool				SetShaderConstants(CConstantBuffer& CBuffer, DWORD Offset, void const* const pData, DWORD Size) = 0; //???offset or HConst Address?
	template<typename T> bool	SetShaderConstant(CConstantBuffer& CBuffer, DWORD Offset, const T& Data);
	template<typename T> bool	SetShaderConstantArray(CConstantBuffer& CBuffer, DWORD Offset, const T* pData, DWORD Count);
	virtual void				EndShaderConstants(CConstantBuffer& CBuffer) = 0;
	virtual bool				BindConstantBuffer(EShaderType ShaderType, HConstBuffer Handle, CConstantBuffer* pCBuffer) = 0;
	virtual bool				BindResource(EShaderType ShaderType, HResource Handle, CTexture* pResource) = 0;
	virtual bool				BindSampler(EShaderType ShaderType, HSampler Handle, CSampler* pSampler) = 0;

	virtual bool				BeginFrame() = 0;
	virtual void				EndFrame() = 0;
	virtual void				Clear(DWORD Flags, const vector4& ColorRGBA, float Depth, uchar Stencil) = 0;
	virtual void				ClearRenderTarget(CRenderTarget& RT, const vector4& ColorRGBA) = 0;
	virtual bool				Draw(const CPrimitiveGroup& PrimGroup) = 0;

	//!!!constant buffers are not present in D3D9 and are handled differently
	//!!!???need copy subresource?! (has meaning for textures only! ArraySlice, MipLevel)
	//???allow copying subresource region? to CopySubresource as optional arg
	//!!!ArraySlice is valid for cubemap faces even in D3D9, with enum consts! need proper indexing!
	//MapSubresource -> OutMappedData (pointer and 2 pitches where applicable), buffers can only return a pointer!
	//UnmapResource returns is still mapped
	//???how to handle texture-less render target? allow it at all? RT without texture may not be mapped?!
	//???can make these calls async? copy, map etc.
	//!!!all sync for now until I run async job manager!
	//???for read and write optional callback for memory copying? with memcpy by default. may use for aligned copying with SSE
	// It is a good rule of thumb to supply at least 16-byte aligned pointers to WriteToResource and ReadFromResource
	virtual bool				MapResource(void** ppOutData, const CVertexBuffer& Resource, EResourceMapMode Mode) = 0;
	virtual bool				MapResource(void** ppOutData, const CIndexBuffer& Resource, EResourceMapMode Mode) = 0;
	virtual bool				MapResource(CImageData& OutData, const CTexture& Resource, EResourceMapMode Mode, DWORD ArraySlice = 0, DWORD MipLevel = 0) = 0;
	virtual bool				UnmapResource(const CVertexBuffer& Resource) = 0;
	virtual bool				UnmapResource(const CIndexBuffer& Resource) = 0;
	virtual bool				UnmapResource(const CTexture& Resource, DWORD ArraySlice = 0, DWORD MipLevel = 0) = 0;
	virtual bool				ReadFromResource(void* pDest, const CVertexBuffer& Resource, DWORD Size = 0, DWORD Offset = 0) = 0;
	virtual bool				ReadFromResource(void* pDest, const CIndexBuffer& Resource, DWORD Size = 0, DWORD Offset = 0) = 0;
	virtual bool				ReadFromResource(const CImageData& Dest, const CTexture& Resource, DWORD ArraySlice = 0, DWORD MipLevel = 0, const Data::CBox* pRegion = NULL) = 0;
	virtual bool				WriteToResource(CVertexBuffer& Resource, const void* pData, DWORD Size = 0, DWORD Offset = 0) = 0;
	virtual bool				WriteToResource(CIndexBuffer& Resource, const void* pData, DWORD Size = 0, DWORD Offset = 0) = 0;
	virtual bool				WriteToResource(CTexture& Resource, const CImageData& SrcData, DWORD ArraySlice = 0, DWORD MipLevel = 0, const Data::CBox* pRegion = NULL) = 0;
	//virtual PVertexBuffer		CopyResource(const CVertexBuffer& Source, DWORD NewAccessFlags) = 0;
	//virtual PIndexBuffer		CopyResource(const CIndexBuffer& Source, DWORD NewAccessFlags) = 0;
	//virtual PTexture			CopyResource(const CTexture& Source, DWORD NewAccessFlags) = 0;

	EGPUDriverType				GetType() const { return Type; }
};

typedef Ptr<CGPUDriver> PGPUDriver;

inline bool CGPUDriver::PresentBlankScreen(DWORD SwapChainID, const vector4& ColorRGBA)
{
	if (BeginFrame())
	{
		Clear(Clear_Color, ColorRGBA, 1.f, 0);
		EndFrame();
		Present(SwapChainID);
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

template<typename T> inline bool CGPUDriver::SetShaderConstant(CConstantBuffer& CBuffer, DWORD Offset, const T& Data)
{
	return SetShaderConstants(CBuffer, Offset, &Data, sizeof(T));
}
//---------------------------------------------------------------------

template<typename T> inline bool CGPUDriver::SetShaderConstantArray(CConstantBuffer& CBuffer, DWORD Offset, const T* pData, DWORD Count)
{
	return SetShaderConstants(CBuffer, Offset, pData, Count * sizeof(T));
}
//---------------------------------------------------------------------

}

#endif
