#include "SM30ShaderMetadata.h"

#include <Render/D3D9/SM30ShaderConstant.h>
#include <IO/BinaryReader.h>

namespace Render
{

bool CSM30ShaderMetadata::Load(IO::CStream& Stream)
{
	IO::CBinaryReader R(Stream);

	Buffers.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		CSM30BufferMeta* pMeta = &Buffers[i];
		if (!R.Read(pMeta->Name)) FAIL;
		if (!R.Read(pMeta->SlotIndex)) FAIL;

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

		// For non-empty buffers open handles at the load time to reference buffers from constants
		pMeta->Handle =
			(pMeta->Float4.GetCount() || pMeta->Int4.GetCount() || pMeta->Bool.GetCount()) ?
			HandleMgr.OpenHandle(pMeta) :
			INVALID_HANDLE;
	}

	Structs.SetSize(R.Read<U32>());

	// Open handles at the load time to reference structs from constants and members
	for (UPTR i = 0; i < Structs.GetCount(); ++i)
	{
		CSM30StructMeta* pMeta = &Structs[i];
		pMeta->Handle = HandleMgr.OpenHandle(pMeta);
	}

	// Load members
	for (UPTR i = 0; i < Structs.GetCount(); ++i)
	{
		CSM30StructMeta* pMeta = &Structs[i];
		pMeta->Members.SetSize(R.Read<U32>());

		for (UPTR j = 0; j < pMeta->Members.GetCount(); ++j)
		{
			CSM30StructMemberMeta* pMemberMeta = &pMeta->Members[j];

			if (!R.Read(pMemberMeta->Name)) FAIL;

			U32 StructIndex;
			if (!R.Read<U32>(StructIndex)) FAIL;
			pMemberMeta->StructHandle = (StructIndex == (U32)(-1)) ? INVALID_HANDLE : Structs[StructIndex].Handle;

			if (!R.Read(pMemberMeta->RegisterOffset)) FAIL;
			if (!R.Read(pMemberMeta->ElementRegisterCount)) FAIL;
			if (!R.Read(pMemberMeta->ElementCount)) FAIL;
			if (!R.Read(pMemberMeta->Columns)) FAIL;
			if (!R.Read(pMemberMeta->Rows)) FAIL;
			if (!R.Read(pMemberMeta->Flags)) FAIL;
		}
	}

	Consts.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		CSM30ConstMeta* pMeta = &Consts[i];
		if (!R.Read(pMeta->Name)) FAIL;

		U32 BufIdx;
		if (!R.Read<U32>(BufIdx)) FAIL;
		pMeta->BufferHandle = Buffers[BufIdx].Handle;

		U32 StructIndex;
		if (!R.Read<U32>(StructIndex)) FAIL;
		pMeta->StructHandle = (StructIndex == (U32)(-1)) ? INVALID_HANDLE : Structs[StructIndex].Handle;

		U8 RegSet;
		if (!R.Read<U8>(RegSet)) FAIL;
		pMeta->RegSet = (ESM30RegisterSet)RegSet;
		n_assert_dbg(RegSet == Reg_Bool || RegSet == Reg_Int4 || RegSet == Reg_Float4);

		if (!R.Read(pMeta->RegisterStart)) FAIL;
		if (!R.Read(pMeta->ElementRegisterCount)) FAIL;
		if (!R.Read(pMeta->ElementCount)) FAIL;
		if (!R.Read(pMeta->Columns)) FAIL;
		if (!R.Read(pMeta->Rows)) FAIL;
		if (!R.Read(pMeta->Flags)) FAIL;

		pMeta->Handle = INVALID_HANDLE;
	}

	Resources.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		CSM30RsrcMeta* pMeta = &Resources[i];
		if (!R.Read(pMeta->Name)) FAIL;
		if (!R.Read<U32>(pMeta->Register)) FAIL;
		
		pMeta->Handle = INVALID_HANDLE;
	}

	Samplers.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		CSM30SamplerMeta* pMeta = &Samplers[i];
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

	for (UPTR i = 0; i < Structs.GetCount(); ++i)
	{
		HHandle Handle = Structs[i].Handle;
		if (Handle) HandleMgr.CloseHandle(Handle);
	}
	Structs.Clear();

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
		CSM30ConstMeta* pMeta = &Consts[i];
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
		CSM30BufferMeta* pMeta = &Buffers[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}

	if (Buffers.GetCount())
	{
		// Default buffer
		CSM30BufferMeta* pMeta = &Buffers[0];
		if (!pMeta->Handle) pMeta->Handle = HandleMgr.OpenHandle(pMeta);
		return pMeta->Handle;
	}

	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HResource CSM30ShaderMetadata::GetResourceHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		CSM30RsrcMeta* pMeta = &Resources[i];
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
		CSM30SamplerMeta* pMeta = &Samplers[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

/*
bool CSM30ShaderMetadata::GetConstDesc(CStrID ID, CShaderConstDesc& Out) const
{
	HConst Handle = GetConstHandle(ID);
	if (Handle == INVALID_HANDLE) FAIL;
	CSM30ConstMeta* pMeta = (CSM30ConstMeta*)HandleMgr.GetHandleData(Handle);
	Out.Handle = Handle;
	Out.BufferHandle = pMeta->BufferHandle;
	Out.ElementCount = pMeta->ElementCount;
	Out.Flags = 0;

	U32 RegisterElements;
	switch (pMeta->RegSet)
	{
		case Reg_Bool:		//ConstType_Bool;
		{
			RegisterElements = 1;
			break;
		}
		case Reg_Int4:		//ConstType_Int;
		case Reg_Float4:	//ConstType_Float;
		{
			RegisterElements = 4;
			break;
		}
	}

	if (pMeta->Flags & ShaderConst_ColumnMajor)
	{
		Out.Flags |= ShaderConst_ColumnMajor;
		Out.Rows = RegisterElements;
		Out.Columns = pMeta->ElementRegisterCount;
	}
	else
	{
		Out.Rows = pMeta->ElementRegisterCount;
		Out.Columns = RegisterElements;
	}

	OK;
}
//---------------------------------------------------------------------
*/

PShaderConstant CSM30ShaderMetadata::GetConstant(HConst hConst) const
{
	CSM30ConstMeta* pMeta = (CSM30ConstMeta*)HandleMgr.GetHandleData(hConst);
	if (!pMeta) return NULL;

	if (pMeta->ConstObject.IsNullPtr())
	{
		PSM30ShaderConstant Const = n_new(CSM30ShaderConstant);
		if (!Const->Init(hConst)) return NULL;
		pMeta->ConstObject = Const.GetUnsafe();
	}

	return pMeta->ConstObject;
}
//---------------------------------------------------------------------

}
