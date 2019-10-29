#include "D3D9ConstantBuffer.h"
#include <Render/D3D9/SM30ShaderMetadata.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClassNoFactory(Render::CD3D9ConstantBuffer, Render::CConstantBuffer);

CD3D9ConstantBuffer::CD3D9ConstantBuffer(PSM30ConstantBufferParam Meta, const CD3D9ConstantBuffer* pInitData, bool Temporary)
{
	if (!Meta) return;

	_Meta = Meta;

	Float4Count = 0;
	const auto& Float4 = Meta->Float4;
	for (const auto& Range : Meta->Float4)
		Float4Count += Range.second;

	Int4Count = 0;
	for (const auto& Range : Meta->Int4)
		Int4Count += Range.second;

	BoolCount = 0;
	for (const auto& Range : Meta->Bool)
		BoolCount += Range.second;

	UPTR Float4Size = Float4Count * sizeof(float) * 4;
	UPTR Int4Size = Int4Count * sizeof(int) * 4;
	UPTR BoolSize = BoolCount * sizeof(BOOL);
	UPTR TotalSize = Float4Size + Int4Size + BoolSize;
	if (!TotalSize) return;

	if (pInitData && (Float4Count != pInitData->Float4Count || Int4Count != pInitData->Int4Count || BoolCount != pInitData->BoolCount)) return;

	char* pData = (char*)n_malloc_aligned(TotalSize, 16);
	if (!pData)
	{
		Float4Count = 0;
		Int4Count = 0;
		BoolCount = 0;
		return;
	}

	if (pInitData)
	{
		const void* pInitDataPtr = pInitData->pFloat4Data ? (const void*)pInitData->pFloat4Data :
			(pInitData->pInt4Data ? (const void*)pInitData->pInt4Data :
			(const void*)pInitData->pBoolData);
		memcpy(pData, pInitDataPtr, TotalSize);
	}
	else
	{
		// Documented SM 3.0 defaults are 0, 0.f and FALSE
		memset(pData, 0, TotalSize);
	}

	if (Float4Size)
	{
		pFloat4Data = (float*)pData;
		pData += Float4Size;
	}

	if (Int4Size)
	{
		pInt4Data = (int*)pData;
		pData += Int4Size;
	}

	if (BoolSize)
	{
		pBoolData = (BOOL*)pData;
		pData += BoolSize;
	}
}
//---------------------------------------------------------------------

CD3D9ConstantBuffer::~CD3D9ConstantBuffer()
{
	if (pFloat4Data) n_free_aligned(pFloat4Data);
	else if (pInt4Data) n_free_aligned(pInt4Data);
	else if (pBoolData) n_free_aligned(pBoolData);
	pFloat4Data = nullptr;
	pInt4Data = nullptr;
	pBoolData = nullptr;
	Float4Count = 0;
	Int4Count = 0;
	BoolCount = 0;
	_Meta = nullptr;
}
//---------------------------------------------------------------------

U8 CD3D9ConstantBuffer::GetAccessFlags() const
{
	return Access_CPU_Read | Access_CPU_Write | Access_GPU_Read;
}
//---------------------------------------------------------------------

void CD3D9ConstantBuffer::WriteData(ESM30RegisterSet RegSet, UPTR Offset, const void* pData, UPTR Size)
{
	n_assert_dbg(pData && Size);

	char* pDest;
	switch (RegSet)
	{
		case Reg_Float4:
			pDest = (char*)pFloat4Data + Offset * sizeof(float) * 4;
			Flags.Set(CB9_DirtyFloat4);
			break;
		case Reg_Int4:
			pDest = (char*)pInt4Data + Offset * sizeof(int) * 4;
			Flags.Set(CB9_DirtyInt4);
			break;
		case Reg_Bool:
			pDest = (char*)pBoolData + Offset * sizeof(BOOL);
			Flags.Set(CB9_DirtyBool);
			break;
		default:
			pDest = nullptr;
			break;
	};

	n_assert_dbg(pDest);
	n_assert_dbg(pDest + Size <= (char*)pFloat4Data + Float4Count * sizeof(float) * 4 + Int4Count * sizeof(int) * 4 + BoolCount * sizeof(BOOL));

	memcpy(pDest, pData, Size);
}
//---------------------------------------------------------------------

}
