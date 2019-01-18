#include "RenderPathLoaderRP.h"

#include <Frame/RenderPath.h>
#include <Frame/RenderPhase.h>
#include <Render/D3D9/SM30ShaderMetadata.h>
#include <Render/D3D11/USMShaderMetadata.h>
#include <IO/BinaryReader.h>
#include <Data/DataArray.h>
#include <Core/Factory.h>

namespace Resources
{
// Defined in Render/EffectLoadingUtils.cpp
bool LoadEffectParams(IO::CBinaryReader& Reader, Render::CShaderLibrary* pShaderLibrary, const Render::IShaderMetadata* pDefaultShaderMeta, CFixedArray<Render::CEffectConstant>& OutConsts, CFixedArray<Render::CEffectResource>& OutResources, CFixedArray<Render::CEffectSampler>& OutSamplers);

const Core::CRTTI& CRenderPathLoaderRP::GetResultType() const
{
	return Frame::CRenderPath::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CRenderPathLoaderRP::CreateResource(CStrID UID)
{
	const char* pSubId;
	IO::PStream Stream = OpenStream(UID, pSubId);
	if (!Stream) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'RPTH') return NULL;

	U32 Version;
	if (!Reader.Read<U32>(Version)) return NULL;

	U32 ShaderModel;
	if (!Reader.Read<U32>(ShaderModel)) return NULL; // 0 for SM3.0, 1 for USM

	Data::CDataArray RTSlots;
	if (!Reader.Read(RTSlots)) return NULL;

	Data::CDataArray DSSlots;
	if (!Reader.Read(DSSlots)) return NULL;

	Data::PParams Phases = n_new(Data::CParams);
	if (!Reader.ReadParams(*Phases)) return NULL;

	Frame::PRenderPath RP = n_new(Frame::CRenderPath);

	RP->RTSlots.SetSize(RTSlots.GetCount());
	for (UPTR i = 0; i < RTSlots.GetCount(); ++i)
	{
		Frame::CRenderPath::CRenderTargetSlot& Slot = RP->RTSlots[i];
		Slot.ClearValue = RTSlots[i].GetValue<Data::PParams>()->Get<vector4>(CStrID("ClearValue"), vector4(0.5f, 0.5f, 0.f, 1.f));
	}

	RP->DSSlots.SetSize(DSSlots.GetCount());
	for (UPTR i = 0; i < DSSlots.GetCount(); ++i)
	{
		Frame::CRenderPath::CDepthStencilSlot& Slot = RP->DSSlots[i];
		Data::PParams SlotParams = DSSlots[i].GetValue<Data::PParams>();

		Slot.ClearFlags = 0;

		float ZClear;
		if (SlotParams->Get(ZClear, CStrID("DepthClearValue")))
		{
			Slot.DepthClearValue = ZClear;
			Slot.ClearFlags |= Render::Clear_Depth;
		}

		int StencilClear;
		if (SlotParams->Get(StencilClear, CStrID("StencilClearValue")))
		{
			Slot.StencilClearValue = StencilClear;
			Slot.ClearFlags |= Render::Clear_Stencil;
		}
	}

	// Breaks API independence. Honestly, RP file format breaks it even earlier,
	// by including API-specific metadata. Subject to redesign.
	switch (ShaderModel)
	{
		case 0: // SM3.0
		{
			Render::CSM30ShaderMetadata* pGlobals = n_new(Render::CSM30ShaderMetadata);
			if (!pGlobals->Load(*Stream)) return NULL;
			RP->pGlobals = pGlobals;
			break;
		}
		case 1: // USM
		{
			Render::CUSMShaderMetadata* pGlobals = n_new(Render::CUSMShaderMetadata);
			if (!pGlobals->Load(*Stream)) return NULL;
			RP->pGlobals = pGlobals;
			break;
		}
	}

	if (!LoadEffectParams(Reader, nullptr, RP->pGlobals, RP->Consts, RP->Resources, RP->Samplers)) return NULL;

	// Phases are intentionally initialized at the end, because they may access global params etc
	RP->Phases.SetSize(Phases->GetCount());

	for (UPTR i = 0; i < Phases->GetCount(); ++i)
	{
		const Data::CParam& Prm = Phases->Get(i);
		Data::CParams& PhaseDesc = *Prm.GetValue<Data::PParams>();

		const CString& PhaseType = PhaseDesc.Get<CString>(CStrID("Type"), CString::Empty); //???use FourCC in PRM?
		CString ClassName = "Frame::CRenderPhase" + PhaseType;
		Frame::PRenderPhase CurrPhase = (Frame::CRenderPhase*)Factory->Create(ClassName.CStr());

		if (!CurrPhase->Init(*RP.Get(), Prm.GetName(), PhaseDesc)) return NULL;

		RP->Phases[i] = CurrPhase;
	}

	return RP.Get();
}
//---------------------------------------------------------------------

}