#include "Command.h"

#include "DBServer.h"
#include "Database.h"
#include <sqlite3.h>

namespace DB
{
__ImplementClass(DB::CCommand, 'DBCD', Core::CRefCounted);

CCommand::CCommand(): SQLiteStmt(NULL)
{
}
//---------------------------------------------------------------------

CCommand::~CCommand()
{
	Clear();
}
//---------------------------------------------------------------------

void CCommand::Clear()
{
	if (SQLiteStmt)
	{
		sqlite3_finalize(SQLiteStmt);
		SQLiteStmt = NULL;
	}
	VT = NULL;
	ResultIdxMap.Clear();
}
//---------------------------------------------------------------------

// This compiles an SQL statement against the provided DB. The SQL
// statement may contain placeholders which should be filled with 
// values using the various BindValue() methods. After values have been
// bound, the statement can be executed using the Execute() method.
// Remember that the SQL statement string must be UTF-8 encoded!
bool CCommand::Compile(const PDatabase& DB)
{
	n_assert(DB.isvalid());
	n_assert(!SQLiteStmt);

	//!!!can clear SQLCmd & use sqlite3_sql to retrieve SQL code!

	const char* pTail = NULL;
	if (sqlite3_prepare_v2(DB->GetSQLiteHandle(), SQLCmd.Get(), -1, &SQLiteStmt, &pTail) != SQLITE_OK)
	{
		SetError(sqlite3_errmsg(DB->GetSQLiteHandle()));
		FAIL;
	}
	n_assert(SQLiteStmt);

	// check if more then one SQL statement was in the string, we don't support that
	n_assert(pTail);
	if (pTail[0])
	{
		n_error("CCommand::Compile(): Only one SQL statement allowed (SQL: %s)\n", SQLCmd.Get());
		Clear();
		FAIL;
	}

	// If VT changed, create an index map to map value table indices to sqlite Result indices
	if (VT.isvalid() && ResultIdxMap.Size() == 0)
	{
		int ColCount = sqlite3_column_count(SQLiteStmt);
		if (ColCount > 0)
		{
			if (VT->GetNumColumns() == 0)
			{
				// SELECT * -> add all table columns, ignore unknown
				VT->BeginAddColumns();
				for (int ResultColIdx = 0; ResultColIdx < ColCount; ResultColIdx++)
				{
					CAttrID ID = DBSrv->FindAttrID(sqlite3_column_name(SQLiteStmt, ResultColIdx));
					if (ID)
					{
						ResultIdxMap.Append(VT->GetNumColumns());
						VT->AddColumn(ID, false);
					}
					else ResultIdxMap.Append(-1);
				}
				VT->EndAddColumns();
			}
			else
			{
				for (int ResultColIdx = 0; ResultColIdx < ColCount; ResultColIdx++)
				{
					//???assert NULL ID & inexistent VT column?
					CAttrID ID = DBSrv->FindAttrID(sqlite3_column_name(SQLiteStmt, ResultColIdx));
					if (ID && VT->HasColumn(ID))
						ResultIdxMap.Append(VT->GetColumnIndex(ID));
					else ResultIdxMap.Append(-1);
				}
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

// Execute a compiled command and gather the result
bool CCommand::Execute(const PDatabase& DB)
{
	n_assert(DB.isvalid()); //!!!only for error msg!
	n_assert(IsValid());

	if (VT.isvalid()) VT->SetModifiedTracking(false);

	bool Done = false;
	while (!Done)
	{
		switch (sqlite3_step(SQLiteStmt))
		{
			case SQLITE_DONE:	Done = true; break;
			case SQLITE_BUSY:	n_sleep(0.0001); break;
			case SQLITE_ROW:	if (VT.isvalid()) ReadRow(); break;
			case SQLITE_ERROR:	SetError(sqlite3_errmsg(DB->GetSQLiteHandle())); FAIL;
			case SQLITE_MISUSE:
			{
				n_error("CCommand::Execute(): sqlite3_step() returned SQLITE_MISUSE!");
				FAIL;
			}
			default:
			{
				n_error("CCommand::Execute(): unknown error code returned from sqlite3_step()");
				FAIL;
			}
		}
	}

	// Reset the command, this clears the bound values
	n_assert(sqlite3_reset(SQLiteStmt) == SQLITE_OK);

	if (VT.isvalid()) VT->SetModifiedTracking(true);

	OK;
}
//---------------------------------------------------------------------

// Gather row of Result values from SQLite and add them to the Result CValueTable.
// Note that modification tracking is turned off in the value table, because reading
// from the DB doesn't count as modification.
void CCommand::ReadRow()
{
	n_assert(SQLiteStmt);
	n_assert(VT.isvalid());

	int RowIdx = VT->AddRow();
	int ResultColIdx;
	const int ColCount = sqlite3_data_count(SQLiteStmt);
	for (ResultColIdx = 0; ResultColIdx < ColCount; ResultColIdx++)
	{
		int ResultColType = sqlite3_column_type(SQLiteStmt, ResultColIdx);
		if (SQLITE_NULL == ResultColType) continue;

		int ColIdx = ResultIdxMap[ResultColIdx];
		if (ColIdx < 0) continue;

		const CType* Type = VT->GetColumnValueType(ColIdx);
		if (Type == TInt)
		{
			n_assert(SQLITE_INTEGER == ResultColType);
			int Val = sqlite3_column_int(SQLiteStmt, ResultColIdx);
			VT->Set<int>(ColIdx, RowIdx, Val);
		}
		else if (Type == TFloat)
		{
			n_assert(SQLITE_FLOAT == ResultColType);
			float Val = (float)sqlite3_column_double(SQLiteStmt, ResultColIdx);                        
			VT->Set<float>(ColIdx, RowIdx, Val);
		}
		else if (Type == TBool)
		{
			n_assert(SQLITE_INTEGER == ResultColType);
			int Val = sqlite3_column_int(SQLiteStmt, ResultColIdx);
			VT->Set<bool>(ColIdx, RowIdx, (Val == 1));
		}
		else if (Type == TString)
		{
			n_assert(SQLITE_TEXT == ResultColType);
			nString Val = (LPCSTR)sqlite3_column_text(SQLiteStmt, ResultColIdx);
			VT->Set<nString>(ColIdx, RowIdx, Val);
		}
		else if (Type == TStrID)
		{
			n_assert(SQLITE_TEXT == ResultColType);
			CStrID Val((LPCSTR)sqlite3_column_text(SQLiteStmt, ResultColIdx));
			VT->Set<CStrID>(ColIdx, RowIdx, Val);
		}
		else if (Type == TVector4)
		{
			n_assert(SQLITE_BLOB == ResultColType);
			const void* ptr = sqlite3_column_blob(SQLiteStmt, ResultColIdx);
			uint size = sqlite3_column_bytes(SQLiteStmt, ResultColIdx);                        
			const float* fptr = (const float*)ptr;

			vector4 value;
			if (size < sizeof(vector4))
			{
				n_assert(size == 12); // vector3
				value.set(fptr[0], fptr[1], fptr[2], 0);
			}
			else
			{
				n_assert(size == sizeof(vector4));
				value.set(fptr[0], fptr[1], fptr[2], fptr[3]);
			}

			VT->Set<vector4>(ColIdx, RowIdx, value);
		}
		else if (Type == TMatrix44)
		{
			n_assert(SQLITE_BLOB == ResultColType);
			n_assert(sqlite3_column_bytes(SQLiteStmt, ResultColIdx) == sizeof(matrix44));
			matrix44 mtx(*(const matrix44*)sqlite3_column_blob(SQLiteStmt, ResultColIdx));
			VT->Set<matrix44>(ColIdx, RowIdx, mtx);                                            
		}                   
		else if (Type == TBuffer)
		{
			n_assert(SQLITE_BLOB == ResultColType);
			const void* ptr = sqlite3_column_blob(SQLiteStmt, ResultColIdx);
			int size = sqlite3_column_bytes(SQLiteStmt, ResultColIdx);
			CBuffer Blob(ptr, size);
			VT->Set<CBuffer>(ColIdx, RowIdx, Blob);
		}
		else if (!Type)
		{
			// Variable type column, it supports int, float & string. Bool is represented as int.
			// If ScriptObject wants to save bool to DB & then restore it, this object should
			// implement OnLoad scripted method or smth and convert int values to bool inside.
			switch (ResultColType)
			{
				case SQLITE_INTEGER:
					VT->Set<int>(ColIdx, RowIdx, sqlite3_column_int(SQLiteStmt, ResultColIdx));
					break;
				case SQLITE_FLOAT:
					VT->Set<float>(ColIdx, RowIdx, (float)sqlite3_column_double(SQLiteStmt, ResultColIdx));
					break;
				case SQLITE_TEXT:
					VT->Set<nString>(ColIdx, RowIdx, nString((LPCSTR)sqlite3_column_text(SQLiteStmt, ResultColIdx)));
					break;
				default: n_error("CCommand::ReadResultRow(): invalid attribute type!");
			}
		}
		else n_error("CCommand::ReadResultRow(): invalid attribute type!");
	}
}
//---------------------------------------------------------------------

// Returns the Idx of a placeholder value by Name. The placeholder Name
// must be UTF-8 encoded! NOTE: this method is slow because a temporary string
// must be created! It's much faster to work with unnamed wildcards ("?") and
// set them directly by Idx!
int CCommand::BindingIndexOf(const nString& Name) const
{
#ifdef _DEBUG
	n_message("Performance warning - CCommand::BindingIndexOf(const nString& Name)");
#endif

	n_assert(SQLiteStmt);

	nString wildcard = ":";
	wildcard.Append(Name);

	int Idx = sqlite3_bind_parameter_index(SQLiteStmt, wildcard.Get());
	n_assert2(Idx > 0, "Invalid wildcard Name.");

	// Sqlite's indices are 1-based, convert to 0-based
	return Idx - 1; 
}
//---------------------------------------------------------------------

// NOTE: this method is slow because a temporary string must be created!
// It's much faster to work with unnamed wildcards ("?") and set them directly by Idx!
int CCommand::BindingIndexOf(CAttrID AttrID) const
{
#ifdef _DEBUG
	n_message("Performance warning - CCommand::BindingIndexOf(CAttrID AttrID)");
#endif

	n_assert(SQLiteStmt);

	nString Wildcard(":");
	Wildcard.Append(AttrID->GetName());
	int Idx = sqlite3_bind_parameter_index(SQLiteStmt, Wildcard.Get());
	n_assert2(Idx > 0, "Invalid wildcard Name.");

	// Sqlite's indices are 1-based, convert to 0-based
	return Idx - 1; 
}
//---------------------------------------------------------------------

void CCommand::BindValue(int Idx, const CData& Val)
{
	n_assert(SQLiteStmt);
	int Error = SQLITE_OK;
	if (Val.IsVoid()) ; //equal to "Error = sqlite3_bind_null(SQLiteStmt, Idx + 1);"
	else if (Val.IsA<int>()) Error = sqlite3_bind_int(SQLiteStmt, Idx + 1, Val);
	else if (Val.IsA<float>()) Error = sqlite3_bind_double(SQLiteStmt, Idx + 1, (double)Val.GetValue<float>());
	else if (Val.IsA<bool>()) Error = sqlite3_bind_int(SQLiteStmt, Idx + 1, (bool)Val ? 1 : 0);
	else if (Val.IsA<nString>())
	{
		// NOTE: the string should be in UTF-8 format.
		Error = sqlite3_bind_text(SQLiteStmt, Idx + 1, Val.GetValuePtr<nString>()->Get(),
			-1, SQLITE_TRANSIENT);
	}
	else if (Val.IsA<CStrID>())
	{
		// NOTE: the string should be in UTF-8 format.
		Error = sqlite3_bind_text(SQLiteStmt, Idx + 1, Val.GetValue<CStrID>().CStr(),
			-1, SQLITE_TRANSIENT);
	}
	else if (Val.IsA<vector4>())
	{
		// NOTE: float4's will be saved as blobs in the DB, since the 
		// float4 may go away at any time, let SQLite make its own copy of the data
		Error = sqlite3_bind_blob(SQLiteStmt, Idx + 1, Val.GetValuePtr<vector4>(),
			sizeof(vector4), SQLITE_TRANSIENT);
	}
	else if (Val.IsA<matrix44>())
	{
		// NOTE: matrix44's will be saved as blobs in the DB, since the 
		// matrix44 may go away at any time, let SQLite make its own copy of the data
		Error = sqlite3_bind_blob(SQLiteStmt, Idx + 1, Val.GetValuePtr<matrix44>(),
			sizeof(matrix44), SQLITE_TRANSIENT);
	}                   
	else if (Val.IsA<CBuffer>())
	{
		const CBuffer* Blob = Val.GetValuePtr<CBuffer>();
		if (!Blob->IsValid()) return;
		Error = sqlite3_bind_blob(SQLiteStmt, Idx + 1, Blob->GetPtr(), Blob->GetSize(), SQLITE_TRANSIENT);
	}
	else n_error("CCommand::ReadResultRow(): invalid attribute type!");
	n_assert(SQLITE_OK == Error);
}
//---------------------------------------------------------------------

}