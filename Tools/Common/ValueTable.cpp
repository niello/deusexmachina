#include "ValueTable.h"

void CValueTable::Clear()
{
	// This is much like a DeleteRowData() but checks type only once per column
	for (size_t ColIdx = 0; ColIdx < Columns.size(); ++ColIdx)
	{
		const Data::CType* Type = GetColumnValueType(ColIdx);

		//???use Type->Delete()? virtual call even if type will call empty destructor like !int()
		if (Type == DATA_TYPE(std::string))
			for (size_t RowIdx = 0; RowIdx < NumRows; RowIdx++)
				DATA_TYPE_NV(std::string)::Delete(GetValuePtr(ColIdx, RowIdx));
		else if (Type == DATA_TYPE(CBuffer))
			for (size_t RowIdx = 0; RowIdx < NumRows; RowIdx++)
				DATA_TYPE_NV(CBuffer)::Delete(GetValuePtr(ColIdx, RowIdx));
		else if (!Type)
			for (size_t RowIdx = 0; RowIdx < NumRows; RowIdx++)
				((Data::CData*)GetValuePtr(ColIdx, RowIdx))->Clear();
	}

	if (ValueBuffer)
	{
		free(ValueBuffer);
		ValueBuffer = nullptr;
	}

	if (RowStateBuffer)
	{
		free(RowStateBuffer);
		RowStateBuffer = nullptr;
	}

	NumRows = 0;
	AllocatedRows = 0;

	//???need?
/*	FirstNewRowIndex = std::numeric_limits<intptr_t>().max();
	NewRowsCount = 0;
	FirstDeletedRowIndex = std::numeric_limits<intptr_t>().max();
	DeletedRowsCount = 0;
*/
	_IsModified = false;
	_HasModifiedRows = false;
}
//---------------------------------------------------------------------

void CValueTable::Realloc(size_t NewPitch, size_t NewAllocRows)
{
	assert(NewAllocRows >= NumRows);
	assert(NewPitch >= RowPitch);

	AllocatedRows = NewAllocRows;
	int NewValueBufferSize = NewPitch * NewAllocRows;
	ValueBuffer = realloc(ValueBuffer, NewValueBufferSize);

	if (NewPitch != RowPitch)
	{
		size_t RowIdx = NumRows - 1;
		int PitchDiff = NewPitch - RowPitch;
		char* FromPtr = (char*)ValueBuffer + RowIdx * RowPitch;
		char* ToPtr = (char*)ValueBuffer + RowIdx * NewPitch;
		while (FromPtr > (char*)ValueBuffer)
		{
			memcpy(ToPtr, FromPtr, RowPitch);
			memset(ToPtr - PitchDiff, 0, PitchDiff);
			FromPtr -= RowPitch;
			ToPtr -= NewPitch;
		}
	}

	int LastRowDataEndOffset = NewPitch * (NumRows - 1) + RowPitch;
	memset((char*)ValueBuffer + LastRowDataEndOffset, 0, NewValueBufferSize - LastRowDataEndOffset);

	RowPitch = NewPitch;

	//???pad to 4 bytes (see buffer-wide operations & DWORD usage)?
	RowStateBuffer = (uint8_t*)realloc(RowStateBuffer, NewAllocRows);
	memset(RowStateBuffer + NumRows, 0, NewAllocRows - NumRows);
}
//---------------------------------------------------------------------

int CValueTable::UpdateColumnOffsets()
{
	int CurrOffset = 0;
	for (size_t i = 0; i < Columns.size(); ++i)
	{
		Columns[i].ByteOffset = CurrOffset;
		const Data::CType* Type = GetColumnValueType(i);
		CurrOffset += (Type) ? (IsSpecialType(Type) ? Type->GetSize() : sizeof(void*)) : sizeof(Data::CData);
	}
	return ((CurrOffset + 3) >> 2) << 2;
}
//---------------------------------------------------------------------

// Begin adding Columns. Columns can be added at any time, but it will
// be much more efficient when called between BeginAddColumns() and
// EndAddColumns(), since this will save a lot of re-allocations.
void CValueTable::BeginAddColumns()
{
	assert(!_InBeginAddColumns);
	_InBeginAddColumns = true;
	FirstAddedColIndex = Columns.size();
}
//---------------------------------------------------------------------

void CValueTable::AddColumn(CStrID ID, const Data::CType* Type, bool RecAsNewCol)
{
	if (HasColumn(ID)) return;

	ColumnInfo NewColInfo;
	NewColInfo.ID = ID;
	NewColInfo.Type = Type;
	Columns.push_back(std::move(NewColInfo));

	ColumnIndexMap.emplace(ID, Columns.size() - 1);

	if (_TrackModifications)
	{
		if (RecAsNewCol) NewColumnIndices.push_back(Columns.size() - 1);
		_IsModified = true;
	}

	if (!_InBeginAddColumns)
	{
		if (NumRows > 0) Realloc(UpdateColumnOffsets(), NumRows);
		else RowPitch = UpdateColumnOffsets();

		SetColumnToDefaultValues(Columns.size() - 1);
	}
}
//---------------------------------------------------------------------

void CValueTable::EndAddColumns()
{
	assert(_InBeginAddColumns);
	_InBeginAddColumns = false;

	if ((size_t)FirstAddedColIndex < Columns.size())
	{
		if (NumRows > 0) Realloc(UpdateColumnOffsets(), NumRows);
		else RowPitch = UpdateColumnOffsets();

		for (size_t ColIdx = FirstAddedColIndex; ColIdx < Columns.size(); ++ColIdx)
			SetColumnToDefaultValues(ColIdx);
	}
}
//---------------------------------------------------------------------

// This marks a row for deletion. Note that the row will only be marked
// for deletion, deleted row indices are returned with the GetDeletedRowIndices()
// call. The row will never be physically removed from memory!
void CValueTable::DeleteRow(size_t RowIdx)
{
	assert(IsRowValid(RowIdx));
	if (_TrackModifications)
	{
		if (FirstDeletedRowIndex > RowIdx) FirstDeletedRowIndex = RowIdx;
		
		RowStateBuffer[RowIdx] |= DeletedRow;
		++DeletedRowsCount;
		
		// NewRow flag has priority over DeletedRow flag, so clear it when new row deleted
		if (RowStateBuffer[RowIdx] & NewRow)
		{
			RowStateBuffer[RowIdx] &= ~NewRow;
			--NewRowsCount;
		}

		_IsModified = true;
	}
}
//---------------------------------------------------------------------

void CValueTable::SetValue(size_t ColIdx, size_t RowIdx, const Data::CData& Val)
{
	const Data::CType* Type = GetColumnValueType(ColIdx);
	assert(!Type || Type == Val.GetType());
	assert(IsRowValid(RowIdx));
	
	void** pObj = GetValuePtr(ColIdx, RowIdx);
	if (Type) Type->Copy(IsSpecialType(Type) ? (void**)&pObj : pObj, Val.GetValueObjectPtr());
	else *(Data::CData*)pObj = Val;
	
	if (_TrackModifications)
	{
		RowStateBuffer[RowIdx] |= UpdatedRow;
		_IsModified = true;
		_HasModifiedRows = true;
	}
}
//---------------------------------------------------------------------

// This creates a new row as a copy of an existing row. Returns the index of the new row.
// NOTE: the user data will be initialized to nullptr for the new row!
int CValueTable::CopyRow(size_t FromRowIdx)
{
	assert(FromRowIdx < NumRows);
	int ToRowIdx = AddRow();
	Data::CData Value; //???or inside the loop?
	for (size_t ColIdx = 0; ColIdx < Columns.size(); ColIdx++)
	{
		GetValue(ColIdx, FromRowIdx, Value);
		SetValue(ColIdx, ToRowIdx, Value);
		//!!!or switch-case GetColumnValueType(ColIdx) == bool: Set<bool>(Get<bool>()) etc
	}
	return ToRowIdx;
}
//---------------------------------------------------------------------

// Create a new row as a copy of a row in another value table. The layouts of the value tables
// must not match, since only matching Columns will be considered.
// NOTE: the user data will be initialised to nullptr for the new row.
int CValueTable::CopyExtRow(CValueTable* pOther, size_t OtherRowIdx, bool CreateMissingCols)
{
	assert(pOther);
	assert(OtherRowIdx < pOther->GetRowCount());
	int MyRowIdx = AddRow();
	Data::CData Value;
	for (size_t OtherColIdx = 0; OtherColIdx < pOther->GetColumnCount(); OtherColIdx++)
	{
		const ColumnInfo& ColInfo = Columns[OtherColIdx];
		if (CreateMissingCols) AddColumn(ColInfo.ID, ColInfo.Type);
		pOther->GetValue(OtherColIdx, OtherRowIdx, Value);
		SetValue(ColInfo.ID, MyRowIdx, Value); //???do we already know col idx if just added col?
	}
	return MyRowIdx;
}
//---------------------------------------------------------------------

// Adds an empty row at the end of the value buffer. The row will be
// marked invalid until the first value is set in the row. This will
// re-allocate the existing value buffer. If you know beforehand how
// many rows will exist in the table it is more efficient to use
// one SetNumRows(N) instead of N times AddRow()! The method returns
// the index of the newly added row. The row will be filled with
// the row attribute's default values.
int CValueTable::AddRow()
{
	if (NumRows >= AllocatedRows)
	{
		int NewNumRows = AllocatedRows + AllocatedRows;
		if (NewNumRows == 0) NewNumRows = 10;
		Realloc(RowPitch, NewNumRows);
	}

	if (_TrackModifications)
	{
		if (FirstNewRowIndex > NumRows) FirstNewRowIndex = NumRows;
		RowStateBuffer[NumRows] |= NewRow;
		++NewRowsCount;
		_IsModified = true;
	}

	SetRowToDefaultValues(NumRows++); // Copy to stack, then increment, then call. Intended, don't change.
	return NumRows - 1;
}
//---------------------------------------------------------------------

// Finds a row index by single attribute value. This method can be slow since
// it may search linearly (and vertically) through the table. 
// FIXME: keep row indices for indexed rows in nDictionaries?
std::vector<int> CValueTable::InternalFindRowIndicesByAttr(CStrID AttrID, const Data::CData& Value, bool FirstMatchOnly) const
{
	std::vector<int> Result;
	size_t ColIdx = GetColumnIndex(AttrID);
	const Data::CType* Type = GetColumnValueType(ColIdx);
	assert(Type == Value.GetType());
	for (size_t RowIdx = 0; RowIdx < GetRowCount(); RowIdx++)
	{
		if (IsRowValid(RowIdx))
		{
			void** pObj = GetValuePtr(ColIdx, RowIdx);
			if (Type->IsEqualT(Value.GetValueObjectPtr(), IsSpecialType(Type) ? (void*)pObj : *pObj))
			{
				Result.push_back(RowIdx);
				if (FirstMatchOnly) return Result;
			}
		}
	}
	return Result;
}
//---------------------------------------------------------------------

// Clears all data associated with cells of one row (string, blob and guid)    
void CValueTable::DeleteRowData(size_t RowIdx)
{
	for (size_t ColIdx = 0; ColIdx < Columns.size(); ++ColIdx)
	{
		const Data::CType* Type = GetColumnValueType(ColIdx);

		if (Type == DATA_TYPE(std::string))
			DATA_TYPE_NV(std::string)::Delete(GetValuePtr(ColIdx, RowIdx));
		else if (Type == DATA_TYPE(CBuffer))
			DATA_TYPE_NV(CBuffer)::Delete(GetValuePtr(ColIdx, RowIdx));
		else if (!Type)
			((Data::CData*)GetValuePtr(ColIdx, RowIdx))->Clear();
	}
}
//---------------------------------------------------------------------

void CValueTable::ClearDeletedRows()
{
	for (size_t RowIdx = FirstDeletedRowIndex; RowIdx < NumRows && DeletedRowsCount > 0; RowIdx++)
		if (IsRowDeleted(RowIdx))
		{
			--DeletedRowsCount;
			DeleteRowData(RowIdx);
			RowStateBuffer[RowIdx] = DestroyedRow; //???remember first & count? reuse only when NumRows == NumAllocRows
		}

	assert(!DeletedRowsCount);

	FirstDeletedRowIndex = std::numeric_limits<intptr_t>().max();
}
//---------------------------------------------------------------------
