#include "ShaderParamTable.h"
#include <algorithm>

namespace Render
{
__ImplementClassNoFactory(IConstantBufferParam, Core::CObject);

// DEM matrices are row-major
void IShaderConstantParam::SetMatrices(CConstantBuffer& CB, const matrix44* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	const auto Rows = GetRowCount();
	const auto Columns = GetColumnCount();
	n_assert_dbg(Columns > 0 && Rows > 0);

	if (IsColumnMajor())
	{
		//!!!limit count!

		const matrix44* pCurrMatrix = pValue;
		const matrix44* pEndMatrix = pValue + Count;
		for (; pCurrMatrix < pEndMatrix; ++pCurrMatrix)
		{
			const matrix44 Transposed = pCurrMatrix->transposed();

			NOT_IMPLEMENTED;
			// set array element
		}
	}
	else
	{
		if (Columns == 4)
		{
			if (Rows == 4)
			{
				// Copy matrices without breaking
				SetFloats(CB, *pValue->m, 16 * Count);
			}
			else
			{
				// Copy rows without breaking
			}
		}
		else
		{
			// Copy row by row
		}
	}
}
//---------------------------------------------------------------------

/*

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

void CSM30Constant::SetMatrix(const CConstantBuffer& CB, const matrix44* pValues, UPTR Count, U32 StartIndex) const
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
			NOT_IMPLEMENTED;
			CB9.WriteData(RegSet, Offset + MatrixSize * StartIndex, &TransposedData, MatrixSize);
		}
	}
	else
	{
		//!!!check columns and rows! (really need to check only rows, as all columns are used)
		NOT_IMPLEMENTED;
		CB9.WriteData(RegSet, Offset + sizeof(matrix44) * StartIndex, pValues, sizeof(matrix44) * Count);
	}
}
//---------------------------------------------------------------------

*/

CShaderParamTable::CShaderParamTable(std::vector<PShaderConstantParam>&& Constants,
	std::vector<PConstantBufferParam>&& ConstantBuffers,
	std::vector<PResourceParam>&& Resources,
	std::vector<PSamplerParam>&& Samplers)

	: _Constants(std::move(Constants))
	, _ConstantBuffers(std::move(ConstantBuffers))
	, _Resources(std::move(Resources))
	, _Samplers(std::move(Samplers))
{
	std::sort(_Constants.begin(), _Constants.end(), [](const PShaderConstantParam& a, const PShaderConstantParam& b)
	{
		return a->GetID() < b->GetID();
	});
	std::sort(_ConstantBuffers.begin(), _ConstantBuffers.end(), [](const PConstantBufferParam& a, const PConstantBufferParam& b)
	{
		return a->GetID() < b->GetID();
	});
	std::sort(_Resources.begin(), _Resources.end(), [](const PResourceParam& a, const PResourceParam& b)
	{
		return a->GetID() < b->GetID();
	});
	std::sort(_Samplers.begin(), _Samplers.end(), [](const PSamplerParam& a, const PSamplerParam& b)
	{
		return a->GetID() < b->GetID();
	});
}
//---------------------------------------------------------------------

}
