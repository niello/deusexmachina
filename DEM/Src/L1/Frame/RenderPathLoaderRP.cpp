#include "RenderPathLoaderRP.h"

#include <Frame/RenderPath.h>
#include <Frame/RenderPhase.h>
#include <Render/D3D9/SM30ShaderMetadata.h>
#include <Render/D3D11/USMShaderMetadata.h>
#include <IO/BinaryReader.h>
#include <Core/Factory.h>

namespace Resources
{
__ImplementClass(Resources::CRenderPathLoaderRP, 'RPLD', Resources::CResourceLoader);

// Defined in Render/EffectLoadingUtils.cpp
bool LoadEffectParams(IO::CBinaryReader& Reader, Render::PShaderLibrary ShaderLibrary, const Render::IShaderMetadata* pDefaultShaderMeta, CFixedArray<Render::CEffectConstant>& OutConsts, CFixedArray<Render::CEffectResource>& OutResources, CFixedArray<Render::CEffectSampler>& OutSamplers);

const Core::CRTTI& CRenderPathLoaderRP::GetResultType() const
{
	return Frame::CRenderPath::RTTI;
}
//---------------------------------------------------------------------

PResourceObject CRenderPathLoaderRP::Load(IO::CStream& Stream)
{
	IO::CBinaryReader Reader(Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'RPTH') return NULL;

	U32 Version;
	if (!Reader.Read<U32>(Version)) return NULL;

	U32 ShaderModel;
	if (!Reader.Read<U32>(ShaderModel)) return NULL; // 0 for SM3.0, 1 for USM

	Data::PParams Phases = n_new(Data::CParams);
	if (!Reader.ReadParams(*Phases)) return NULL;

	Frame::PRenderPath RP = n_new(Frame::CRenderPath);

	RP->Phases.SetSize(Phases->GetCount());
	for (UPTR i = 0; i < Phases->GetCount(); ++i)
	{
		const Data::CParam& Prm = Phases->Get(i);
		Data::CParams& PhaseDesc = *Prm.GetValue<Data::PParams>();

		const CString& PhaseType = PhaseDesc.Get<CString>(CStrID("Type"), CString::Empty); //???use FourCC in PRM?
		CString ClassName = "Frame::CRenderPhase" + PhaseType;
		Frame::PRenderPhase CurrPhase = (Frame::CRenderPhase*)Factory->Create(ClassName.CStr());

		if (!CurrPhase->Init(Prm.GetName(), PhaseDesc)) return NULL;

		RP->Phases[i] = CurrPhase;
	}

	// Breaks API independence. Honestly, RP file format breaks it even earlier,
	// by including API-specific metadata. Subject to redesign.
	switch (ShaderModel)
	{
		case 0: // SM3.0
		{
			Render::CSM30ShaderMetadata* pGlobals = n_new(Render::CSM30ShaderMetadata);
			if (!pGlobals->Load(Stream)) return NULL;
			RP->pGlobals = pGlobals;
			break;
		}
		case 1: // USM
		{
			Render::CUSMShaderMetadata* pGlobals = n_new(Render::CUSMShaderMetadata);
			if (!pGlobals->Load(Stream)) return NULL;
			RP->pGlobals = pGlobals;
			break;
		}
	}

	// const Render::IShaderMetadata* pDefaultShaderMeta = RP->pGlobals; instead of a shader library, for shader ID 0
	if (!LoadEffectParams(Reader, NULL, RP->pGlobals, RP->Consts, RP->Resources, RP->Samplers)) return NULL;

	return RP.GetUnsafe();
}
//---------------------------------------------------------------------

}