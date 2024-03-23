#include "GPURenderablePicker.h"
#include <Frame/View.h>
#include <Frame/RenderPath.h>
#include <Frame/CameraAttribute.h>
#include <Render/Renderable.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Render/Texture.h>
#include <Render/DepthStencilBuffer.h> // at least for a destructor
#include <Render/GPUFence.h>

namespace Frame
{
static const vector4 PickerTargetEmptyValue{ reinterpret_cast<const float&>(INVALID_INDEX_T<U32>), reinterpret_cast<const float&>(INVALID_INDEX_T<U32>), 1.f, 0.f };
static const CStrID sidPickViewProj("PickViewProj");
static const CStrID sidAlphaThreshold("AlphaThreshold");
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
		// Set per-batch constants only with the first instance
		if (!InstanceIndex)
		{
			PerInstanceParams.SetMatrix(PerInstanceParams.GetParamTable().GetConstant(sidPickViewProj), _ViewProj);
			PerInstanceParams.SetFloat(PerInstanceParams.GetParamTable().GetConstant(sidAlphaThreshold), 0.5f); // TODO: make tuneable!
		}
		PerInstanceParams.SetUInt(PerInstanceParams.GetParamTable().GetConstant(sidObjectIndex)[InstanceIndex], _ObjectIndex);
	}
};
//---------------------------------------------------------------------

CGPURenderablePicker::CGPURenderablePicker(Render::CGPUDriver& GPU, std::map<Render::EEffectType, CStrID>&& GPUPickEffects)
	: _GPUPickEffects(std::move(GPUPickEffects))
{
	Render::CRenderTargetDesc RTDesc;
	RTDesc.Width = 1;
	RTDesc.Height = 1;
	RTDesc.Format = Render::PixelFmt_R32G32B32A32_F; // Guaranteed for D3D10 and above
	RTDesc.MSAAQuality = Render::MSAA_None;
	RTDesc.UseAsShaderInput = false;
	RTDesc.MipLevels = 1;
	_RT = GPU.CreateRenderTarget(RTDesc);

	Render::CRenderTargetDesc DSDesc;
	DSDesc.Width = 1;
	DSDesc.Height = 1;
	DSDesc.Format = Render::PixelFmt_DefaultDepthBuffer;
	DSDesc.MSAAQuality = Render::MSAA_None;
	DSDesc.UseAsShaderInput = false;
	DSDesc.MipLevels = 0;
	_DS = GPU.CreateDepthStencilBuffer(DSDesc);
}
//---------------------------------------------------------------------

CGPURenderablePicker::~CGPURenderablePicker() = default;
//---------------------------------------------------------------------

bool CGPURenderablePicker::PickAsync(CPickRequest& AsyncRequest)
{
	auto pView = AsyncRequest.pView;
	if (AsyncRequest.Objects.empty() || !pView || !pView->GetGraphicsScene() || !pView->GetCamera()) return false;

	auto pGPU = pView->GetGPU();
	auto pCamera = pView->GetCamera();

	ZoneScoped;
	DEM_RENDER_EVENT_SCOPED(pGPU, L"CGPURenderablePicker");

	// Bind render targets and a depth-stencil buffer
	pGPU->SetRenderTarget(0, _RT);
	pGPU->SetDepthStencilBuffer(_DS);
	pGPU->SetViewport(0, &Render::GetRenderTargetViewport(_RT->GetDesc()));
	pGPU->ClearRenderTarget(*_RT, PickerTargetEmptyValue);
	pGPU->ClearDepthStencilBuffer(*_DS, Render::Clear_Depth, 1.f, 0);

	// Initialize rendering context
	Render::IRenderer::CRenderContext Ctx;
	Ctx.pGPU = pGPU;
	Ctx.pShaderTechCache = pView->GetGPUPickShaderTechCache();

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
		l += AsyncRequest.RelRect.X * w;
		t -= AsyncRequest.RelRect.Y * h;
		w *= AsyncRequest.RelRect.W;
		h *= AsyncRequest.RelRect.H;

		const auto Proj = Math::matrix_perspective_off_center_rh(l, l + w, t - h, t, pCamera->GetNearPlane(), pCamera->GetFarPlane());
		ViewProj = rtm::matrix_mul(rtm::matrix_cast(pCamera->GetViewMatrix()), Proj);
	}

	// Render hit test candidates to 1x1 target with an override material
	Render::IRenderer* pCurrRenderer = nullptr;
	U8 CurrRendererIndex = 0;
	const auto ObjectCount = AsyncRequest.Objects.size();
	for (U32 i = 0; i < ObjectCount; ++i)
	{
		Render::IRenderable* pRenderable = AsyncRequest.Objects[i].first;
		n_assert_dbg(pRenderable && pRenderable->IsVisible);

		if (CurrRendererIndex != pRenderable->RendererIndex)
		{
			if (pCurrRenderer) pCurrRenderer->EndRange(Ctx);
			CurrRendererIndex = pRenderable->RendererIndex;
			pCurrRenderer = pView->GetRenderer(CurrRendererIndex);
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

	if (!pGPU->ReadFromResource(AsyncRequest.CPUReadableTexture, *_RT)) return false;

	// Issue a fence after the async copy request to know when it is completed
	AsyncRequest.CopyFence = pGPU->SignalFence();

	return true;
}
//---------------------------------------------------------------------

bool CGPURenderablePicker::CPickRequest::IsValid() const
{
	return !Objects.empty();
}
//---------------------------------------------------------------------

bool CGPURenderablePicker::CPickRequest::IsReady() const
{
	return !CPUReadableTexture || !CopyFence || CopyFence->IsSignaled();
}
//---------------------------------------------------------------------

void CGPURenderablePicker::CPickRequest::Wait() const
{
	if (CopyFence) CopyFence->Wait();
}
//---------------------------------------------------------------------

void CGPURenderablePicker::CPickRequest::Get(CPickInfo& Out)
{
	auto pGPU = pView->GetGPU();
	auto pCamera = pView->GetCamera(); //???FIXME: need to copy camera when a request is issued? Tfm might change already!

	// This is not necessary because Map in ReadFromResource will wait if needed
	//CopyFence->Wait();

	// Read back a pick target value containing an intersection info
	struct alignas(16)
	{
		U32   ObjectIndex = INVALID_INDEX_T<U32>;
		float Z = 1.f;
		U16   PackedNormalX = 0;
		U16   PackedNormalY = 0;
		U16   PackedU = 0;
		U16   PackedV = 0;
	} PickTargetData;

	Render::CImageData Dest;
	Dest.pData = reinterpret_cast<char*>(&PickTargetData);
	Dest.RowPitch = CPUReadableTexture->GetRowPitch();
	Dest.SlicePitch = CPUReadableTexture->GetSlicePitch();
	if (pGPU->ReadFromResource(Dest, *CPUReadableTexture) && PickTargetData.ObjectIndex < Objects.size())
	{
		Out.pRenderable = Objects[PickTargetData.ObjectIndex].first;
		Out.UserValue = Objects[PickTargetData.ObjectIndex].second;

		const auto PixelCenter = RelRect.Center();
		const auto WorldPos = pCamera->ReconstructWorldPosition(PixelCenter.x, PixelCenter.y, PickTargetData.Z);
		Out.Position.set(rtm::vector_get_x(WorldPos), rtm::vector_get_y(WorldPos), rtm::vector_get_z(WorldPos));

		Out.Normal.x = Math::HalfToFloat(PickTargetData.PackedNormalX);
		Out.Normal.y = Math::HalfToFloat(PickTargetData.PackedNormalY);
		Out.Normal.z = std::sqrt(1.f - (Out.Normal.x * Out.Normal.x) - (Out.Normal.y * Out.Normal.y));

		Out.TexCoord.x = Math::HalfToFloat(PickTargetData.PackedU);
		Out.TexCoord.y = Math::HalfToFloat(PickTargetData.PackedV);
	}

	// Always clear to indicate that the request is no longer pending (valid)
	Objects.clear();
}
//---------------------------------------------------------------------

}
