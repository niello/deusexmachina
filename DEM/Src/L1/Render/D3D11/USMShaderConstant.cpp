#include "USMShaderConstant.h"

#include <Render/D3D11/USMShaderMetadata.h>
#include <Render/D3D11/D3D11ConstantBuffer.h>

namespace Render
{

bool CUSMShaderConstant::Init(HConst hConst)
{
	if (!hConst) FAIL;

	CUSMConstMeta* pMeta = (CUSMConstMeta*)IShaderMetadata::GetHandleData(hConst);
	if (!pMeta) FAIL;

	Offset = pMeta->Offset; 
	OK;
}
//---------------------------------------------------------------------

UPTR CUSMShaderConstant::GetMemberCount() const
{
	if (StructHandle == INVALID_HANDLE) return 0;
	CUSMStructMeta* pStructMeta = (CUSMStructMeta*)IShaderMetadata::GetHandleData(StructHandle);
	return pStructMeta ? pStructMeta->Members.GetCount() : 0;
}
//---------------------------------------------------------------------

PShaderConstant CUSMShaderConstant::GetElement(U32 Index) const
{
	if (Index >= ElementCount) return NULL;

	//!!!can use pool!
	PUSMShaderConstant Const = n_new(CUSMShaderConstant);
	Const->Offset = Offset + Index * ElementSize;
	Const->StructHandle = StructHandle;
	Const->ElementCount = 1;
	Const->ElementSize = ElementSize;

	//!!!add structured buffer support (always is an array with 'any' number of elements, $Element is a type, may be struct, may be not!
	/*
	if (ElementIndex)
	{
		CUSMBufferMeta* pBufMeta = (CUSMBufferMeta*)IShaderMetadata::GetHandleData(pMeta->BufferHandle);
		n_assert_dbg(pBufMeta);
		switch (pBufMeta->Type)
		{
			case USMBuffer_Structured:	pConst->Offset = Offset + pBufMeta->Size * ElementIndex; break;
		}
	}
	*/


	return Const.GetUnsafe();
}
//---------------------------------------------------------------------

PShaderConstant CUSMShaderConstant::GetMember(CStrID Name) const
{
	CUSMStructMeta* pStructMeta = (CUSMStructMeta*)IShaderMetadata::GetHandleData(StructHandle);
	if (!pStructMeta) return NULL;

	for (CFixedArray<CUSMStructMemberMeta>::CIterator It = pStructMeta->Members.Begin(); It < pStructMeta->Members.End(); ++It)
		if (It->Name == Name)
		{
			//!!!can use pool!
			PUSMShaderConstant Const = n_new(CUSMShaderConstant);
			Const->Offset = Offset + It->Offset;
			Const->StructHandle = It->StructHandle;
			Const->ElementCount = It->ElementCount;
			Const->ElementSize = It->ElementSize;

			return Const.GetUnsafe();
		}

	return NULL;
}
//---------------------------------------------------------------------

void CUSMShaderConstant::SetRawValue(const CConstantBuffer& CB, const void* pData, UPTR Size) const
{
	n_assert_dbg(CB.IsA<CD3D11ConstantBuffer>());
	if (Size == WholeSize) Size = ElementCount * ElementSize;
	CD3D11ConstantBuffer& CB11 = (CD3D11ConstantBuffer&)CB;
	CB11.WriteData(Offset, pData, Size);
}
//---------------------------------------------------------------------

}
