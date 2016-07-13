#include "SM30ShaderMetadata.h"

#include <IO/BinaryReader.h>

namespace Render
{

bool CSM30ShaderMetadata::Load(IO::CStream& Stream)
{
	IO::CBinaryReader R(Stream);

	Buffers.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		CSM30ShaderBufferMeta* pMeta = &Buffers[i];
		if (!R.Read(pMeta->Name)) FAIL;

		CFixedArray<CRange>& Ranges1 = pMeta->Float4;
		Ranges1.SetSize(R.Read<U32>());
		for (UPTR r = 0; r < Ranges1.GetCount(); ++r)
		{
			CRange& Range = Ranges1[r];
			if (!R.Read<U32>(Range.Start)) FAIL;
			if (!R.Read<U32>(Range.Count)) FAIL;
		}

		CFixedArray<CRange>& Ranges2 = pMeta->Int4;
		Ranges2.SetSize(R.Read<U32>());
		for (UPTR r = 0; r < Ranges2.GetCount(); ++r)
		{
			CRange& Range = Ranges2[r];
			if (!R.Read<U32>(Range.Start)) FAIL;
			if (!R.Read<U32>(Range.Count)) FAIL;
		}

		CFixedArray<CRange>& Ranges3 = pMeta->Bool;
		Ranges3.SetSize(R.Read<U32>());
		for (UPTR r = 0; r < Ranges3.GetCount(); ++r)
		{
			CRange& Range = Ranges3[r];
			if (!R.Read<U32>(Range.Start)) FAIL;
			if (!R.Read<U32>(Range.Count)) FAIL;
		}

		pMeta->SlotIndex = i;

		// For non-empty buffers open handles at the load time to reference buffers from constants
		pMeta->Handle =
			(pMeta->Float4.GetCount() || pMeta->Int4.GetCount() || pMeta->Bool.GetCount()) ?
			HandleMgr.OpenHandle(pMeta) :
			INVALID_HANDLE;
	}

	Consts.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		CSM30ShaderConstMeta* pMeta = &Consts[i];
		if (!R.Read(pMeta->Name)) FAIL;

		U32 BufIdx;
		if (!R.Read<U32>(BufIdx)) FAIL;
		pMeta->BufferHandle = Buffers[BufIdx].Handle;

		U8 RegSet;
		if (!R.Read<U8>(RegSet)) FAIL;
		pMeta->RegSet = (ESM30RegisterSet)RegSet;

		if (!R.Read<U32>(pMeta->RegisterStart)) FAIL;
		if (!R.Read<U32>(pMeta->ElementRegisterCount)) FAIL;
		if (!R.Read<U32>(pMeta->ElementCount)) FAIL;

		pMeta->Handle = INVALID_HANDLE;
	}

	Resources.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		CSM30ShaderRsrcMeta* pMeta = &Resources[i];
		if (!R.Read(pMeta->Name)) FAIL;
		if (!R.Read<U32>(pMeta->Register)) FAIL;
		
		pMeta->Handle = INVALID_HANDLE;
	}

	Samplers.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		CSM30ShaderSamplerMeta* pMeta = &Samplers[i];
		if (!R.Read(pMeta->Name)) FAIL;

		U8 Type;
		if (!R.Read<U8>(Type)) FAIL;
		pMeta->Type = (ESM30SamplerType)Type;
		
		if (!R.Read<U32>(pMeta->RegisterStart)) FAIL;
		if (!R.Read<U32>(pMeta->RegisterCount)) FAIL;

		pMeta->Handle = INVALID_HANDLE;
	}

	OK;
}
//---------------------------------------------------------------------

void CSM30ShaderMetadata::Clear()
{
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		HHandle Handle = Consts[i].Handle;
		if (Handle) HandleMgr.CloseHandle(Handle);
	}
	Consts.Clear();

	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		HHandle Handle = Buffers[i].Handle;
		if (Handle) HandleMgr.CloseHandle(Handle);
	}
	Buffers.Clear();

	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		HHandle Handle = Samplers[i].Handle;
		if (Handle) HandleMgr.CloseHandle(Handle);
	}
	Samplers.Clear();
}
//---------------------------------------------------------------------

HConst CSM30ShaderMetadata::GetConstHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		CSM30ShaderConstMeta* pMeta = &Consts[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HConstBuffer CSM30ShaderMetadata::GetConstBufferHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		CSM30ShaderBufferMeta* pMeta = &Buffers[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}

	if (Buffers.GetCount())
	{
		// Default buffer
		CSM30ShaderBufferMeta* pMeta = &Buffers[0];
		if (!pMeta->Handle) pMeta->Handle = HandleMgr.OpenHandle(pMeta);
		return pMeta->Handle;
	}

	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HConstBuffer CSM30ShaderMetadata::GetConstBufferHandle(HConst hConst) const
{
	if (!hConst) return INVALID_HANDLE;
	CSM30ShaderConstMeta* pMeta = (CSM30ShaderConstMeta*)HandleMgr.GetHandleData(hConst);
	return pMeta ? pMeta->BufferHandle : INVALID_HANDLE;
}
//---------------------------------------------------------------------

HResource CSM30ShaderMetadata::GetResourceHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		CSM30ShaderRsrcMeta* pMeta = &Resources[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HSampler CSM30ShaderMetadata::GetSamplerHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		CSM30ShaderSamplerMeta* pMeta = &Samplers[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

EConstType CSM30ShaderMetadata::GetConstType(HConst hConst) const
{
	if (!hConst) return ConstType_Invalid;
	CSM30ShaderConstMeta* pMeta = (CSM30ShaderConstMeta*)HandleMgr.GetHandleData(hConst);
	switch (pMeta->RegSet)
	{
		case Reg_Bool:		return ConstType_Bool;
		case Reg_Int4:		return ConstType_Int;
		case Reg_Float4:	return ConstType_Float;
		default:			return ConstType_Invalid;
	}
}
//---------------------------------------------------------------------

U32 CSM30ShaderMetadata::GetConstElementCount(HConst hConst) const
{
	if (!hConst) return ConstType_Invalid;
	CSM30ShaderConstMeta* pMeta = (CSM30ShaderConstMeta*)HandleMgr.GetHandleData(hConst);
	return pMeta->ElementCount;
}
//---------------------------------------------------------------------

}
