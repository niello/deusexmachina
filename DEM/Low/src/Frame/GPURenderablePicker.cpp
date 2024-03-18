#include "GPURenderablePicker.h"
#include <Frame/View.h>
#include <Frame/RenderPath.h>
#include <Frame/CameraAttribute.h>
#include <Render/Renderable.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Render/Texture.h>
#include <Render/DepthStencilBuffer.h> // at least for a destructor

namespace Frame
{
static const vector4 PickerTargetEmptyValue{ reinterpret_cast<const float&>(INVALID_INDEX_T<U32>), reinterpret_cast<const float&>(INVALID_INDEX_T<U32>), 1.f, 0.f };
static const CStrID sidPickViewProj("PickViewProj");
static const CStrID sidObjectIndex("ObjectIndex");

// Set the object index as a shader constant. This works like a kind of a mix-in, providing additional
// shader parameters required by the GPU picker (external rendering facility) alongside any input set.
class CGPUPickRenderModifier : public Render::IRenderModifier
{
public:

	const rtm::matrix4x4f& _ViewProj;
	U32                    _ObjectIndex = INVALID_INDEX_T<U32>;

	CGPUPickRenderModifier(const rtm::matrix4x4f& ViewProj, U32 ObjectIndex) : _ViewProj(ViewProj), _ObjectIndex(ObjectIndex) {}

	virtual void ModifyPerInstanceShaderParams(Render::CShaderParamStorage& PerInstanceParams, UPTR InstanceIndex) override
	{
		// Set matrix only with the first instance
		if (!InstanceIndex)
			PerInstanceParams.SetMatrix(PerInstanceParams.GetParamTable().GetConstant(sidPickViewProj), _ViewProj);
		PerInstanceParams.SetUInt(PerInstanceParams.GetParamTable().GetConstant(sidObjectIndex)[InstanceIndex], _ObjectIndex);
	}
};
//---------------------------------------------------------------------

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

CGPURenderablePicker::CPickInfo CGPURenderablePicker::Pick(const CView& View, const Data::CRectF& RelRect, const std::pair<Render::IRenderable*, UPTR>* pObjects, U32 ObjectCount, UPTR ShaderTechCacheIndex)
{
	auto pGPU = View.GetGPU();
	auto pCamera = View.GetCamera();

	ZoneScoped;
	DEM_RENDER_EVENT_SCOPED(pGPU, L"CGPURenderablePicker");

	CPickInfo PickInfo;

	if (!View.GetGraphicsScene() || !pCamera) return PickInfo;

	// Bind render targets and a depth-stencil buffer
	pGPU->SetRenderTarget(0, _RT);
	pGPU->SetDepthStencilBuffer(_DS);
	pGPU->SetViewport(0, &Render::GetRenderTargetViewport(_RT->GetDesc()));
	pGPU->ClearRenderTarget(*_RT, PickerTargetEmptyValue);
	pGPU->ClearDepthStencilBuffer(*_DS, Render::Clear_Depth, 1.f, 0);

	// Initialize rendering context
	Render::IRenderer::CRenderContext Ctx;
	Ctx.pGPU = pGPU;
	Ctx.pShaderTechCache = View.GetShaderTechCache(ShaderTechCacheIndex);

	// Calculate a view-projection matrix to render only the requested rect (typically a single pixel)
	rtm::matrix4x4f ViewProj;
	{
		n_assert_dbg(!pCamera->IsOrthographic());

		// First calculate params of a full projection matrix
		float t = pCamera->GetNearPlane() * rtm::scalar_tan(pCamera->GetFOV() * 0.5f);
		float h = t + t;
		float w = pCamera->GetAspectRatio() * h;
		float l = -0.5f * w;

		// And then crop to the requested rect
		l += RelRect.X * w;
		t -= RelRect.Y * h;
		w *= RelRect.W;
		h *= RelRect.H;

		const auto Proj = Math::matrix_perspective_off_center_rh(l, l + w, t - h, t, pCamera->GetNearPlane(), pCamera->GetFarPlane());
		ViewProj = rtm::matrix_mul(rtm::matrix_cast(pCamera->GetViewMatrix()), Proj);
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

		if (pCurrRenderer) pCurrRenderer->Render(Ctx, *pRenderable, &CGPUPickRenderModifier(ViewProj, i));
	}
	if (pCurrRenderer) pCurrRenderer->EndRange(Ctx);

	// Unbind render target(s) & DS buffer
	pGPU->SetRenderTarget(0, nullptr);
	pGPU->SetDepthStencilBuffer(nullptr);

	// Read back a pick target value containing an intersection info

	//???!!!what to do with multiple calls in flight?
	//???store textures internally and return a future with already read structure to the caller?
	//when the caller reads the future, the corresponding texture is locked, read and discarded to the pool, or deleted.
	//???how to discard a texture if its future was abandoned?!
	Render::PTexture CPUReadableTexture;
	if (!pGPU->ReadFromResource(CPUReadableTexture, *_RT)) return PickInfo;

	//!!!FIXME PERF: stall is right here! Must give GPU time for working async on our request!

	Render::CImageData Dest;
	Dest.pData = reinterpret_cast<char*>(&PickInfo);
	Dest.RowPitch = CPUReadableTexture->GetRowPitch();
	Dest.SlicePitch = CPUReadableTexture->GetSlicePitch();
	if (!pGPU->ReadFromResource(Dest, *CPUReadableTexture)) return PickInfo;

	//!!!TODO: return future? Or wait for 2 frames, see MSDN. Could use D3D11.3 fence: ID3D11Fence::SetEventOnCompletion + WaitForSingleObject,
	//or older widely supported ID3D11Query of type D3D11_QUERY_EVENT

	return PickInfo;
}
//---------------------------------------------------------------------

}
