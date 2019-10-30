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

UPTR CUSMConstant::GetMemberCount() const
{
	if (StructHandle == INVALID_HANDLE) return 0;
	CUSMStructMeta* pStructMeta = (CUSMStructMeta*)IShaderMetadata::GetHandleData(StructHandle);
	return pStructMeta ? pStructMeta->Members.GetCount() : 0;
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

void CUSMConstant::SetMatrix(const CConstantBuffer& CB, const matrix44* pValues, UPTR Count, U32 StartIndex) const
{
	CD3D11ConstantBuffer& CB11 = (CD3D11ConstantBuffer&)CB;

	n_assert_dbg(StartIndex < ElementCount);

	if (Flags & ShaderConst_ColumnMajor)
	{
		n_assert_dbg(Columns > 0 && Rows > 0);

		 // The maximum is 16 floats, may be less
		float TransposedData[16];
		const UPTR MatrixSize = Columns * Rows * sizeof(float);

		const matrix44* pCurrMatrix = pValues;
		const matrix44* pEndMatrix = pValues + Count;
		for (; pCurrMatrix < pEndMatrix; ++pCurrMatrix)
		{
			float* pCurrData = TransposedData;
			for (U32 Col = 0; Col < Columns; ++Col)
				for (U32 Row = 0; Row < Rows; ++Row)
				{
					*pCurrData = pCurrMatrix->m[Row][Col];
					++pCurrData;
				}

			//!!!need total columns used, check float3x3, is it 9 or 12 floats?!
			NOT_IMPLEMENTED;
			CB11.WriteData(Offset + MatrixSize * StartIndex, &TransposedData, MatrixSize);
		}
	}
	else
	{
		const UPTR DataSize = 4 * Rows * sizeof(float);
		UPTR CurrOffset = Offset + DataSize * StartIndex;
		if (DataSize == sizeof(matrix44))
		{
			CB11.WriteData(CurrOffset, pValues, sizeof(matrix44) * Count);
		}
		else
		{
			const matrix44* pCurrValue = pValues;
			for (UPTR i = 0; i < Count; ++i)
			{
				CB11.WriteData(CurrOffset, pValues, DataSize);
				CurrOffset += DataSize;
				pCurrValue += sizeof(matrix44);
			}
		}
	}
}
//---------------------------------------------------------------------

}
