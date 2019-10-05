#include "Material.h"

#include <Render/RenderFwd.h>
#include <Render/GPUDriver.h>
#include <Render/Effect.h>
#include <Render/ShaderConstant.h>
#include <Render/ConstantBuffer.h>
#include <Render/Sampler.h>
#include <Render/Texture.h>
#include <IO/BinaryReader.h>

namespace Render
{
CMaterial::CMaterial() {}
CMaterial::~CMaterial() {}

bool CMaterial::Load(CGPUDriver& GPU, IO::CStream& Stream)
{
	IO::CBinaryReader Reader(Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'MTRL') FAIL;

	U32 Version;
	if (!Reader.Read<U32>(Version)) FAIL;

	CStrID EffectID;
	if (!Reader.Read(EffectID)) FAIL;
	if (!EffectID.IsValid()) FAIL;

	CString RUID("Effects:");
	RUID += EffectID.CStr();
	RUID += ".eff"; //???replace ID by full UID on export?

	Effect = GPU.GetEffect(CStrID(RUID));

	// Build parameters

	CDict<CStrID, void*>			ConstValues;
	CDict<CStrID, Render::PTexture>	ResourceValues;
	CDict<CStrID, Render::PSampler>	SamplerValues;
	void*							pConstValueBuffer;	// Must be n_free()'d if not nullptr

	if (!CEffect::LoadParamValues(Reader, GPU, ConstValues, ResourceValues, SamplerValues, pConstValueBuffer)) FAIL;

	const CFixedArray<Render::CEffectConstant>& Consts = Effect->GetMaterialConstants();
	ConstBuffers.SetSize(Effect->GetMaterialConstantBufferCount());
	UPTR CurrCBCount = 0;
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		const Render::CEffectConstant& Const = Consts[i];

		const Render::HConstantBuffer hCB = Const.Const->GetConstantBufferHandle();
		Render::CMaterial::CConstBufferRecord* pRec = nullptr;
		for (UPTR BufIdx = 0; BufIdx < CurrCBCount; ++BufIdx)
			if (ConstBuffers[BufIdx].Handle == hCB)
			{
				pRec = &ConstBuffers[BufIdx];
				break;
			}

		if (!pRec)
		{
			pRec = &ConstBuffers[CurrCBCount];
			pRec->Handle = hCB;
			pRec->ShaderType = Const.ShaderType;
			pRec->Buffer = GPU.CreateConstantBuffer(hCB, Render::Access_CPU_Write); //!!!must be a RAM-only buffer!
			++CurrCBCount;

			if (!GPU.BeginShaderConstants(*pRec->Buffer.Get())) FAIL;
		}

		IPTR Idx = ConstValues.FindIndex(Const.ID);
		void* pValue = (Idx != INVALID_INDEX) ? ConstValues.ValueAt(Idx) : nullptr;
		if (!pValue) pValue = Effect->GetConstantDefaultValue(Const.ID);

		if (pValue) //???fail if value is undefined? or fill with zeroes?
		{
			if (Const.Const.IsNullPtr()) FAIL;
			Const.Const->SetRawValue(*pRec->Buffer.Get(), pValue, Render::CShaderConstant::WholeSize);
		}
	}

	SAFE_FREE(pConstValueBuffer);

	for (UPTR BufIdx = 0; BufIdx < CurrCBCount; ++BufIdx)
	{
		Render::CMaterial::CConstBufferRecord* pRec = &ConstBuffers[BufIdx];
		Render::PConstantBuffer RAMBuffer = pRec->Buffer;
		if (!GPU.CommitShaderConstants(*RAMBuffer.Get())) FAIL; //!!!must not do any VRAM operations inside!

		//???do only if current buffer doesn't support VRAM? DX9 will support, DX11 will not.
		//if supports VRAM, can reuse as VRAM buffer without data copying between RAMBuffer and a new one.
		pRec->Buffer = GPU.CreateConstantBuffer(pRec->Handle, Render::Access_GPU_Read, RAMBuffer.Get());
	}

	const CFixedArray<Render::CEffectResource>& EffectResources = Effect->GetMaterialResources();
	Resources.SetSize(EffectResources.GetCount());
	for (UPTR i = 0; i < EffectResources.GetCount(); ++i)
	{
		const Render::CEffectResource& Rsrc = EffectResources[i];

		IPTR Idx = ResourceValues.FindIndex(Rsrc.ID);
		Render::PTexture Value = (Idx != INVALID_INDEX) ? ResourceValues.ValueAt(Idx) : Render::PTexture();

		Render::CMaterial::CResourceRecord& Rec = Resources[i];
		Rec.Handle = Rsrc.Handle;
		Rec.ShaderType = Rsrc.ShaderType;
		Rec.Resource = Value.IsValidPtr() ? Value : Effect->GetResourceDefaultValue(Rsrc.ID);
	}

	const CFixedArray<Render::CEffectSampler>& EffectSamplers = Effect->GetMaterialSamplers();
	Samplers.SetSize(EffectSamplers.GetCount());
	for (UPTR i = 0; i < EffectSamplers.GetCount(); ++i)
	{
		const Render::CEffectSampler& Sampler = EffectSamplers[i];

		IPTR Idx = SamplerValues.FindIndex(Sampler.ID);
		Render::PSampler Value = (Idx != INVALID_INDEX) ? SamplerValues.ValueAt(Idx) : Render::PSampler();

		Render::CMaterial::CSamplerRecord& Rec = Samplers[i];
		Rec.Handle = Sampler.Handle;
		Rec.ShaderType = Sampler.ShaderType;
		Rec.Sampler = Value.IsValidPtr() ? Value : Effect->GetSamplerDefaultValue(Sampler.ID);
	}

	OK;
}
//---------------------------------------------------------------------

// NB: GPU must be the same as used for loading
bool CMaterial::Apply(CGPUDriver& GPU) const
{
	for (UPTR i = 0 ; i < ConstBuffers.GetCount(); ++i)
	{
		const CConstBufferRecord& Rec = ConstBuffers[i];
		if (!GPU.BindConstantBuffer(Rec.ShaderType, Rec.Handle, Rec.Buffer.Get())) FAIL;
	}

	for (UPTR i = 0 ; i < Resources.GetCount(); ++i)
	{
		const CResourceRecord& Rec = Resources[i];
		if (!GPU.BindResource(Rec.ShaderType, Rec.Handle, Rec.Resource.Get())) FAIL;
	}

	for (UPTR i = 0 ; i < Samplers.GetCount(); ++i)
	{
		const CSamplerRecord& Rec = Samplers[i];
		if (!GPU.BindSampler(Rec.ShaderType, Rec.Handle, Rec.Sampler.Get())) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

}