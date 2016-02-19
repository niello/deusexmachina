#include "ValueTable.h"

namespace DB
{

void CValueTable::Clear()
{
	// This is much like a DeleteRowData() but checks type only once per column
	for (UPTR ColIdx = 0; ColIdx < Columns.GetCount(); ++ColIdx)
	{
		const Data::CType* Type = GetColumnValueType(ColIdx);

		//???use Type->Delete()? virtual call even if type will call empty destructor like !int()
		if (Type == TString)
			for (UPTR RowIdx = 0; RowIdx < NumRows; RowIdx++)
				DATA_TYPE_NV(CString)::Delete(GetValuePtr(ColIdx, RowIdx));
		else if (Type == DATA_TYPE(Data::CBuffer))
			for (UPTR RowIdx = 0; RowIdx < NumRows; RowIdx++)
				DATA_TYPE_NV(Data::CBuffer)::Delete(GetValuePtr(ColIdx, RowIdx));
		else if (!Type)
			for (UPTR RowIdx = 0; RowIdx < NumRows; RowIdx++)
				((Data::CData*)GetValuePtr(ColIdx, RowIdx))->Clear();
	}

	SAFE_FREE(ValueBuffer);
	SAFE_FREE(RowStateBuffer);

	NumRows = 0;
	AllocatedRows = 0;

	//???need?
/*	FirstNewRowIndex = INTPTR_MAX;
	NewRowsCount = 0;
	FirstDeletedRowIndex = INTPTR_MAX;
	DeletedRowsCount = 0;
*/
	Flags.Clear(_IsModified);
	Flags.Clear(_HasModifiedRows);
}
//---------------------------------------------------------------------

void CValueTable::Realloc(UPTR NewPitch, UPTR NewAllocRows)
{
	n_assert(NewAllocRows >= NumRows);
	n_assert(NewPitch >= RowPitch);

	AllocatedRows = NewAllocRows;
	int NewValueBufferSize = NewPitch * NewAllocRows;
	ValueBuffer = n_realloc(ValueBuffer, NewValueBufferSize);

	if (NewPitch != RowPitch)
	{
		UPTR RowIdx = NumRows - 1;
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
	RowStateBuffer = (U8*)n_realloc(RowStateBuffer, NewAllocRows);
	memset(RowStateBuffer + NumRows, 0, NewAllocRows - NumRows);
}
//---------------------------------------------------------------------

int CValueTable::UpdateColumnOffsets()
{
	int CurrOffset = 0;
	for (UPTR i = 0; i < Columns.GetCount(); ++i)
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
	n_assert(!Flags.Is(_InBeginAddColumns));
	Flags.Set(_InBeginAddColumns);
	FirstAddedColIndex = Columns.GetCount();
	ColumnIndexMap.BeginAdd();
}
//---------------------------------------------------------------------

void CValueTable::AddColumn(CStrID ID, const Data::CType* Type, bool RecAsNewCol)
{
	if (ColumnIndexMap.Contains(ID)) return;

	ColumnInfo NewColInfo;
	NewColInfo.ID = ID;
	NewColInfo.Type = Type;
	Columns.Add(NewColInfo);

	ColumnIndexMap.Add(ID, Columns.GetCount() - 1);

	if (Flags.Is(_TrackModifications))
	{
		if (RecAsNewCol) NewColumnIndices.Add(Columns.GetCount() - 1);
		Flags.Set(_IsModified);
	}

	if (!Flags.Is(_InBeginAddColumns))
	{
		if (NumRows > 0) Realloc(UpdateColumnOffsets(), NumRows);
		else RowPitch = UpdateColumnOffsets();

		SetColumnToDefaultValues(Columns.GetCount() - 1);
	}
}
//---------------------------------------------------------------------

void CValueTable::EndAddColumns()
{
	n_assert(Flags.Is(_InBeginAddColumns));
	Flags.Clear(_InBeginAddColumns);
	ColumnIndexMap.EndAdd();

	if ((UPTR)FirstAddedColIndex < Columns.GetCount())
	{
		if (NumRows > 0) Realloc(UpdateColumnOffsets(), NumRows);
		else RowPitch = UpdateColumnOffsets();

		for (UPTR ColIdx = FirstAddedColIndex; ColIdx < Columns.GetCount(); ++ColIdx)
			SetColumnToDefaultValues(ColIdx);
	}
}
//---------------------------------------------------------------------

// This marks a row for deletion. Note that the row will only be marked
// for deletion, deleted row indices are returned with the GetDeletedRowIndices()
// call. The row will never be physically removed from memory!
void CValueTable::DeleteRow(UPTR RowIdx)
{
	n_assert(IsRowValid(RowIdx));
	if (Flags.Is(_TrackModifications))
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

		Flags.Set(_IsModified);
	}
}
//---------------------------------------------------------------------

void CValueTable::SetValue(UPTR ColIdx, UPTR RowIdx, const Data::CData& Val)
{
	const Data::CType* Type = GetColumnValueType(ColIdx);
	n_assert(!Type || Type == Val.GetType());
	n_assert(IsRowValid(RowIdx));
	
	void** pObj = GetValuePtr(ColIdx, RowIdx);
	if (Type) Type->Copy(IsSpecialType(Type) ? (void**)&pObj : pObj, Val.GetValueObjectPtr());
	else *(Data::CData*)pObj = Val;
	
	if (Flags.Is(_TrackModifications))
	{
		RowStateBuffer[RowIdx] |= UpdatedRow;
		Flags.Set(_IsModified);
		Flags.Set(_HasModifiedRows);
	}
}
//---------------------------------------------------------------------

// This creates a new row as a copy of an existing row. Returns the index of the new row.
// NOTE: the user data will be initialized to NULL for the new row!
int CValueTable::CopyRow(UPTR FromRowIdx)
{
	n_assert(FromRowIdx < NumRows);
	int ToRowIdx = AddRow();
	Data::CData Value; //???or inside the loop?
	for (UPTR ColIdx = 0; ColIdx < Columns.GetCount(); ColIdx++)
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
// NOTE: the user data will be initialised to NULL for the new row.
int CValueTable::CopyExtRow(CValueTable* pOther, UPTR OtherRowIdx, bool CreateMissingCols)
{
	n_assert(pOther);
	n_assert(OtherRowIdx < pOther->GetRowCount());
	int MyRowIdx = AddRow();
	Data::CData Value;
	for (UPTR OtherColIdx = 0; OtherColIdx < pOther->GetColumnCount(); OtherColIdx++)
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

	if (Flags.Is(_TrackModifications))
	{
		if (FirstNewRowIndex > NumRows) FirstNewRowIndex = NumRows;
		RowStateBuffer[NumRows] |= NewRow;
		++NewRowsCount;
		Flags.Set(_IsModified);
	}

	SetRowToDefaultValues(NumRows++); // Copy to stack, then increment, then call. Intended, don't change.
	return NumRows - 1;
}
//---------------------------------------------------------------------

// Finds a row index by single attribute value. This method can be slow since
// it may search linearly (and vertically) through the table. 
// FIXME: keep row indices for indexed rows in nDictionaries?
CArray<int> CValueTable::InternalFindRowIndicesByAttr(CStrID AttrID, const Data::CData& Value, bool FirstMatchOnly) const
{
	CArray<int> Result;
	UPTR ColIdx = GetColumnIndex(AttrID);
	const Data::CType* Type = GetColumnValueType(ColIdx);
	n_assert(Type == Value.GetType());
	for (UPTR RowIdx = 0; RowIdx < GetRowCount(); RowIdx++)
	{
		if (IsRowValid(RowIdx))
		{
			void** pObj = GetValuePtr(ColIdx, RowIdx);
			if (Type->IsEqualT(Value.GetValueObjectPtr(), IsSpecialType(Type) ? (void*)pObj : *pObj))
			{
				Result.Add(RowIdx);
				if (FirstMatchOnly) return Result;
			}
		}
	}
	return Result;
}
//---------------------------------------------------------------------

// Clears all data associated with cells of one row (string, blob and guid)    
void CValueTable::DeleteRowData(UPTR RowIdx)
{
	for (UPTR ColIdx = 0; ColIdx < Columns.GetCount(); ++ColIdx)
	{
		const Data::CType* Type = GetColumnValueType(ColIdx);

		if (Type == TString)
			DATA_TYPE_NV(CString)::Delete(GetValuePtr(ColIdx, RowIdx));
		else if (Type == DATA_TYPE(Data::CBuffer))
			DATA_TYPE_NV(Data::CBuffer)::Delete(GetValuePtr(ColIdx, RowIdx));
		else if (!Type)
			((Data::CData*)GetValuePtr(ColIdx, RowIdx))->Clear();
	}
}
//---------------------------------------------------------------------

void CValueTable::ClearDeletedRows()
{
	for (UPTR RowIdx = FirstDeletedRowIndex; RowIdx < NumRows && DeletedRowsCount > 0; RowIdx++)
		if (IsRowDeleted(RowIdx))
		{
			--DeletedRowsCount;
			DeleteRowData(RowIdx);
			RowStateBuffer[RowIdx] = DestroyedRow; //???remember first & count? reuse only when NumRows == NumAllocRows
		}

	n_assert(!DeletedRowsCount);

	FirstDeletedRowIndex = INTPTR_MAX;
}
//---------------------------------------------------------------------

}