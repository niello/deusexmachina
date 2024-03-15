#include "GPURenderablePicker.h"
#include <Frame/View.h>
#include <Frame/RenderPath.h>
#include <Frame/CameraAttribute.h>
#include <Render/Renderable.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Render/DepthStencilBuffer.h> // at least for a destructor

namespace Frame
{

CGPURenderablePicker::~CGPURenderablePicker() = default;
//---------------------------------------------------------------------

//!!!in - vector of renderables associated with user data, out - index or renderable or user data of the picked one, or empty for no pick!
bool CGPURenderablePicker::Render(CView& View)
{
	auto pGPU = View.GetGPU();
	auto pCamera = View.GetCamera();

	ZoneScoped;
	DEM_RENDER_EVENT_SCOPED(pGPU, L"CGPURenderablePicker");

	if (!View.GetGraphicsScene() || !pCamera) OK;

	// Bind render targets and a depth-stencil buffer

	pGPU->SetRenderTarget(0, _RT);
	pGPU->SetDepthStencilBuffer(_DS);
	pGPU->SetViewport(0, &Render::GetRenderTargetViewport(_RT->GetDesc()));

	Render::IRenderer::CRenderContext Ctx;
	Ctx.pGPU = pGPU;
	Ctx.pShaderTechCache = View.GetShaderTechCache(_ShaderTechCacheIndex);
	Ctx.CameraPosition = pCamera->GetPosition();

	// Calculate view-projection matrix to render only the requested pixel
	{
		float t = pCamera->GetNearPlane() * rtm::scalar_tan(pCamera->GetFOV() * 0.5f);
		float h = t + t;
		float w = pCamera->GetAspectRatio() * h;
		float l = -0.5f * w;

		//!!!need to calculate view region from the pixel and actual view target size (Main target, where to define its ID, pass on creation?)!
		//const vector2 PixelSize = View.GetRenderTarget(SomeID).GetPixelSize();
		//l += Point.x * PixelSize.x * w;
		//t -= Point.y * PixelSize.y * h;
		//w *= PixelSize.x;
		//h *= PixelSize.y;

		const auto Proj = Math::matrix_perspective_off_center_rh(l, l + w, t - h, t, pCamera->GetNearPlane(), pCamera->GetFarPlane());

		Ctx.ViewProjection = rtm::matrix_mul(rtm::matrix_cast(pCamera->GetViewMatrix()), Proj);
	}

	Render::IRenderer* pCurrRenderer = nullptr;
	U8 CurrRendererIndex = 0;
	/*
	for (Render::IRenderable* pRenderable : Renderables)
	{
		n_assert_dbg(pRenderable->IsVisible);

		if (CurrRendererIndex != pRenderable->RendererIndex)
		{
			if (pCurrRenderer) pCurrRenderer->EndRange(Ctx);
			CurrRendererIndex = pRenderable->RendererIndex;
			pCurrRenderer = View.GetRenderer(CurrRendererIndex);
			if (pCurrRenderer)
				if (!pCurrRenderer->BeginRange(Ctx))
					pCurrRenderer = nullptr;
		}

		if (pCurrRenderer) pCurrRenderer->Render(Ctx, *pRenderable);
	}
	*/
	if (pCurrRenderer) pCurrRenderer->EndRange(Ctx);

	//???need to call something to finalize rendering to target? how to wait for it to start a readback?

	// Unbind render target(s) & DS buffer
	pGPU->SetRenderTarget(0, nullptr);
	pGPU->SetDepthStencilBuffer(nullptr);

	OK;
}
//---------------------------------------------------------------------

//!!!TODO: must pass here a set of effect overrides! can control alpha bleanded and alpha tested picking behaviour with different overrides!
bool CGPURenderablePicker::Init(CView& View, std::map<Render::EEffectType, CStrID>&& EffectOverrides)
{
	// RT formats with guaranteed support from D3D10:
	// https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/format-support-for-direct3d-feature-level-10-0-hardware
	// DXGI_FORMAT_R32G32B32A32_FLOAT, UINT, SINT
	// DXGI_FORMAT_R16G16B16A16_FLOAT, UNORM, UINT, SNORM, SINT
	// DXGI_FORMAT_R32G32_FLOAT, UINT, SINT
	// DXGI_FORMAT_R10G10B10A2_UNORM, UINT
	// DXGI_FORMAT_R11G11B10_FLOAT
	// DXGI_FORMAT_R16G16_FLOAT, UNORM, UINT, SNORM, SINT
	// DXGI_FORMAT_R32_FLOAT, UINT, SINT
	// DXGI_FORMAT_B5G6R5_UNORM

	// Create 1x1 render target or multiple, able to contain:
	// - object ID (U32, index in a rendered vector, not a renderable attr's global 64-bit UID)
	// - triangle ID(U32 from uint SV_PrimitiveId. Do really need? Will correspond to order in a mesh VB/IB data?)
	// - Z coord (normalized F32, can be quantized to U32? or written bitwise? or prepared for real world depth restore?)
	// - normal? or restore from triangle on CPU? don't sample normal map anyway in picking shaders? could use last U32 as 2xF16.

	// Don't forget that with D3D10 and up, it is possible to interpret data in groovy ways. The shader intrinsics starting
	// with "as" can be used to convert the representation of data from one data type to other. This means that you could carry
	// integer data in your buffer's(texture's) channels that are typed as float.
	// https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-asuint

	Render::CRenderTargetDesc RTDesc;
	RTDesc.Width = 1;
	RTDesc.Height = 1;
	RTDesc.Format = Render::PixelFmt_R32G32B32A32_F;
	RTDesc.MSAAQuality = Render::MSAA_None;
	RTDesc.UseAsShaderInput = false;
	RTDesc.MipLevels = 1;
	_RT = View.GetGPU()->CreateRenderTarget(RTDesc);

	// https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-mapping
	// https://chromium.googlesource.com/angle/angle/+/a787b6187a134c64cc9c336a2cdd20ed08778eb2/src/libANGLE/renderer/d3d/d3d11/Buffer11.cpp
	// https://learn.microsoft.com/en-us/windows/win32/direct3d12/readback-data-using-heaps
	// https://stackoverflow.com/questions/13479259/read-pixel-data-from-render-target-in-d3d11
	//
	// Asynchronous copy: immediateContext->CopySubresourceRegion(mStagingTexture.get(), ...)
	// then give some time for GPU to finish job, and then:
	// immediateContext->Map(mStagingTexture.get(), _mip, D3D11_MAP_READ, 0, &mapped)
	//
	//!!!TODO: improve ReadFromD3DBuffer, now copies and then maps immediately, guaranteed stall! Return future instead? Or wait for 2 frames, see MSDN.
	//If your app needs to copy an entire resource, we recommend to use ID3D11DeviceContext::CopyResource instead. (!!!)
	// Could use D3D11.3 fence: ID3D11Fence::SetEventOnCompletion + WaitForSingleObject, or older widely supported ID3D11Query of type D3D11_QUERY_EVENT
	//And always creates a new staging resource, we could create it once here!
	//???can run async in another thread and wait on future until available. Or use job counter for waiting and release-acquire?
	//
	//!!!mapped data will be align16, can load into SSE register with movaps if needed!

	Render::CRenderTargetDesc DSDesc;
	DSDesc.Width = 1;
	DSDesc.Height = 1;
	DSDesc.Format = Render::PixelFmt_DefaultDepthBuffer;
	DSDesc.MSAAQuality = Render::MSAA_None;
	DSDesc.UseAsShaderInput = false;
	DSDesc.MipLevels = 0;
	_DS = View.GetGPU()->CreateDepthStencilBuffer(DSDesc);

	//!!!need to refresh the cache in CView because now it is fixed at the start!!!
	//or create picker in CView constructor and deny lazy creation? then pass _ShaderTechCacheIndex here.
	View.GetRenderPath()->EffectOverrides.push_back(std::move(EffectOverrides));
	_ShaderTechCacheIndex = View.GetRenderPath()->EffectOverrides.size();

	OK;
}
//---------------------------------------------------------------------

}
