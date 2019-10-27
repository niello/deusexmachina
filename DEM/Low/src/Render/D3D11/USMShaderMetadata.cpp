#include "USMShaderMetadata.h"

#include <Render/D3D11/USMShaderConstant.h>
#include <IO/BinaryReader.h>

namespace Render
{
CUSMMetadata::CUSMMetadata() {}

CUSMMetadata::~CUSMMetadata()
{
	Clear();
}
//---------------------------------------------------------------------

bool CUSMMetadata::Load(IO::CStream& Stream)
{
	IO::CBinaryReader R(Stream);

	Buffers.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		CUSMBufferMeta* pMeta = &Buffers[i];
		if (!R.Read(pMeta->Name)) FAIL;
		if (!R.Read<U32>(pMeta->Register)) FAIL;

		U32 BufType = pMeta->Register >> 30;
		switch (BufType)
		{
			case 0:		pMeta->Type = USMBuffer_Constant; break;
			case 1:		pMeta->Type = USMBuffer_Texture; break;
			case 2:		pMeta->Type = USMBuffer_Structured; break;
			default:	FAIL;
		};

		pMeta->Register &= 0x3fffffff; // Clear bits 30 and 31

		if (!R.Read<U32>(pMeta->Size)) FAIL;

		// For non-empty buffers open handles at the load time to reference buffers from constants
		pMeta->Handle = pMeta->Size ? HandleMgr.OpenHandle(pMeta) : INVALID_HANDLE;
	}

	Structs.SetSize(R.Read<U32>());

	// Open handles at the load time to reference structs from constants and members
	for (UPTR i = 0; i < Structs.GetCount(); ++i)
	{
		CUSMStructMeta* pMeta = &Structs[i];
		pMeta->Handle = HandleMgr.OpenHandle(pMeta);
	}

	// Load members
	for (UPTR i = 0; i < Structs.GetCount(); ++i)
	{
		CUSMStructMeta* pMeta = &Structs[i];
		pMeta->Members.SetSize(R.Read<U32>());

		for (UPTR j = 0; j < pMeta->Members.GetCount(); ++j)
		{
			CUSMStructMemberMeta* pMemberMeta = &pMeta->Members[j];

			if (!R.Read(pMemberMeta->Name)) FAIL;

			U32 StructIndex;
			if (!R.Read<U32>(StructIndex)) FAIL;
			pMemberMeta->StructHandle = (StructIndex == (U32)(-1)) ? INVALID_HANDLE : Structs[StructIndex].Handle;

			U8 Type;
			if (!R.Read<U8>(Type)) FAIL;
			pMemberMeta->Type = (EUSMConstType)Type;

			if (!R.Read(pMemberMeta->Offset)) FAIL;
			if (!R.Read(pMemberMeta->ElementSize)) FAIL;
			if (!R.Read(pMemberMeta->ElementCount)) FAIL;
			if (!R.Read(pMemberMeta->Columns)) FAIL;
			if (!R.Read(pMemberMeta->Rows)) FAIL;
			if (!R.Read(pMemberMeta->Flags)) FAIL;
		}
	}

	Consts.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		CUSMConstantMeta* pMeta = &Consts[i];
		if (!R.Read(pMeta->Name)) FAIL;

		U32 BufIdx;
		if (!R.Read(BufIdx)) FAIL;
		pMeta->BufferHandle = Buffers[BufIdx].Handle;

		U32 StructIndex;
		if (!R.Read<U32>(StructIndex)) FAIL;
		pMeta->StructHandle = (StructIndex == (U32)(-1)) ? INVALID_HANDLE : Structs[StructIndex].Handle;

		U8 Type;
		if (!R.Read<U8>(Type)) FAIL;
		pMeta->Type = (EUSMConstType)Type;

		if (!R.Read(pMeta->Offset)) FAIL;
		if (!R.Read(pMeta->ElementSize)) FAIL;
		if (!R.Read(pMeta->ElementCount)) FAIL;
		if (!R.Read(pMeta->Columns)) FAIL;
		if (!R.Read(pMeta->Rows)) FAIL;
		if (!R.Read(pMeta->Flags)) FAIL;

		pMeta->Handle = INVALID_HANDLE;
	}

	Resources.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		CUSMResourceMeta* pMeta = &Resources[i];
		if (!R.Read(pMeta->Name)) FAIL;

		U8 Type;
		if (!R.Read<U8>(Type)) FAIL;
		pMeta->Type = (EUSMResourceType)Type;

		if (!R.Read<U32>(pMeta->RegisterStart)) FAIL;
		if (!R.Read<U32>(pMeta->RegisterCount)) FAIL;

		pMeta->Handle = INVALID_HANDLE;
	}

	Samplers.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		CUSMSamplerMeta* pMeta = &Samplers[i];
		if (!R.Read(pMeta->Name)) FAIL;
		if (!R.Read<U32>(pMeta->RegisterStart)) FAIL;
		if (!R.Read<U32>(pMeta->RegisterCount)) FAIL;

		pMeta->Handle = INVALID_HANDLE;
	}

	OK;
}
//---------------------------------------------------------------------

void CUSMMetadata::Clear()
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

	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		HHandle Handle = Resources[i].Handle;
		if (Handle) HandleMgr.CloseHandle(Handle);
	}
	Resources.Clear();

	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		HHandle Handle = Samplers[i].Handle;
		if (Handle) HandleMgr.CloseHandle(Handle);
	}
	Samplers.Clear();
}
//---------------------------------------------------------------------

HConstant CUSMMetadata::GetConstantHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		CUSMConstantMeta* pMeta = &Consts[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HConstantBuffer CUSMMetadata::GetConstantBufferHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Buffers.GetCount(); ++i)
	{
		CUSMBufferMeta* pMeta = &Buffers[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HResource CUSMMetadata::GetResourceHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Resources.GetCount(); ++i)
	{
		CUSMResourceMeta* pMeta = &Resources[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HSampler CUSMMetadata::GetSamplerHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		CUSMSamplerMeta* pMeta = &Samplers[i];
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
bool CUSMMetadata::GetConstDesc(CStrID ID, CShaderConstDesc& Out) const
{
	HConstant Handle = GetConstantHandle(ID);
	if (Handle == INVALID_HANDLE) FAIL;
	CUSMConstantMeta* pMeta = (CUSMConstantMeta*)HandleMgr.GetHandleData(Handle);
	Out.Handle = Handle;
	Out.BufferHandle = pMeta->BufferHandle;
	Out.ElementCount = pMeta->ElementCount;
	Out.Flags = 0;

	//!!!IMPLEMENT!
	Out.Rows = 0;
	Out.Columns = 0;
	if (pMeta->Flags & ShaderConst_ColumnMajor)
	{
		Out.Flags |= ShaderConst_ColumnMajor;
	}
	else
	{
	}

	//switch (pMeta->Type)
	//{
	//	case USMConst_Bool:		return ConstType_Bool;
	//	case USMConst_Int:		return ConstType_Int;
	//	case USMConst_Float:	return ConstType_Float;
	//	case USMConst_Struct:	return ConstType_Other;
	//	default:				return ConstType_Invalid;
	//}
	OK;
}
//---------------------------------------------------------------------
*/

PShaderConstant CUSMMetadata::GetConstant(HConstant hConst) const
{
	CUSMConstantMeta* pMeta = (CUSMConstantMeta*)HandleMgr.GetHandleData(hConst);
	if (!pMeta) return nullptr;

	if (pMeta->ConstObject.IsNullPtr())
	{
		PUSMConstant Const = n_new(CUSMConstant);
		if (!Const->Init(hConst)) return nullptr;
		pMeta->ConstObject = Const.Get();
	}

	return pMeta->ConstObject;
}
//---------------------------------------------------------------------

}
