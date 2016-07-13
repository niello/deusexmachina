#include "USMShaderMetadata.h"

#include <IO/BinaryReader.h>

namespace Render
{

bool CUSMShaderMetadata::Load(IO::CStream& Stream)
{
	IO::CBinaryReader R(Stream);

	//???where to validate? will be loaded at all? mb load and check these fields before creating D3D API shader object?
	U32 MinFeatureLevelValue;
	R.Read<U32>(MinFeatureLevelValue);
	MinFeatureLevel = (Render::EGPUFeatureLevel)MinFeatureLevelValue;

	R.Read<U64>(RequiresFlags);

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

	Consts.SetSize(R.Read<U32>());
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		CUSMConstMeta* pMeta = &Consts[i];
		if (!R.Read(pMeta->Name)) FAIL;

		U32 BufIdx;
		if (!R.Read(BufIdx)) FAIL;
		pMeta->BufferHandle = Buffers[BufIdx].Handle;

		U8 Type;
		if (!R.Read<U8>(Type)) FAIL;
		pMeta->Type = (EUSMConstType)Type;

		if (!R.Read<U32>(pMeta->Offset)) FAIL;
		if (!R.Read<U32>(pMeta->ElementSize)) FAIL;
		if (!R.Read<U32>(pMeta->ElementCount)) FAIL;

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

void CUSMShaderMetadata::Clear()
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

HConst CUSMShaderMetadata::GetConstHandle(CStrID ID) const
{
	//???!!!implement binary search for fixed arrays?!
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		CUSMConstMeta* pMeta = &Consts[i];
		if (pMeta->Name == ID)
		{
			if (!pMeta->Handle) pMeta->Handle = HandleMgr.OpenHandle(pMeta);
			return pMeta->Handle;
		}
	}
	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

HConstBuffer CUSMShaderMetadata::GetConstBufferHandle(CStrID ID) const
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

HConstBuffer CUSMShaderMetadata::GetConstBufferHandle(HConst hConst) const
{
	if (!hConst) return INVALID_HANDLE;
	CUSMConstMeta* pMeta = (CUSMConstMeta*)HandleMgr.GetHandleData(hConst);
	return pMeta ? pMeta->BufferHandle : INVALID_HANDLE;
}
//---------------------------------------------------------------------

HResource CUSMShaderMetadata::GetResourceHandle(CStrID ID) const
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

HSampler CUSMShaderMetadata::GetSamplerHandle(CStrID ID) const
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

EConstType CUSMShaderMetadata::GetConstType(HConst hConst) const
{
	if (!hConst) return ConstType_Invalid;
	CUSMConstMeta* pMeta = (CUSMConstMeta*)HandleMgr.GetHandleData(hConst);
	switch (pMeta->Type)
	{
		case USMConst_Bool:		return ConstType_Bool;
		case USMConst_Int:		return ConstType_Int;
		case USMConst_Float:	return ConstType_Float;
		case USMConst_Struct:	return ConstType_Other;
		default:				return ConstType_Invalid;
	}
}
//---------------------------------------------------------------------

U32 CUSMShaderMetadata::GetConstElementCount(HConst hConst) const
{
	if (!hConst) return ConstType_Invalid;
	CUSMConstMeta* pMeta = (CUSMConstMeta*)HandleMgr.GetHandleData(hConst);
	return pMeta->ElementCount;
}
//---------------------------------------------------------------------

}
