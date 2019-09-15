#pragma once
#include <Data.h>
#include <StringID.h>
#include <vector>
#include <map>
#include <cassert>

#undef min
#undef max

class CValueTable
{
private:

	struct ColumnInfo
	{
		CStrID				ID;
		const Data::CType*	Type;
		int					ByteOffset;
	};

	enum
	{
		NewRow			= 0x01,
		UpdatedRow		= 0x02,
		DeletedRow		= 0x04,	// Marks row to be deleted at the next commit
		DestroyedRow	= 0x08	// Marks row as not existing in DB and invalid (reusable) in ValueTable
	};

	void*					ValueBuffer = nullptr;
	uint8_t*				RowStateBuffer = nullptr;

	std::vector<ColumnInfo>	Columns;
	std::map<CStrID, int>	ColumnIndexMap;		// map attribute ID to column index
	std::vector<int>		NewColumnIndices;	// indices of new Columns since last ResetModifiedState

	size_t					FirstAddedColIndex = std::numeric_limits<size_t>().max();
	size_t					FirstNewRowIndex = std::numeric_limits<size_t>().max();
	size_t					NewRowsCount = 0;
	size_t					FirstDeletedRowIndex = std::numeric_limits<size_t>().max();
	int						DeletedRowsCount = 0;

	size_t					RowPitch = 0;			// pitch of a row in bytes
	size_t					NumRows = 0;			// number of rows
	size_t					AllocatedRows = 0;		// number of allocated rows

	bool					_TrackModifications : 1;
	bool					_IsModified : 1;
	bool					_HasModifiedRows : 1;
	bool					_InBeginAddColumns : 1;

	void		Realloc(size_t NewPitch, size_t NewAllocRows);
	int			UpdateColumnOffsets();

	void**		GetValuePtr(size_t ColIdx, size_t RowIdx) const;

	std::vector<int>	InternalFindRowIndicesByAttr(CStrID AttrID, const Data::CData& Value, bool FirstMatchOnly) const;

	void		SetColumnToDefaultValues(size_t ColIdx);
	void		SetRowToDefaultValues(size_t RowIdx);

	bool		IsSpecialType(const Data::CType* T) const { return false; } // { return T == TVector4 || T == TMatrix44; }

public:

	CValueTable();
	~CValueTable() { Clear(); }

	void				BeginAddColumns();
	void				AddColumn(CStrID ID, const Data::CType* Type, bool IsNew = true);
	void				EndAddColumns();
	bool				HasColumn(CStrID ID) const { return ColumnIndexMap.find(ID) != ColumnIndexMap.cend(); }
	int					GetColumnIndex(CStrID ID) const;
	size_t				GetColumnCount() const { return Columns.size(); }
	CStrID				GetColumnID(size_t ColIdx) const { return Columns[ColIdx].ID; }
	const Data::CType*	GetColumnValueType(size_t ColIdx) const { return Columns[ColIdx].Type; }
	const std::vector<int>&	GetNewColumnIndices() { return NewColumnIndices; }

	void				Clear();
	void				ReserveRows(size_t NumRows) { assert(NumRows > 0); Realloc(RowPitch, AllocatedRows + NumRows); }
	int					AddRow();
	int					CopyRow(size_t FromRowIdx);
	int					CopyExtRow(CValueTable* pOther, size_t OtherRowIdx, bool CreateMissingCols = false);
	void				DeleteRow(size_t RowIdx);
	void				DeleteAllRows() { for (size_t i = 0; i < NumRows; i++) DeleteRow(i); }
	void				DeleteRowData(size_t RowIdx);

	uint8_t				GetRowState(size_t RowIdx) const;
	bool				IsRowNew(size_t RowIdx) const;
	bool				IsRowUpdated(size_t RowIdx) const;
	bool				IsRowOnlyUpdated(size_t RowIdx) const;
	bool				IsRowDeleted(size_t RowIdx) const;
	bool				IsRowValid(size_t RowIdx) const;
	bool				IsRowUntouched(size_t RowIdx) const;
	bool				IsRowModified(size_t RowIdx) const;
	bool				IsModified() const { return _IsModified; }
	bool				HasModifiedRows() const { return _HasModifiedRows; }
	void				TrackModifications(bool Track) { _TrackModifications = Track; }
	bool				IsTrackingModifications() const { return _TrackModifications; }
	void				ClearNewRowStats();
	void				ClearDeletedRows();
	void				ClearRowFlags(size_t RowIdx);
	void				ResetModifiedState();
	size_t				GetRowCount() const { return NumRows; }
	int					GetFirstNewRowIndex() const { return FirstNewRowIndex; }
	int					GetNewRowsCount() const { return NewRowsCount; }
	int					GetFirstDeletedRowIndex() const { return FirstDeletedRowIndex; }
	int					GetDeletedRowsCount() const { return DeletedRowsCount; }

	std::vector<int>	FindRowIndicesByValue(CStrID AttrID, const Data::CData& Value, bool FirstMatchOnly) const;
	int					FindRowIndexByValue(CStrID AttrID, const Data::CData& Value) const;

	void				GetValue(size_t ColIdx, size_t RowIdx, Data::CData& Val) const;
	void				GetValue(CStrID AttrID, size_t RowIdx, Data::CData& Val) const { GetValue(ColumnIndexMap.at(AttrID), RowIdx, Val); }
	template<class T>
	const T&			Get(size_t ColIdx, size_t RowIdx) const;
	template<class T>
	const T&			Get(CStrID AttrID, size_t RowIdx) const { return Get<T>(ColumnIndexMap.at(AttrID), RowIdx); }
	void				SetValue(size_t ColIdx, size_t RowIdx, const Data::CData& Val);
	void				SetValue(CStrID AttrID, size_t RowIdx, const Data::CData& Val) { SetValue(ColumnIndexMap.at(AttrID), RowIdx, Val); }
	template<class T>
	void				Set(size_t ColIdx, size_t RowIdx, const T& Val);
	template<class T>
	void				Set(CStrID AttrID, size_t RowIdx, const T& Val) { Set(ColumnIndexMap.at(AttrID), RowIdx, Val); }
};

inline CValueTable::CValueTable():
	_TrackModifications(true),
	_IsModified(false),
	_HasModifiedRows(false),
	_InBeginAddColumns(false)
{
}
//---------------------------------------------------------------------

inline void CValueTable::ClearNewRowStats()
{
	FirstNewRowIndex = std::numeric_limits<intptr_t>().max();
	NewRowsCount = 0;
}
//---------------------------------------------------------------------

// Doesn't update modification statistics
inline void CValueTable::ClearRowFlags(size_t RowIdx)
{
	assert(RowIdx < NumRows);
	RowStateBuffer[RowIdx] = 0;
}
//---------------------------------------------------------------------

inline void CValueTable::ResetModifiedState()
{
	_IsModified = false;
	_HasModifiedRows = false;
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowModified(size_t RowIdx) const
{
	assert(RowIdx < NumRows);
	return RowStateBuffer[RowIdx] != 0;
}
//---------------------------------------------------------------------

inline uint8_t CValueTable::GetRowState(size_t RowIdx) const
{
	assert(RowIdx < NumRows);
	return RowStateBuffer[RowIdx];
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowNew(size_t RowIdx) const
{
	assert(RowIdx < NumRows);
	return (RowStateBuffer[RowIdx] & NewRow) != 0;
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowUpdated(size_t RowIdx) const
{
	assert(RowIdx < NumRows);
	return (RowStateBuffer[RowIdx] & UpdatedRow) != 0;
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowOnlyUpdated(size_t RowIdx) const
{
	assert(RowIdx < NumRows);
	return RowStateBuffer[RowIdx] == UpdatedRow;
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowDeleted(size_t RowIdx) const
{
	assert(RowIdx < NumRows);
	return (RowStateBuffer[RowIdx] & DeletedRow) != 0;
}
//---------------------------------------------------------------------

// Returns true if row isn't deleted or destroyed
inline bool CValueTable::IsRowValid(size_t RowIdx) const
{
	assert(RowIdx < NumRows); //???incorporate into return value?
	return (RowStateBuffer[RowIdx] & (DeletedRow | DestroyedRow)) == 0;
}
//---------------------------------------------------------------------

inline bool CValueTable::IsRowUntouched(size_t RowIdx) const
{
	assert(RowIdx < NumRows); //???incorporate into return value?
	return RowStateBuffer[RowIdx] == 0;
}
//---------------------------------------------------------------------

inline int CValueTable::GetColumnIndex(CStrID ID) const
{
	auto It = ColumnIndexMap.find(ID);
	return (It != ColumnIndexMap.cend()) ? It->second : -1;
}
//---------------------------------------------------------------------

inline void** CValueTable::GetValuePtr(size_t ColIdx, size_t RowIdx) const
{
	assert(ColIdx < Columns.size() && RowIdx < NumRows);
	return (void**)((char*)ValueBuffer + RowIdx * RowPitch + Columns[ColIdx].ByteOffset);
}
//---------------------------------------------------------------------

template<class T> inline void CValueTable::Set(size_t ColIdx, size_t RowIdx, const T& Val)
{
	const Data::CType* Type = GetColumnValueType(ColIdx);
	assert(!Type || Type == DATA_TYPE(T));
	assert(IsRowValid(RowIdx));

	void** pObj = GetValuePtr(ColIdx, RowIdx);
	if (Type) DATA_TYPE_NV(T)::CopyT(IsSpecialType(Type) ? (void**)&pObj : pObj, &Val);
	else *(Data::CData*)pObj = Val;

	if (_TrackModifications)
	{
		RowStateBuffer[RowIdx] |= UpdatedRow;
		_IsModified = true;
		_HasModifiedRows = true;
	}
}
//---------------------------------------------------------------------

inline void CValueTable::GetValue(size_t ColIdx, size_t RowIdx, Data::CData& Val) const
{
	const Data::CType* Type = GetColumnValueType(ColIdx);
	void** pObj = GetValuePtr(ColIdx, RowIdx);
	if (Type) Val.SetTypeValue(Type, IsSpecialType(Type) ? (void**)&pObj : pObj);
	else Val = *(Data::CData*)pObj;
}
//---------------------------------------------------------------------

template<class T> inline const T& CValueTable::Get(size_t ColIdx, size_t RowIdx) const
{
	const Data::CType* Type = GetColumnValueType(ColIdx);
	assert(!Type || Type == DATA_TYPE(T));
	void** pObj = GetValuePtr(ColIdx, RowIdx);
	return (Type) ?
		*(T*)DATA_TYPE_NV(T)::GetPtr(IsSpecialType(Type) ? (void**)&pObj : pObj) :
		((Data::CData*)pObj)->GetValue<T>();
}
//---------------------------------------------------------------------

// Finds multiple row indices by matching attribute. This method can be slow since
// it may search linearly (and vertically) through the table.
inline std::vector<int> CValueTable::FindRowIndicesByValue(CStrID AttrID, const Data::CData& Value, bool FirstMatchOnly) const
{
	return InternalFindRowIndicesByAttr(AttrID, Value, FirstMatchOnly);
}
//---------------------------------------------------------------------

// Finds multiple row indices by multiple matching attribute. This method can be slow since
// it may search linearly (and vertically) through the table.
//inline nArray<int> CValueTable::FindRowIndicesByAttrs(const nArray<CAttr>& Attrs, bool FirstMatchOnly) const
//{
//	return InternalFindRowIndicesByAttrs(Attrs, FirstMatchOnly);
//}
////---------------------------------------------------------------------

// Finds single row index by matching attribute. This method can be slow since
// it may search linearly (and vertically) through the table.
inline int CValueTable::FindRowIndexByValue(CStrID AttrID, const Data::CData& Value) const
{
	std::vector<int> RowIndices = InternalFindRowIndicesByAttr(AttrID, Value, true);
	return (RowIndices.size() == 1) ? RowIndices[0] : -1; //???or if > 0?
}
//---------------------------------------------------------------------

// Finds single row index by multiple matching attributes. This method can be slow since
// it may search linearly (and vertically) through the table.
//inline int CValueTable::FindRowIndexByAttrs(const nArray<CAttr>& Attrs) const
//{
//	nArray<int> RowIndices = InternalFindRowIndicesByAttrs(Attrs, true);
//	return (RowIndices.Size() == 1) ? RowIndices[0] : -1; //???or if > 0?
//}
////---------------------------------------------------------------------

inline void CValueTable::SetRowToDefaultValues(size_t RowIdx)
{
	for (size_t ColIdx = 0; ColIdx < GetColumnCount(); ++ColIdx)
		SetValue(ColIdx, RowIdx, Data::CData(GetColumnValueType(ColIdx)));
}
//---------------------------------------------------------------------

inline void CValueTable::SetColumnToDefaultValues(size_t ColIdx)
{
	Data::CData DefVal(GetColumnValueType(ColIdx));
	for (size_t RowIdx = 0; RowIdx < GetRowCount(); ++RowIdx)
		SetValue(ColIdx, RowIdx, DefVal);
}
//---------------------------------------------------------------------
