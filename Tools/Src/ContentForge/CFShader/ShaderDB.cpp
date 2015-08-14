#include <ConsoleApp.h>
#include <ValueTable.h>
#include <Util/UtilFwd.h> // CRC
#include <sqlite3.h>

sqlite3* SQLiteHandle = NULL;

//!!!store concrete statements, provide interface like "ShaderExists" or "GetShaderPath" or smth!
//or use struct with fields same as DB attributes!

bool ExecuteStatement(sqlite3_stmt* SQLiteStmt, DB::CValueTable* pOutTable = NULL)
{
	int Result;
	do
	{
		Result = sqlite3_step(SQLiteStmt);
		switch (Result)
		{
			case SQLITE_BUSY:	Sys::Sleep(1); break;
			case SQLITE_DONE:	break;
			case SQLITE_ROW:
			{
				if (!pOutTable) break;

				int RowIdx = pOutTable->AddRow();
				int ResultColIdx;
				const int ColCount = sqlite3_data_count(SQLiteStmt);
				for (ResultColIdx = 0; ResultColIdx < ColCount; ++ResultColIdx)
				{
					int ResultColType = sqlite3_column_type(SQLiteStmt, ResultColIdx);
					if (SQLITE_NULL == ResultColType) continue;

					int ColIdx = pOutTable->GetColumnIndex(CStrID(sqlite3_column_name(SQLiteStmt, ResultColIdx)));
					if (ColIdx < 0) continue;

					const Data::CType* Type = pOutTable->GetColumnValueType(ColIdx);
					if (Type == TInt)
					{
						n_assert(SQLITE_INTEGER == ResultColType);
						int Val = sqlite3_column_int(SQLiteStmt, ResultColIdx);
						pOutTable->Set<int>(ColIdx, RowIdx, Val);
					}
					else if (Type == TFloat)
					{
						n_assert(SQLITE_FLOAT == ResultColType);
						float Val = (float)sqlite3_column_double(SQLiteStmt, ResultColIdx);                        
						pOutTable->Set<float>(ColIdx, RowIdx, Val);
					}
					else if (Type == TBool)
					{
						n_assert(SQLITE_INTEGER == ResultColType);
						int Val = sqlite3_column_int(SQLiteStmt, ResultColIdx);
						pOutTable->Set<bool>(ColIdx, RowIdx, (Val == 1));
					}
					else if (Type == TString)
					{
						n_assert(SQLITE_TEXT == ResultColType);
						CString Val((const char*)sqlite3_column_text(SQLiteStmt, ResultColIdx));
						pOutTable->Set<CString>(ColIdx, RowIdx, Val);
					}
					else if (Type == TStrID)
					{
						n_assert(SQLITE_TEXT == ResultColType);
						CStrID Val((LPCSTR)sqlite3_column_text(SQLiteStmt, ResultColIdx));
						pOutTable->Set<CStrID>(ColIdx, RowIdx, Val);
					}
					else if (Type == TBuffer)
					{
						n_assert(SQLITE_BLOB == ResultColType);
						const void* ptr = sqlite3_column_blob(SQLiteStmt, ResultColIdx);
						int size = sqlite3_column_bytes(SQLiteStmt, ResultColIdx);
						Data::CBuffer Blob(ptr, size);
						pOutTable->Set<Data::CBuffer>(ColIdx, RowIdx, Blob);
					}
					else if (!Type)
					{
						// Variable type column, it supports int, float & string. Bool is represented as int.
						// If ScriptObject wants to save bool to DB & then restore it, this object should
						// implement OnLoad scripted method or smth and convert int values to bool inside.
						switch (ResultColType)
						{
							case SQLITE_INTEGER:
								pOutTable->Set<int>(ColIdx, RowIdx, sqlite3_column_int(SQLiteStmt, ResultColIdx));
								break;
							case SQLITE_FLOAT:
								pOutTable->Set<float>(ColIdx, RowIdx, (float)sqlite3_column_double(SQLiteStmt, ResultColIdx));
								break;
							case SQLITE_TEXT:
								pOutTable->Set<CString>(ColIdx, RowIdx, CString((LPCSTR)sqlite3_column_text(SQLiteStmt, ResultColIdx)));
								break;
							default: Sys::Error("ExecuteStatement() > unsupported variable-type DB column type!");
						}
					}
					else Sys::Error("ExecuteStatement() > unsupported type!");
				}

				break;
			}
			case SQLITE_ERROR:
			{
				n_msg(VL_ERROR, "SQLite error during sqlite3_step(): %s\n", sqlite3_errmsg(SQLiteHandle));
				FAIL;
			}
			default:
			{
				n_msg(VL_ERROR, "sqlite3_step() returned error code %d\n", Result);
				FAIL;
			}
		}
	}
	while (Result != SQLITE_DONE);

	OK;
}
//---------------------------------------------------------------------

bool ExecuteSQLQuery(const char* pSQL, DB::CValueTable* pOutTable = NULL)
{
	if (!pSQL) OK;

	bool Result = true;
	do
	{
		sqlite3_stmt* SQLiteStmt = NULL;
		if (sqlite3_prepare_v2(SQLiteHandle, pSQL, -1, &SQLiteStmt, &pSQL) != SQLITE_OK)
		{
			n_msg(VL_ERROR, "SQLite error: %s\n", sqlite3_errmsg(SQLiteHandle));
			FAIL;
		}

		while (*pSQL && strchr(DEM_WHITESPACE, *pSQL)) ++pSQL;

		//bind params

		if (!*pSQL && pOutTable) // The last query, fill table
		{
			pOutTable->TrackModifications(false);
			
			int ColCount = sqlite3_column_count(SQLiteStmt);
			if (ColCount > 0)
			{
				if (!pOutTable->GetColumnCount())
				{
					// SELECT * -> add all table columns, ignore unknown
					pOutTable->BeginAddColumns();
					for (int ResultColIdx = 0; ResultColIdx < ColCount; ++ResultColIdx)
						pOutTable->AddColumn(CStrID(sqlite3_column_name(SQLiteStmt, ResultColIdx)), NULL, false);
					pOutTable->EndAddColumns();
				}
			}
		
			Result = ExecuteStatement(SQLiteStmt, pOutTable);
			
			pOutTable->TrackModifications(true);
		}
		else Result = ExecuteStatement(SQLiteStmt);

		sqlite3_finalize(SQLiteStmt);
	}
	while (Result && *pSQL);

	return Result;
}
//---------------------------------------------------------------------

// NB: pURI must be encoded in UTF-8
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

	//???synchronous mode?
	const char* pSQL = "\
PRAGMA journal_mode=MEMORY;\
PRAGMA locking_mode=EXCLUSIVE;\
PRAGMA cache_size=2048;\
PRAGMA synchronous=ON;\
PRAGMA temp_store=MEMORY";
	if (!ExecuteSQLQuery(pSQL))
	{
		sqlite3_close(SQLiteHandle);
		SQLiteHandle = NULL;
		FAIL;
	}

	// Query tables and create them if DB is empty
	DB::CValueTable Tables;
	if (!ExecuteSQLQuery("SELECT name FROM sqlite_master WHERE type='table'", &Tables))
	{
		sqlite3_close(SQLiteHandle);
		SQLiteHandle = NULL;
		FAIL;
	}

	if (!Tables.GetRowCount())
	{
		const char* pCreateDBSQL = "\
CREATE TABLE 'Shaders' (\
	'ID' INTEGER,\
	'ShaderType' INTEGER,\
	'Target' INTEGER,\
	PRIMARY KEY (ID) ON CONFLICT REPLACE);\
\
CREATE TABLE 'Macros' (\
	'ShaderID' INTEGER,\
	'Name' TEXT,\
	'Value' TEXT,\
	PRIMARY KEY (ShaderID, Name) ON CONFLICT REPLACE,\
	FOREIGN KEY (ShaderID) REFERENCES Shaders(ID));\
\
CREATE INDEX Shaders_MainIndex ON Shaders (ShaderType, Target)";
		if (!ExecuteSQLQuery(pCreateDBSQL))
		{
			sqlite3_close(SQLiteHandle);
			SQLiteHandle = NULL;
			FAIL;
		}
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