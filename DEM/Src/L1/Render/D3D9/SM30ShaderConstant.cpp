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
		BufferHandle = pMeta->BufferHandle;
		StructHandle = pMeta->StructHandle;
		ElementCount = pMeta->ElementCount;
		ElementRegisterCount = pMeta->ElementRegisterCount;
		Flags = pMeta->Flags;

//!!!FILL!
Columns = 0;
Rows = 0;

		//!!!for mixed calculate per-member!
		SizeInBytes = ElementCount * ElementRegisterCount;
		switch (RegSet)
		{
			case Reg_Float4:	SizeInBytes *= sizeof(float) * 4; break;
			case Reg_Int4:		SizeInBytes *= sizeof(int) * 4; break;
			case Reg_Bool:		SizeInBytes *= sizeof(BOOL); break;
		}

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
	Const->Columns = Columns;
	Const->Rows = Rows;
	Const->Flags = Flags;

	return Const.GetUnsafe();
}
//---------------------------------------------------------------------

PShaderConstant CSM30ShaderConstant::GetMember(CStrID Name) const
{
	CSM30StructMeta* pStructMeta = (CSM30StructMeta*)IShaderMetadata::GetHandleData(StructHandle);
	if (!pStructMeta) return NULL;

	//???sort?
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
			Const->Columns = 0; //It->Columns;
			Const->Rows = 0; //It->Rows;
			Const->Flags = It->Flags;

			return Const.GetUnsafe();
		}

	return NULL;
}
//---------------------------------------------------------------------

void CSM30ShaderConstant::SetRawValue(const CConstantBuffer& CB, const void* pData, UPTR Size) const
{
	n_assert_dbg(RegSet != Reg_Invalid && CB.IsA<CD3D9ConstantBuffer>());
	if (Size == WholeSize) Size = SizeInBytes;
	CD3D9ConstantBuffer& CB9 = (CD3D9ConstantBuffer&)CB;
	CB9.WriteData(RegSet, Offset, pData, Size);
}
//---------------------------------------------------------------------

void CSM30ShaderConstant::SetUInt(const CConstantBuffer& CB, U32 Value) const
{
	CD3D9ConstantBuffer& CB9 = (CD3D9ConstantBuffer&)CB;
	switch (RegSet)
	{
		case Reg_Int4:
		{
			CB9.WriteData(Reg_Int4, Offset, &Value, sizeof(U32));
			break;
		}
		case Reg_Float4:
		{
			float FloatValue = (float)Value;
			CB9.WriteData(Reg_Float4, Offset, &FloatValue, sizeof(float));
			break;
		}
		case Reg_Bool:
		{
			BOOL BoolValue = (Value != 0);
			CB9.WriteData(Reg_Bool, Offset, &BoolValue, sizeof(BOOL));
			break;
		}
	}
}
//---------------------------------------------------------------------

void CSM30ShaderConstant::SetFloat(const CConstantBuffer& CB, const float* pValues, UPTR Count) const
{
	CD3D9ConstantBuffer& CB9 = (CD3D9ConstantBuffer&)CB;

	switch (RegSet)
	{
		case Reg_Float4:
		{
			CB9.WriteData(Reg_Float4, Offset, pValues, sizeof(float) * Count);
			break;
		}
		case Reg_Int4:
		{
			U32 CurrOffset = Offset;
			for (UPTR i = 0; i < Count; ++i)
			{
				U32 IntValue = (U32)pValues[i];
				CB9.WriteData(Reg_Int4, CurrOffset, &IntValue, sizeof(U32));
				CurrOffset += sizeof(U32);
			}
			break;
		}
	}
}
//---------------------------------------------------------------------

void CSM30ShaderConstant::SetMatrix(const CConstantBuffer& CB, const matrix44* pValues, UPTR Count, U32 StartIndex) const
{
	if (RegSet != Reg_Float4) return;

	n_assert_dbg(StartIndex < ElementCount);

	CD3D9ConstantBuffer& CB9 = (CD3D9ConstantBuffer&)CB;

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
			CB9.WriteData(RegSet, Offset + MatrixSize * StartIndex, &TransposedData, MatrixSize);
		}
	}
	else
	{
		//!!!check columns and rows! (really need to check only rows, as all columns are used)
		CB9.WriteData(RegSet, Offset + sizeof(matrix44) * StartIndex, pValues, sizeof(matrix44) * Count);
	}
}
//---------------------------------------------------------------------

}
