#include "EffectConstSetValues.h"

#include <Render/GPUDriver.h>
//#include <Render/ConstantBuffer.h> // For IsInEditMode() only

namespace Render
{

bool CEffectConstSetValues::SetGPU(PGPUDriver NewGPU)
{
	if (NewGPU == GPU) OK;

	//!!!unbind and kill all buffers stored!
	//!!!recreate all non-tmp buffers with the new GPU!

	GPU = NewGPU;
	OK;
}
//---------------------------------------------------------------------

//???!!!return buffer and use as arg in latter calls to SetConstantValue()!?
bool CEffectConstSetValues::RegisterConstantBuffer(HConstBuffer Handle, CConstantBuffer* pBuffer)
{
	if (Handle == INVALID_HANDLE) FAIL;

	// Already registered buffer won't be registered again
	IPTR BufferIdx = Buffers.FindIndex(Handle);
	if (BufferIdx != INVALID_INDEX) OK;

	CConstBufferRecord& Rec = Buffers.Add(Handle);
	Rec.Buffer = pBuffer;
	Rec.Flags = 0;

	OK;
}
//---------------------------------------------------------------------

bool CEffectConstSetValues::SetConstantValue(const CEffectConstant* pConst, UPTR ElementIndex, const void* pValue, UPTR Size)
{
	const HConstBuffer hCB = pConst->Desc.BufferHandle;
	IPTR BufferIdx = Buffers.FindIndex(hCB);
	if (BufferIdx == INVALID_INDEX) FAIL;

	CConstBufferRecord& CBRec = Buffers.ValueAt(BufferIdx);

	//if (!Buffers[BufferIdx].Buffer->IsInEditMode())
	if (!CBRec.Flags)
	{
		if (CBRec.Buffer.IsNullPtr())
		{
			CBRec.Buffer = GPU->CreateTemporaryConstantBuffer(hCB);
			if (CBRec.Buffer.IsNullPtr()) FAIL;
			CBRec.Flags |= ECSV_TmpBuffer;
		}
		if (!GPU->BeginShaderConstants(*CBRec.Buffer)) FAIL;
	}
	CBRec.Flags |= (1 << pConst->ShaderType);

	return GPU->SetShaderConstant(*CBRec.Buffer, pConst->Desc.Handle, ElementIndex, pValue, Size);
}
//---------------------------------------------------------------------

bool CEffectConstSetValues::ApplyConstantBuffers()
{
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		CConstBufferRecord& CBRec = Buffers.ValueAt(i);
		if (!CBRec.Flags) continue;
		
		CConstantBuffer& Buffer = *CBRec.Buffer;

		if (CBRec.Flags) GPU->CommitShaderConstants(Buffer);
		
		HConstBuffer hCB = Buffers.KeyAt(i);
		for (UPTR j = 0; j < Render::ShaderType_COUNT; ++j)
			if (CBRec.Flags & (1 << j))
				GPU->BindConstantBuffer((Render::EShaderType)j, hCB, &Buffer);

		if (CBRec.Flags & ECSV_TmpBuffer)
		{
			//???should tmp buffer exist until rendered? killing reference may
			//result in a destruction of tmp CB when another one is bound to GPU!
			//GPU must store ref inside until it is not required any more!
			GPU->FreeTemporaryConstantBuffer(Buffer);
			CBRec.Buffer = NULL;
		}

		CBRec.Flags = 0;
	}

	OK;
}
//---------------------------------------------------------------------

void CEffectConstSetValues::UnbindAndClear()
{
	//NOT_IMPLEMENTED; //!!!unbind!
	Buffers.Clear(true);
	GPU = NULL;
}
//---------------------------------------------------------------------

}