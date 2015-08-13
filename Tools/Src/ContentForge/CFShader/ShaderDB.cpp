#include <ConsoleApp.h>
#include <sqlite3.h>

sqlite3* SQLiteHandle = NULL;

bool OpenDB(const char* pURI)
{
	n_assert(!SQLiteHandle);

	int Result = sqlite3_open_v2(pURI, &SQLiteHandle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL); //"Nebula2");
	if (Result != SQLITE_OK)
	{
		n_msg(VL_ERROR, "SQLite error: %s\n", sqlite3_errmsg(SQLiteHandle));
		SQLiteHandle = NULL;
		FAIL;
	}

	if (sqlite3_busy_timeout(SQLiteHandle, 100) != SQLITE_OK ||
		sqlite3_extended_result_codes(SQLiteHandle, 1) != SQLITE_OK)
	{
		n_msg(VL_ERROR, "SQLite error: %s\n", sqlite3_errmsg(SQLiteHandle));
		sqlite3_close(SQLiteHandle);
		SQLiteHandle = NULL;
		FAIL;
	}

	//???sync mode?
	const char* pSQL =
		"PRAGMA journal_mode=MEMORY;\
		PRAGMA locking_mode=EXCLUSIVE;\
		PRAGMA cache_size=2048;\
		PRAGMA synchronous=ON;\
		PRAGMA temp_store=MEMORY";

	while (pSQL && *pSQL)
	{
		sqlite3_stmt* SQLiteStmt = NULL;
		if (sqlite3_prepare_v2(SQLiteHandle, pSQL, -1, &SQLiteStmt, &pSQL) != SQLITE_OK)
		{
			n_msg(VL_ERROR, "SQLite error: %s\n", sqlite3_errmsg(SQLiteHandle));
			FAIL;
		}

		bool Done = false;
		while (!Done)
		{
			Result = sqlite3_step(SQLiteStmt);
			switch (Result)
			{
				case SQLITE_DONE:	Done = true; break;
				case SQLITE_BUSY:	Sys::Sleep(1); break;
				case SQLITE_ROW:	break;
				case SQLITE_ERROR:
				{
					n_msg(VL_ERROR, "SQLite error: %s\n", sqlite3_errmsg(SQLiteHandle));
					sqlite3_close(SQLiteHandle);
					SQLiteHandle = NULL;
					FAIL;
				}
				case SQLITE_MISUSE:
				{
					n_msg(VL_ERROR, "CCommand::Execute(): sqlite3_step() returned SQLITE_MISUSE!\n");
					sqlite3_close(SQLiteHandle);
					SQLiteHandle = NULL;
					FAIL;
				}
				default:
				{
					n_msg(VL_ERROR, "CCommand::Execute(): unknown error code %d returned from sqlite3_step()\n", Result);
					sqlite3_close(SQLiteHandle);
					SQLiteHandle = NULL;
					FAIL;
				}
			}
		}

		sqlite3_finalize(SQLiteStmt);
	}

	OK;
}
//---------------------------------------------------------------------

void CloseDB()
{
	if (!SQLiteHandle) return;
	if (sqlite3_close(SQLiteHandle) != SQLITE_OK)
	{
		n_msg(VL_ERROR, "SQLite error: %s\n", sqlite3_errmsg(SQLiteHandle));
	}
	SQLiteHandle = NULL;
}
//---------------------------------------------------------------------

/*
int Idx = sqlite3_bind_parameter_index(SQLiteStmt, ":param1");

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


// Reset the command, this clears the bound values
n_assert(sqlite3_reset(SQLiteStmt) == SQLITE_OK);

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
*/