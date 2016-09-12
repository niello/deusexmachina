#include "SM30ShaderConstant.h"

#include <Render/D3D9/SM30ShaderMetadata.h>
#include <Render/D3D9/D3D9ConstantBuffer.h>

namespace Render
{

bool CSM30ShaderConstant::Init(HConst hConst)
{
	if (!hConst) FAIL;
	CSM30ConstMeta* pMeta = (CSM30ConstMeta*)IShaderMetadata::GetHandleData(hConst);
	if (!pMeta) FAIL;

	CSM30BufferMeta* pBufferMeta = (CSM30BufferMeta*)IShaderMetadata::GetHandleData(pMeta->BufferHandle);
	if (!pBufferMeta) FAIL;

	CFixedArray<CRange>* pRanges = NULL;
	switch (pMeta->RegSet)
	{
		case Reg_Float4:	pRanges = &pBufferMeta->Float4; break;
		case Reg_Int4:		pRanges = &pBufferMeta->Int4; break;
		case Reg_Bool:		pRanges = &pBufferMeta->Bool; break;
		default:			FAIL;
	};

	Offset = 0;
	for (UPTR i = 0; i < pRanges->GetCount(); ++i)
	{
		CRange& Range = pRanges->operator[](i);
		if (Range.Start > pMeta->RegisterStart) FAIL; // As ranges are sorted ascending
		if (Range.Start + Range.Count <= pMeta->RegisterStart)
		{
			Offset += Range.Count;
			continue;
		}
		n_assert_dbg(Range.Start + Range.Count >= pMeta->RegisterStart + pMeta->ElementRegisterCount * pMeta->ElementCount);

		// Range found, initialize constant
		//Handle = hConst;
		Offset += pMeta->RegisterStart - Range.Start;
		RegSet = pMeta->RegSet;
		ElementCount = pMeta->ElementCount;
		ElementRegisterCount = pMeta->ElementRegisterCount;
		StructHandle = pMeta->StructHandle;
		//???calc size for verification of Set*** calls?

		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

UPTR CSM30ShaderConstant::GetMemberCount() const
{
	if (StructHandle == INVALID_HANDLE) return 0;
	CSM30StructMeta* pStructMeta = (CSM30StructMeta*)IShaderMetadata::GetHandleData(StructHandle);
	return pStructMeta ? pStructMeta->Members.GetCount() : 0;
}
//---------------------------------------------------------------------

PShaderConstant CSM30ShaderConstant::GetElement(U32 Index) const
{
	if (Index >= ElementCount) return NULL;

	//!!!can use pool!
	PSM30ShaderConstant Const = n_new(CSM30ShaderConstant);
	Const->Offset = Offset + Index * ElementRegisterCount;
	Const->RegSet = RegSet;
	Const->StructHandle = StructHandle;
	Const->ElementCount = 1;
	Const->ElementRegisterCount = ElementRegisterCount;

	return Const.GetUnsafe();
}
//---------------------------------------------------------------------

PShaderConstant CSM30ShaderConstant::GetMember(CStrID Name) const
{
	CSM30StructMeta* pStructMeta = (CSM30StructMeta*)IShaderMetadata::GetHandleData(StructHandle);
	if (!pStructMeta) return NULL;

	for (CFixedArray<CSM30StructMemberMeta>::CIterator It = pStructMeta->Members.Begin(); It < pStructMeta->Members.End(); ++It)
		if (It->Name == Name)
		{
			//!!!can use pool!
			PSM30ShaderConstant Const = n_new(CSM30ShaderConstant);
			Const->Offset = Offset + It->RegisterOffset;
			Const->RegSet = RegSet;
			Const->StructHandle = It->StructHandle;
			Const->ElementCount = It->ElementCount;
			Const->ElementRegisterCount = It->ElementRegisterCount;

			return Const.GetUnsafe();
		}

	return NULL;
}
//---------------------------------------------------------------------

void CSM30ShaderConstant::SetRawValue(const CConstantBuffer& CB, const void* pData, UPTR Size) const
{
	n_assert_dbg(RegSet != Reg_Invalid && CB.IsA<CD3D9ConstantBuffer>());
	if (Size == WholeSize)
	{
		//!!!calc whole size in bytes and store!
		NOT_IMPLEMENTED;
		//Size = ElementCount * ElementRegisterCount * sizeof(register);
	}
	CD3D9ConstantBuffer& CB9 = (CD3D9ConstantBuffer&)CB;
	CB9.WriteData(RegSet, Offset, pData, Size);
}
//---------------------------------------------------------------------

}
