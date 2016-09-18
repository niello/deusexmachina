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
	BufferHandle = pMeta->BufferHandle;
	StructHandle = pMeta->StructHandle;
	ElementCount = pMeta->ElementCount;
	ElementSize = pMeta->ElementSize;
	Flags = pMeta->Flags;

//!!!FILL!
Columns = 0;
Rows = 0;

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

	return Const.GetUnsafe();
}
//---------------------------------------------------------------------

PShaderConstant CUSMShaderConstant::GetMember(CStrID Name) const
{
	CUSMStructMeta* pStructMeta = (CUSMStructMeta*)IShaderMetadata::GetHandleData(StructHandle);
	if (!pStructMeta) return NULL;

	//???sort?
	for (CFixedArray<CUSMStructMemberMeta>::CIterator It = pStructMeta->Members.Begin(); It < pStructMeta->Members.End(); ++It)
		if (It->Name == Name)
		{
			//!!!can use pool!
			PUSMShaderConstant Const = n_new(CUSMShaderConstant);
			Const->Offset = Offset + It->Offset;
			Const->StructHandle = It->StructHandle;
			Const->ElementCount = It->ElementCount;
			Const->ElementSize = It->ElementSize;
			Const->Columns = 0; //It->Columns;
			Const->Rows = 0; //It->Rows;
			Const->Flags = It->Flags;

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

void CUSMShaderConstant::SetUInt(const CConstantBuffer& CB, U32 Value) const
{
	//???switch type, convert to const type?
	CD3D11ConstantBuffer& CB11 = (CD3D11ConstantBuffer&)CB;
	CB11.WriteData(Offset, &Value, sizeof(U32));
}
//---------------------------------------------------------------------

void CUSMShaderConstant::SetFloat(const CConstantBuffer& CB, const float* pValues, UPTR Count) const
{
	//???switch type, convert to const type?
	CD3D11ConstantBuffer& CB11 = (CD3D11ConstantBuffer&)CB;
	CB11.WriteData(Offset, pValues, sizeof(float) * Count);
}
//---------------------------------------------------------------------

void CUSMShaderConstant::SetMatrix(const CConstantBuffer& CB, const matrix44* pValues, UPTR Count, U32 StartIndex) const
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
			CB11.WriteData(Offset + MatrixSize * StartIndex, &TransposedData, MatrixSize);
		}
	}
	else
	{
		//!!!check columns and rows! (really need to check only rows, as all columns are used)
		CB11.WriteData(Offset + sizeof(matrix44) * StartIndex, pValues, sizeof(matrix44) * Count);
	}
}
//---------------------------------------------------------------------

}
