#include <ShaderDB.h>
#include <ConsoleApp.h>
#include <ValueTable.h>
#include <Data/Params.h>
#include <sqlite3.h>

sqlite3*		SQLiteHandle = NULL;
sqlite3_stmt*	SQLFindShader = NULL;

bool BindQueryParams(sqlite3_stmt* SQLiteStmt, const Data::CParams& Params)
{
	if (!SQLiteStmt || sqlite3_reset(SQLiteStmt) != SQLITE_OK) FAIL;

	CString ParamName(NULL, 0, 64);
	for (int i = 0; i < Params.GetCount(); ++i)
	{
		const Data::CParam& Prm = Params.Get(i);

		ParamName = ":";
		ParamName += Prm.GetName().CStr();
		int Idx = sqlite3_bind_parameter_index(SQLiteStmt, ParamName.CStr());
		if (Idx < 1) continue;

		const Data::CData& Val = Prm.GetRawValue();
		int Error = SQLITE_OK;
		if (Val.IsVoid()) ; //equal to "Error = sqlite3_bind_null(SQLiteStmt, Idx);"
		else if (Val.IsA<int>()) Error = sqlite3_bind_int(SQLiteStmt, Idx, Val);
		else if (Val.IsA<float>()) Error = sqlite3_bind_double(SQLiteStmt, Idx, (double)Val.GetValue<float>());
		else if (Val.IsA<bool>()) Error = sqlite3_bind_int(SQLiteStmt, Idx, (bool)Val ? 1 : 0);
		else if (Val.IsA<CString>())
		{
			// NOTE: the string should be in UTF-8 format.
			Error = sqlite3_bind_text(SQLiteStmt, Idx, Val.GetValuePtr<CString>()->CStr(),
				-1, SQLITE_TRANSIENT);
		}
		else if (Val.IsA<CStrID>())
		{
			// NOTE: the string should be in UTF-8 format.
			Error = sqlite3_bind_text(SQLiteStmt, Idx, Val.GetValue<CStrID>().CStr(),
				-1, SQLITE_TRANSIENT);
		}
		else if (Val.IsA<Data::CBuffer>())
		{
			const Data::CBuffer* Blob = Val.GetValuePtr<Data::CBuffer>();
			if (Blob->IsValid())
				Error = sqlite3_bind_blob(SQLiteStmt, Idx, Blob->GetPtr(), Blob->GetSize(), SQLITE_TRANSIENT);
		}
		else
		{
			n_msg(VL_ERROR, "BindQueryParams() > invalid parameter type!");
			FAIL;
		}

		if (Error != SQLITE_OK) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool ExecuteStatement(sqlite3_stmt* SQLiteStmt, DB::CValueTable* pOutTable = NULL, const Data::CParams* pParams = NULL)
{
	if (!SQLiteStmt) FAIL;

	if (pParams && !BindQueryParams(SQLiteStmt, *pParams)) FAIL;

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

bool ExecuteSQLQuery(const char* pSQL, DB::CValueTable* pOutTable = NULL, const Data::CParams* pParams = NULL)
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
		
			Result = ExecuteStatement(SQLiteStmt, pOutTable, pParams);
			
			pOutTable->TrackModifications(true);
		}
		else Result = ExecuteStatement(SQLiteStmt, NULL, pParams);

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
		CloseDB();
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
		CloseDB();
		FAIL;
	}

	// Query tables and create them if DB is empty
	DB::CValueTable Tables;
	if (!ExecuteSQLQuery("SELECT name FROM sqlite_master WHERE type='table'", &Tables))
	{
		CloseDB();
		FAIL;
	}

	if (!Tables.GetRowCount())
	{
		const char* pCreateDBSQL = "\
CREATE TABLE 'Files' (\
	'ID' INTEGER,\
	'Path' VARCHAR(1024) NOT NULL,\
	'Size' INTEGER,\
	'CRC' INTEGER,\
	PRIMARY KEY (ID ASC) ON CONFLICT REPLACE);\
\
CREATE TABLE 'Shaders' (\
	'ID' INTEGER,\
	'ShaderType' INTEGER,\
	'Target' INTEGER,\
	'EntryPoint' VARCHAR(64),\
	'CompilerFlags' INTEGER,\
	'SrcPath' VARCHAR(1024) NOT NULL,\
	'SrcModifyTimestamp' INTEGER,\
	'SrcSize' INTEGER,\
	'SrcCRC' INTEGER,\
	'ObjFileID' INTEGER,\
	'InputSigFileID' INTEGER,\
	PRIMARY KEY (ID ASC) ON CONFLICT REPLACE,\
	FOREIGN KEY (ObjFileID) REFERENCES Files(ID));\
\
CREATE TABLE 'Macros' (\
	'ShaderID' INTEGER,\
	'Name' VARCHAR(64),\
	'Value' VARCHAR(1024),\
	PRIMARY KEY (ShaderID, Name) ON CONFLICT REPLACE,\
	FOREIGN KEY (ShaderID) REFERENCES Shaders(ID));\
\
CREATE INDEX Files_MainIndex ON Files (Size, CRC);\
\
CREATE INDEX Shaders_MainIndex ON Shaders (SrcPath, ShaderType, Target, EntryPoint)";
		if (!ExecuteSQLQuery(pCreateDBSQL))
		{
			CloseDB();
			FAIL;
		}
	}

	pSQL = "SELECT * FROM Shaders WHERE SrcPath=:Path AND ShaderType=:Type AND Target=:Target AND EntryPoint=:Entry";
	if (sqlite3_prepare_v2(SQLiteHandle, pSQL, -1, &SQLFindShader, &pSQL) != SQLITE_OK)
	{
		CloseDB();
		FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

void CloseDB()
{
	if (SQLFindShader)
	{
		sqlite3_finalize(SQLFindShader);
		SQLFindShader = NULL;
	}

	if (SQLiteHandle)
	{
		if (sqlite3_close(SQLiteHandle) != SQLITE_OK)
		{
			n_msg(VL_ERROR, "SQLite error: %s\n", sqlite3_errmsg(SQLiteHandle));
		}
		SQLiteHandle = NULL;
	}
}
//---------------------------------------------------------------------

bool FindShaderRec(CShaderDBRec& InOut)
{
	Data::CParams Params(4);
	Params.Set(CStrID("Path"), InOut.SrcFile.Path);
	Params.Set(CStrID("Type"), (int)InOut.ShaderType);
	Params.Set(CStrID("Target"), (int)InOut.Target);
	Params.Set(CStrID("Entry"), InOut.EntryPoint);

	DB::CValueTable Shaders;
	if (!ExecuteStatement(SQLFindShader, &Shaders, &Params)) FAIL;

	int Col_ID = Shaders.GetColumnIndex(CStrID("ID"));
	for (int i = 0; i < Shaders.GetRowCount(); ++i)
	{
		//request and compare defines
		n_msg(VL_DEBUG, "ID = %d\n", Shaders.Get<int>(Col_ID, 0));
	}

	InOut.ID = 0;
	InOut.CompilerFlags = 0;
	InOut.ObjFile.Path = CString::Empty;

	FAIL;
}
//---------------------------------------------------------------------
