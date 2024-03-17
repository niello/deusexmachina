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
static const vector4 PickerTargetEmptyValue{ reinterpret_cast<const float&>(INVALID_INDEX_T<U32>), reinterpret_cast<const float&>(INVALID_INDEX_T<U32>), 1.f, 0.f };
static const CStrID sidInstanceData("InstanceData");
static const CStrID sidObjectIndex("ObjectIndex");

// Set the object index as a shader constant. This works like a kind of a mix-in, providing additional
// shader parameters required by the GPU picker (external rendering facility) alongside any input set.
class CGPUPickRenderModifier : public Render::IRenderModifier
{
public:

	U32 _ObjectIndex = INVALID_INDEX_T<U32>;

	CGPUPickRenderModifier(U32 ObjectIndex) : _ObjectIndex(ObjectIndex) {}

	virtual void ModifyPerInstanceShaderParams(Render::CShaderParamStorage& PerInstanceParams, UPTR InstanceIndex) override
	{
		PerInstanceParams.SetUInt(PerInstanceParams.GetParamTable().GetConstant(sidObjectIndex)[InstanceIndex], _ObjectIndex);
	}
};

CGPURenderablePicker::CGPURenderablePicker(CView& View, std::map<Render::EEffectType, CStrID>&& GPUPickEffects)
	: _GPUPickEffects(std::move(GPUPickEffects))
{
	Render::CRenderTargetDesc RTDesc;
	RTDesc.Width = 1;
	RTDesc.Height = 1;
	RTDesc.Format = Render::PixelFmt_R32G32B32A32_F; // Guaranteed for D3D10 and above
	RTDesc.MSAAQuality = Render::MSAA_None;
	RTDesc.UseAsShaderInput = false;
	RTDesc.MipLevels = 1;
	_RT = View.GetGPU()->CreateRenderTarget(RTDesc);

	Render::CRenderTargetDesc DSDesc;
	DSDesc.Width = 1;
	DSDesc.Height = 1;
	DSDesc.Format = Render::PixelFmt_DefaultDepthBuffer;
	DSDesc.MSAAQuality = Render::MSAA_None;
	DSDesc.UseAsShaderInput = false;
	DSDesc.MipLevels = 0;
	_DS = View.GetGPU()->CreateDepthStencilBuffer(DSDesc);
}
//---------------------------------------------------------------------

CGPURenderablePicker::~CGPURenderablePicker() = default;
//---------------------------------------------------------------------

bool CGPURenderablePicker::Pick(const CView& View, float x, float y, const std::pair<Render::IRenderable*, UPTR>* pObjects, U32 ObjectCount, UPTR ShaderTechCacheIndex)
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
	pGPU->ClearRenderTarget(*_RT, PickerTargetEmptyValue);

	// Initialize rendering context
	Render::IRenderer::CRenderContext Ctx;
	Ctx.pGPU = pGPU;
	Ctx.pShaderTechCache = View.GetShaderTechCache(ShaderTechCacheIndex);
	Ctx.CameraPosition = pCamera->GetPosition();

	// Calculate a view-projection matrix to render only the requested pixel
	{
		n_assert_dbg(!pCamera->IsOrthographic());

		// First calculate params of a full projection matrix
		float t = pCamera->GetNearPlane() * rtm::scalar_tan(pCamera->GetFOV() * 0.5f);
		float h = t + t;
		float w = pCamera->GetAspectRatio() * h;
		float l = -0.5f * w;

		// And then crop to a single pixel at the requested position
		//???!!!turn RenderTargetID into index in Init?! target can change size, can't cache pixel size. But can cache index!
		//???or pass relative x, y and pixel size here?! anyway calculated for GetRay3D for coarse testing!
		//!!!DBG TMP!
		CStrID RenderTargetID("Main");
		if (auto pTarget = View.GetRenderTarget(RenderTargetID))
		{
			const vector2 PixelSize = Render::GetRenderTargetPixelSize(pTarget->GetDesc());
			l += x * PixelSize.x * w;
			t -= y * PixelSize.y * h;
			w *= PixelSize.x;
			h *= PixelSize.y;
		}

		const auto Proj = Math::matrix_perspective_off_center_rh(l, l + w, t - h, t, pCamera->GetNearPlane(), pCamera->GetFarPlane());

		Ctx.ViewProjection = rtm::matrix_mul(rtm::matrix_cast(pCamera->GetViewMatrix()), Proj);
	}

	// Render hit test candidates to 1x1 target with an override material
	Render::IRenderer* pCurrRenderer = nullptr;
	U8 CurrRendererIndex = 0;
	for (U32 i = 0; i < ObjectCount; ++i)
	{
		Render::IRenderable* pRenderable = pObjects[i].first;
		n_assert_dbg(pRenderable && pRenderable->IsVisible);

		if (CurrRendererIndex != pRenderable->RendererIndex)
		{
			if (pCurrRenderer) pCurrRenderer->EndRange(Ctx);
			CurrRendererIndex = pRenderable->RendererIndex;
			pCurrRenderer = View.GetRenderer(CurrRendererIndex);
			if (pCurrRenderer)
				if (!pCurrRenderer->BeginRange(Ctx))
					pCurrRenderer = nullptr;
		}

		if (pCurrRenderer) pCurrRenderer->Render(Ctx, *pRenderable, &CGPUPickRenderModifier(i));
	}
	if (pCurrRenderer) pCurrRenderer->EndRange(Ctx);

	// Unbind render target(s) & DS buffer
	pGPU->SetRenderTarget(0, nullptr);
	pGPU->SetDepthStencilBuffer(nullptr);

	// Read back a pick target value containing an intersection info
	//!!!FIXME: synchronous, just for testing!
	alignas(16) struct
	{
		U32   ObjectIndex = INVALID_INDEX_T<U32>;
		U32   TriangleIndex = INVALID_INDEX_T<U32>;
		float Z = 1.f;
		U32   UNUSED = 0; //??? 2xfloat16 for a normal?
	} PickInfo;
	//Render::CImageData Dest;
	//Dest.pData = reinterpret_cast<char*>(&PickInfo);
	//Dest.RowPitch = RT->GetTexture()->GetRowPitch();
	//Dest.SlicePitch = RT->GetTexture()->GetSlicePitch();
	//pGPU->ReadFromResource(Dest, RT->GetTexture());
	//_RT->CopyResolveToTexture???
	//???for each async request allocate a new staging texture if previous one wasn't yet read and released by the caller? reuse if released?

	//!!!
	// Don't forget that with D3D10 and up, it is possible to interpret data in groovy ways. The shader intrinsics starting
	// with "as" can be used to convert the representation of data from one data type to other. This means that you could carry
	// integer data in your buffer's(texture's) channels that are typed as float.
	// https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-asuint

	//!!!
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

	OK;
}
//---------------------------------------------------------------------

}
