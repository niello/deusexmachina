#include "D3D9ConstantBuffer.h"

#include <Render/D3D9/SM30ShaderMetadata.h>
#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9ConstantBuffer, 'CB09', Render::CConstantBuffer);

//!!!???assert destroyed?!
bool CD3D9ConstantBuffer::Create(const CSM30ShaderBufferMeta& Meta)
{
	Float4Count = 0;
	const CFixedArray<CRange>& Float4 = Meta.Float4;
	for (UPTR i = 0; i < Float4.GetCount(); ++i)
		Float4Count += Float4[i].Count;

	Int4Count = 0;
	const CFixedArray<CRange>& Int4 = Meta.Int4;
	for (UPTR i = 0; i < Int4.GetCount(); ++i)
		Int4Count += Int4[i].Count;

	BoolCount = 0;
	const CFixedArray<CRange>& Bool = Meta.Bool;
	for (UPTR i = 0; i < Bool.GetCount(); ++i)
		BoolCount += Bool[i].Count;

	UPTR Float4Size = Float4Count * sizeof(float) * 4;
	UPTR Int4Size = Int4Count * sizeof(int) * 4;
	UPTR BoolSize = BoolCount * sizeof(BOOL);
	UPTR TotalSize = Float4Size + Int4Size + BoolSize;
	if (!TotalSize) FAIL;

	char* pData = (char*)n_malloc_aligned(TotalSize, 16);
	if (!pData)
	{
		Float4Count = 0;
		Int4Count = 0;
		BoolCount = 0;
		FAIL;
	}

	// Documented SM 3.0 defaults are 0, 0.f and FALSE
	memset(pData, 0, TotalSize);

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

	Handle = Meta.Handle;

	OK;
}
//---------------------------------------------------------------------

void CD3D9ConstantBuffer::InternalDestroy()
{
	if (pFloat4Data) n_free_aligned(pFloat4Data);
	else if (pInt4Data) n_free_aligned(pInt4Data);
	else if (pBoolData) n_free_aligned(pBoolData);
	pFloat4Data = NULL;
	pInt4Data = NULL;
	pBoolData = NULL;
	Float4Count = 0;
	Int4Count = 0;
	BoolCount = 0;
	Handle = INVALID_HANDLE;
}
//---------------------------------------------------------------------

}
