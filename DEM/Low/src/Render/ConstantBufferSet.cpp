#include "ConstantBufferSet.h"

#include <Render/ShaderConstant.h>
#include <Render/ConstantBuffer.h>
#include <Render/GPUDriver.h>

namespace Render
{
CConstantBufferSet::CConstantBufferSet() {}
CConstantBufferSet::~CConstantBufferSet() {}

bool CConstantBufferSet::SetGPU(PGPUDriver NewGPU)
{
	if (NewGPU == GPU) OK;

	if (GPU.IsValidPtr())
	{
		//???!!!recreate all non-tmp buffers with the new GPU!?
		NOT_IMPLEMENTED;
		UnbindAndClear();
	}

	GPU = NewGPU;
	OK;
}
//---------------------------------------------------------------------

bool CConstantBufferSet::RegisterPermanentBuffer(HConstantBuffer Handle, CConstantBuffer& Buffer)
{
	if (Handle == INVALID_HANDLE) FAIL;

	IPTR BufferIdx = Buffers.FindIndex(Handle);
	CConstBufferRecord& Rec = (BufferIdx == INVALID_INDEX) ? Buffers.Add(Handle) : Buffers.ValueAt(BufferIdx);
	Rec.Buffer = &Buffer;
	Rec.Flags = 0;

	OK;
}
//---------------------------------------------------------------------

bool CConstantBufferSet::IsBufferRegistered(HConstantBuffer Handle) const
{
	return Buffers.Contains(Handle);
}
//---------------------------------------------------------------------

CConstantBuffer* CConstantBufferSet::RequestBuffer(HConstantBuffer Handle, EShaderType Stage)
{
	IPTR BufferIdx = Buffers.FindIndex(Handle);
	CConstBufferRecord* pRec;
	if ((BufferIdx == INVALID_INDEX))
	{
		pRec = &Buffers.Add(Handle);
		pRec->Flags = 0;
	}
	else pRec = &Buffers.ValueAt(BufferIdx);

	// If buffer is still not used
	if (!pRec->Flags)
	{
		if (pRec->Buffer.IsNullPtr())
		{
			// No permanent buffer registered, create temporary buffer and mark it for later deletion
			pRec->Buffer = GPU->CreateTemporaryConstantBuffer(Handle);
			if (pRec->Buffer.IsNullPtr()) return nullptr;
		}

		if (!GPU->BeginShaderConstants(*pRec->Buffer)) return nullptr;
	}

	// Mark buffer as used at the Stage
	pRec->Flags |= (1 << Stage);

	return pRec->Buffer.Get();
}
//---------------------------------------------------------------------

bool CConstantBufferSet::CommitChanges()
{
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		// If buffer contents didn't change, skip it
		// NB: due to Flags structure, it works only for one buffer - one stage
		// scenario, which may change later
		CConstBufferRecord& Rec = Buffers.ValueAt(i);
		if (!Rec.Flags) continue;
		
		CConstantBuffer& Buffer = *Rec.Buffer;

		if (!GPU->CommitShaderConstants(Buffer)) FAIL;

		// Bind buffer to all stages it is used at
		HConstantBuffer hCB = Buffers.KeyAt(i);
		for (UPTR j = 0; j < Render::ShaderType_COUNT; ++j)
			if (Rec.Flags & (1 << j))
				GPU->BindConstantBuffer((Render::EShaderType)j, hCB, &Buffer);

		if (Buffer.IsTemporary())
		{
			GPU->FreeTemporaryConstantBuffer(Buffer);
			Rec.Buffer = nullptr;
		}

		Rec.Flags = 0;
	}

	OK;
}
//---------------------------------------------------------------------

void CConstantBufferSet::UnbindAndClear()
{
	if (GPU.IsValidPtr())
	{
		//NOT_IMPLEMENTED; //!!!unbind!
		//can unbind only non-tmp buffers here, unbind tmp inside a GPU!
	}
	Buffers.Clear(true);
	GPU = nullptr;
}
//---------------------------------------------------------------------

}