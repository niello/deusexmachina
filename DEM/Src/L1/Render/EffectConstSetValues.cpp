#include "EffectConstSetValues.h"

#include <Render/GPUDriver.h>
//#include <Render/ConstantBuffer.h> // For IsInEditMode() only

namespace Render
{

bool CEffectConstSetValues::SetGPU(PGPUDriver NewGPU)
{
	if (NewGPU == GPU) OK;

	//!!!unbind and kill all buffers stored!
	//!!!recreate all non-tmp buffers!

	GPU = NewGPU;
	OK;
}
//---------------------------------------------------------------------

bool CEffectConstSetValues::RegisterConstantBuffer(HConstBuffer Handle, CConstantBuffer* pBuffer)
{
	if (Handle == INVALID_HANDLE) FAIL;

	// Already registered buffer won't be registered again
	IPTR BufferIdx = Buffers.FindIndex(Handle);
	if (BufferIdx != INVALID_INDEX) OK;

	Render::CConstBufferRecord& Rec = Buffers.Add(Handle);
	Rec.Handle = Handle;
	Rec.Buffer = pBuffer;
	Rec.ShaderTypes = 0; // Filled when const values are written, not to bind unnecessarily to all possibly requiring stages

	OK;
}
//---------------------------------------------------------------------

bool CEffectConstSetValues::SetConstantValue(const CEffectConstant* pConst, UPTR ElementIndex, const void* pValue, UPTR Size)
{
	IPTR BufferIdx = Buffers.FindIndex(pConst->BufferHandle);
	if (BufferIdx == INVALID_INDEX) FAIL;

	CConstBufferRecord& CBRec = Buffers.ValueAt(BufferIdx);

	//if (!Buffers[BufferIdx].Buffer->IsInEditMode())
	if (!CBRec.ShaderTypes)
	{
		if (!GPU->BeginShaderConstants(*CBRec.Buffer)) FAIL;
	}
	CBRec.ShaderTypes |= (1 << pConst->ShaderType);

	return GPU->SetShaderConstant(*CBRec.Buffer, pConst->Handle, ElementIndex, pValue, Size);
}
//---------------------------------------------------------------------

bool CEffectConstSetValues::ApplyConstantBuffers()
{
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		CConstBufferRecord& CBRec = Buffers.ValueAt(i);
		if (!CBRec.ShaderTypes) continue;
		
		CConstantBuffer& Buffer = *CBRec.Buffer;

		//Buffer.IsInEditMode()
		if (CBRec.ShaderTypes) GPU->CommitShaderConstants(Buffer);
		
		for (UPTR j = 0; j < Render::ShaderType_COUNT; ++j)
			if (CBRec.ShaderTypes & (1 << j))
				GPU->BindConstantBuffer((Render::EShaderType)j, CBRec.Handle, &Buffer);

		//???!!!where to reset shader types?! here?
		CBRec.ShaderTypes = 0;
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