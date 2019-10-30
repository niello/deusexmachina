#include "USMShaderConstant.h"

#include <Render/D3D11/USMShaderMetadata.h>
#include <Render/D3D11/D3D11ConstantBuffer.h>

namespace Render
{

//???process column-major differently?
U32 CUSMConstant::GetComponentOffset(U32 ComponentIndex) const
{
	n_assert_dbg(StructHandle == INVALID_HANDLE);

	const U32 ComponentsPerElement = Columns * Rows;
	const U32 Elm = ComponentIndex / ComponentsPerElement;
	ComponentIndex = ComponentIndex - Elm * ComponentsPerElement;
	const U32 Row = ComponentIndex / Columns;
	const U32 Col = ComponentIndex - Row * Columns;

	const U32 ComponentSize = 4; // 32-bit components only for now, really must depend on const element type
	const U32 ComponentsPerAlignedRow = 4; // Even for, say, float3x3, each row uses full 4-component register

	return Offset + ElementSize * Elm + (Row * ComponentsPerAlignedRow + Col) * ComponentSize; // In bytes
}
//---------------------------------------------------------------------

PShaderConstant CUSMConstant::GetElement(U32 Index) const
{
	if (Index >= ElementCount) return nullptr;

	//!!!can use pool!
	PUSMConstant Const = n_new(CUSMConstant);
	Const->Offset = Offset + Index * ElementSize;
	Const->StructHandle = StructHandle;
	Const->ElementCount = 1;
	Const->ElementSize = ElementSize;
	Const->Columns = Columns;
	Const->Rows = Rows;
	Const->Flags = Flags;

	//!!!add structured buffer support (always is an array with 'any' number of elements, $Element is a type, may be struct, may be not!
	// Structured buffer has only '$Element' structure(?)
	/*
	if (ElementIndex)
	{
		CUSMBufferMeta* pBufMeta = (CUSMBufferMeta*)IShaderMetadata::GetHandleData(pMeta->BufferHandle);
		if (pBufMeta->Type == USMBuffer_Structured)
			pConst->Offset = Offset + pBufMeta->Size * ElementIndex; break;
	}
	*/

	return Const.Get();
}
//---------------------------------------------------------------------

PShaderConstant CUSMConstant::GetMember(CStrID Name) const
{
	CUSMStructMeta* pStructMeta = (CUSMStructMeta*)IShaderMetadata::GetHandleData(StructHandle);
	if (!pStructMeta) return nullptr;

	//???sort?
	for (CFixedArray<CUSMStructMemberMeta>::CIterator It = pStructMeta->Members.Begin(); It < pStructMeta->Members.End(); ++It)
		if (It->Name == Name)
		{
			//!!!can use pool!
			PUSMConstant Const = n_new(CUSMConstant);
			Const->Offset = Offset + It->Offset;
			Const->StructHandle = It->StructHandle;
			Const->ElementCount = It->ElementCount;
			Const->ElementSize = It->ElementSize;
			Const->Columns = It->Columns;
			Const->Rows = It->Rows;
			Const->Flags = It->Flags;

			return Const.Get();
		}

	return nullptr;
}
//---------------------------------------------------------------------

void CUSMConstant::SetUIntComponent(const CConstantBuffer& CB, U32 ComponentIndex, U32 Value) const
{
	//???switch type, convert to const type?
	if (StructHandle != INVALID_HANDLE) return;
	CD3D11ConstantBuffer& CB11 = (CD3D11ConstantBuffer&)CB;
	CB11.WriteData(GetComponentOffset(ComponentIndex), &Value, sizeof(U32));
}
//---------------------------------------------------------------------

void CUSMConstant::SetSIntComponent(const CConstantBuffer& CB, U32 ComponentIndex, I32 Value) const
{
	//???switch type, convert to const type?
	if (StructHandle != INVALID_HANDLE) return;
	CD3D11ConstantBuffer& CB11 = (CD3D11ConstantBuffer&)CB;
	CB11.WriteData(GetComponentOffset(ComponentIndex), &Value, sizeof(U32));
}
//---------------------------------------------------------------------

}
