#include "ShaderDB.h"

#include <ValueTable.h>
#include <Utils.h>
#include <sqlite3.h>
#include <thread>
#include <chrono>
#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

namespace DB
{

sqlite3*		SQLiteHandle = nullptr;
sqlite3_stmt*	SQLFindShader = nullptr;
sqlite3_stmt*	SQLGetObjFile = nullptr;
sqlite3_stmt*	SQLFindObjFileUsage = nullptr;
sqlite3_stmt*	SQLFindObjFile = nullptr;
sqlite3_stmt*	SQLFindObjFileByID = nullptr;
sqlite3_stmt*	SQLFindFreeObjFileRec = nullptr;
sqlite3_stmt*	SQLInsertNewObjFileRec = nullptr;
sqlite3_stmt*	SQLUpdateObjFileRec = nullptr;
sqlite3_stmt*	SQLReleaseObjFileRec = nullptr;
sqlite3_stmt*	SQLInsertNewShaderRec = nullptr;
sqlite3_stmt*	SQLUpdateShaderRec = nullptr;
sqlite3_stmt*	SQLClearDefines = nullptr;
sqlite3_stmt*	SQLInsertDefine = nullptr;
sqlite3_stmt*	SQLGetDefines = nullptr;

bool InitSQL(sqlite3_stmt** ppStmt, const char* pSQL)
{
	if (sqlite3_prepare_v2(SQLiteHandle, pSQL, -1, ppStmt, nullptr) == SQLITE_OK)
		return true;

	CloseConnection();
	assert(false && "Error compiling SQL");
	return false;
}
//---------------------------------------------------------------------

bool BindQueryParams(sqlite3_stmt* SQLiteStmt, const Data::CParams& Params)
{
	if (!SQLiteStmt || sqlite3_reset(SQLiteStmt) != SQLITE_OK) return false;

	std::string ParamName;
	ParamName.reserve(64);
	for (const auto& Pair : Params)
	{
		ParamName = ":";
		ParamName += Pair.first.CStr();
		int Idx = sqlite3_bind_parameter_index(SQLiteStmt, ParamName.c_str());
		if (Idx < 1) continue;

		const Data::CData& Val = Pair.second;
		int Error = SQLITE_OK;
		if (Val.IsVoid()); //equal to "Error = sqlite3_bind_null(SQLiteStmt, Idx);"
		else if (Val.IsA<int>()) Error = sqlite3_bind_int(SQLiteStmt, Idx, Val);
		else if (Val.IsA<float>()) Error = sqlite3_bind_double(SQLiteStmt, Idx, (double)Val.GetValue<float>());
		else if (Val.IsA<bool>()) Error = sqlite3_bind_int(SQLiteStmt, Idx, (bool)Val ? 1 : 0);
		else if (Val.IsA<std::string>())
		{
			// NOTE: the string should be in UTF-8 format.
			Error = sqlite3_bind_text(SQLiteStmt, Idx, Val.GetValuePtr<std::string>()->c_str(),
				-1, SQLITE_TRANSIENT);
		}
		else if (Val.IsA<CStrID>())
		{
			// NOTE: the string should be in UTF-8 format.
			Error = sqlite3_bind_text(SQLiteStmt, Idx, Val.GetValue<CStrID>().CStr(),
				-1, SQLITE_TRANSIENT);
		}
		else if (Val.IsA<CBuffer>())
		{
			const CBuffer* Blob = Val.GetValuePtr<CBuffer>();
			if (Blob && !Blob->empty())
				Error = sqlite3_bind_blob(SQLiteStmt, Idx, Blob->data(), Blob->size(), SQLITE_TRANSIENT);
		}
		else
		{
			//Messages += "BindQueryParams() > invalid parameter type!\n";
			return false;
		}

		if (Error != SQLITE_OK) return false;
	}

	return true;
}
//---------------------------------------------------------------------

bool ExecuteStatement(sqlite3_stmt* SQLiteStmt, CValueTable* pOutTable = nullptr, const Data::CParams* pParams = nullptr)
{
	if (!SQLiteStmt) return false;

	if (pParams && !BindQueryParams(SQLiteStmt, *pParams)) return false;

	if (pOutTable)
	{
		pOutTable->TrackModifications(false);

		int ColCount = sqlite3_column_count(SQLiteStmt);
		if (ColCount > 0)
		{
			pOutTable->BeginAddColumns();
			for (int ResultColIdx = 0; ResultColIdx < ColCount; ++ResultColIdx)
			{
				CStrID ColID = CStrID(sqlite3_column_name(SQLiteStmt, ResultColIdx));
				if (!pOutTable->HasColumn(ColID))
					pOutTable->AddColumn(ColID, nullptr, false);
			}
			pOutTable->EndAddColumns();
		}
	}

	int Result;
	do
	{
		Result = sqlite3_step(SQLiteStmt);
		switch (Result)
		{
			case SQLITE_BUSY:	std::this_thread::sleep_for(std::chrono::seconds(1)); break;
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
					if (Type == DATA_TYPE(int))
					{
						assert(SQLITE_INTEGER == ResultColType);
						int Val = sqlite3_column_int(SQLiteStmt, ResultColIdx);
						pOutTable->Set<int>(ColIdx, RowIdx, Val);
					}
					else if (Type == DATA_TYPE(float))
					{
						assert(SQLITE_FLOAT == ResultColType);
						float Val = (float)sqlite3_column_double(SQLiteStmt, ResultColIdx);
						pOutTable->Set<float>(ColIdx, RowIdx, Val);
					}
					else if (Type == DATA_TYPE(bool))
					{
						assert(SQLITE_INTEGER == ResultColType);
						int Val = sqlite3_column_int(SQLiteStmt, ResultColIdx);
						pOutTable->Set<bool>(ColIdx, RowIdx, (Val == 1));
					}
					else if (Type == DATA_TYPE(std::string))
					{
						assert(SQLITE_TEXT == ResultColType);
						std::string Val((const char*)sqlite3_column_text(SQLiteStmt, ResultColIdx));
						pOutTable->Set<std::string>(ColIdx, RowIdx, Val);
					}
					else if (Type == DATA_TYPE(CStrID))
					{
						assert(SQLITE_TEXT == ResultColType);
						CStrID Val((const char*)sqlite3_column_text(SQLiteStmt, ResultColIdx));
						pOutTable->Set<CStrID>(ColIdx, RowIdx, Val);
					}
					else if (Type == DATA_TYPE(CBuffer))
					{
						assert(SQLITE_BLOB == ResultColType);
						auto ptr = static_cast<const uint8_t*>(sqlite3_column_blob(SQLiteStmt, ResultColIdx));
						int size = sqlite3_column_bytes(SQLiteStmt, ResultColIdx);
						pOutTable->Set<CBuffer>(ColIdx, RowIdx, CBuffer(ptr, ptr + size));
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
								pOutTable->Set<std::string>(ColIdx, RowIdx, std::string((const char*)sqlite3_column_text(SQLiteStmt, ResultColIdx)));
								break;
							default: assert(false && "ExecuteStatement() > unsupported variable-type DB column type!");
						}
					}
					else assert(false && "ExecuteStatement() > unsupported type!");
				}

				break;
			}
			case SQLITE_ERROR:
			{
				//Messages += "SQLite error during sqlite3_step(): ";
				//Messages += sqlite3_errmsg(SQLiteHandle);
				//Messages += '\n';
				return false;
			}
			default:
			{
				//Messages += "sqlite3_step() returned error code ";
				//Messages += std::to_string(Result);
				//Messages += ", ";
				//Messages += sqlite3_errmsg(SQLiteHandle);
				//Messages += '\n';
				return false;
			}
		}
	}
	while (Result != SQLITE_DONE);

	if (pOutTable) pOutTable->TrackModifications(true);

	return true;
}
//---------------------------------------------------------------------

bool ExecuteSQLQuery(const char* pSQL, CValueTable* pOutTable, const Data::CParams* pParams)
{
	if (!pSQL) return true;

	bool Result = true;
	do
	{
		sqlite3_stmt* SQLiteStmt = nullptr;
		if (sqlite3_prepare_v2(SQLiteHandle, pSQL, -1, &SQLiteStmt, &pSQL) != SQLITE_OK)
		{
			//Messages += "SQLite error: ";
			//Messages += sqlite3_errmsg(SQLiteHandle);
			//Messages += '\n';
			return false;
		}

		while (*pSQL && strchr(" \r\n\t", *pSQL)) ++pSQL;

		// In the last query fill table
		Result = ExecuteStatement(SQLiteStmt, (!*pSQL && pOutTable) ? pOutTable : nullptr, pParams);

		sqlite3_finalize(SQLiteStmt);
	}
	while (Result && *pSQL);

	return Result;
}
//---------------------------------------------------------------------

#define INIT_SQL(Stmt, SQL) \
	if (sqlite3_prepare_v2(SQLiteHandle, SQL, -1, &Stmt, nullptr) != SQLITE_OK) \
	{ \
		CloseConnection(); \
		assert(false && "Error compiling SQL: "##SQL); \
		return false; \
	}

// NB: pURI must be encoded in UTF-8
bool OpenConnection(const char* pURI)
{
	assert(!SQLiteHandle);

	EnsureDirectoryExists(ExtractDirName(pURI));

	int Result = sqlite3_open_v2(pURI, &SQLiteHandle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
	if (Result != SQLITE_OK)
	{
		//Messages += "SQLite error: ";
		//Messages += sqlite3_errmsg(SQLiteHandle);
		//Messages += '\n';
		SQLiteHandle = nullptr;
		return false;
	}

	if (sqlite3_busy_timeout(SQLiteHandle, 100) != SQLITE_OK ||
		sqlite3_extended_result_codes(SQLiteHandle, 1) != SQLITE_OK)
	{
		//Messages += "SQLite error: ";
		//Messages += sqlite3_errmsg(SQLiteHandle);
		//Messages += '\n';
		CloseConnection();
		return false;
	}

	// PRAGMA threads = N; - for worker threads
	// The same as sqlite3_limit, see https://www.sqlite.org/c3ref/limit.html

	const char* pSQL = "\
PRAGMA encoding = \"UTF-8\";\
PRAGMA journal_mode=MEMORY;\
PRAGMA cache_size=2048;\
PRAGMA synchronous=NORMAL;\
PRAGMA temp_store=MEMORY";
	if (!ExecuteSQLQuery(pSQL))
	{
		CloseConnection();
		return false;
	}

	// Query tables and create them if DB is empty
	CValueTable Tables;
	if (!ExecuteSQLQuery("SELECT name FROM sqlite_master WHERE type='table'", &Tables))
	{
		CloseConnection();
		return false;
	}

	if (!Tables.GetRowCount())
	{
		const char* pCreateDBSQL = "\
CREATE TABLE 'ShaderBinaries' (\
	'ID' INTEGER,\
	'Path' VARCHAR(1024) NOT NULL,\
	'Size' INTEGER,\
	'BytecodeSize' INTEGER,\
	'CRC' INTEGER,\
	PRIMARY KEY (ID ASC) ON CONFLICT REPLACE);\
\
CREATE TABLE 'InputSignatures' (\
	'ID' INTEGER,\
	'Folder' VARCHAR(1024) NOT NULL,\
	'Size' INTEGER,\
	'CRC' INTEGER,\
	PRIMARY KEY (ID ASC) ON CONFLICT REPLACE);\
\
CREATE TABLE 'Shaders' (\
	'ID' INTEGER,\
	'ShaderType' INTEGER,\
	'Target' INTEGER,\
	'EntryPoint' VARCHAR(64),\
	'CompilerVersion' INTEGER,\
	'CompilerFlags' INTEGER,\
	'SrcPath' VARCHAR(1024) NOT NULL,\
	'SrcModifyTimestamp' INTEGER,\
	'SrcSize' INTEGER,\
	'SrcCRC' INTEGER,\
	'BinaryFileID' INTEGER,\
	'InputSigFileID' INTEGER,\
	PRIMARY KEY (ID ASC) ON CONFLICT REPLACE,\
	FOREIGN KEY (BinaryFileID) REFERENCES ShaderBinaries(ID),\
	FOREIGN KEY (InputSigFileID) REFERENCES InputSignatures(ID));\
\
CREATE TABLE 'Macros' (\
	'ShaderID' INTEGER,\
	'Name' VARCHAR(64),\
	'Value' VARCHAR(1024),\
	PRIMARY KEY (ShaderID, Name) ON CONFLICT REPLACE,\
	FOREIGN KEY (ShaderID) REFERENCES Shaders(ID));\
\
CREATE INDEX ShaderBinaries_MainIndex ON ShaderBinaries (Size, CRC);\
\
CREATE INDEX Shaders_MainIndex ON Shaders (SrcPath, ShaderType, Target, EntryPoint)";
		if (!ExecuteSQLQuery(pCreateDBSQL))
		{
			CloseConnection();
			return false;
		}
	}

	if (!InitSQL(&SQLFindShader, "SELECT * FROM Shaders WHERE SrcPath=:Path AND ShaderType=:Type AND Target=:Target AND EntryPoint=:Entry"))
		return false;

	INIT_SQL(SQLGetObjFile, "SELECT * FROM ShaderBinaries WHERE ID=:ID");
	INIT_SQL(SQLFindObjFileUsage, "SELECT ID FROM Shaders WHERE BinaryFileID=:ID");
	INIT_SQL(SQLFindObjFile, "SELECT ID, Path FROM ShaderBinaries WHERE BytecodeSize=:BytecodeSize AND CRC=:CRC");
	INIT_SQL(SQLFindObjFileByID, "SELECT * FROM ShaderBinaries WHERE ID=:ID");
	INIT_SQL(SQLFindFreeObjFileRec, "SELECT ID FROM ShaderBinaries WHERE Size = 0 ORDER BY ID LIMIT 1");
	INIT_SQL(SQLInsertNewObjFileRec, "INSERT INTO ShaderBinaries (Path, Size, BytecodeSize, CRC) VALUES (\"\", 0, 0, 0)");
	INIT_SQL(SQLUpdateObjFileRec, "UPDATE ShaderBinaries SET Path=:Path, Size=:Size, BytecodeSize=:BytecodeSize, CRC=:CRC WHERE ID=:ID");
	INIT_SQL(SQLReleaseObjFileRec, "UPDATE ShaderBinaries SET Size=0 WHERE ID=:ID");
	INIT_SQL(SQLInsertNewShaderRec, "INSERT INTO Shaders (SrcPath) VALUES (\"\")");
	INIT_SQL(SQLUpdateShaderRec,
		"UPDATE Shaders SET "
		"	ShaderType=:ShaderType, "
		"	Target=:Target, "
		"	EntryPoint=:EntryPoint, "
		"	CompilerVersion=:CompilerVersion, "
		"	CompilerFlags=:CompilerFlags, "
		"	SrcPath=:SrcPath, "
		"	SrcModifyTimestamp=:SrcModifyTimestamp, "
		"	SrcSize=:SrcSize, "
		"	SrcCRC=:SrcCRC, "
		"	BinaryFileID=:BinaryFileID, "
		"	InputSigFileID=:InputSigFileID "
		"WHERE ID=:ID");
	INIT_SQL(SQLClearDefines, "DELETE FROM Macros WHERE ShaderID = :ShaderID");
	INIT_SQL(SQLInsertDefine, "INSERT INTO Macros (ShaderID, Name, Value) VALUES (:ShaderID, :Name, :Value)");
	INIT_SQL(SQLGetDefines, "SELECT Name, Value FROM Macros WHERE ShaderID = :ShaderID"); // ORDER BY Name");

	return true;
}
//---------------------------------------------------------------------

#define SAFE_RELEASE_SQL(Stmt) if (Stmt) { sqlite3_finalize(Stmt); Stmt = nullptr; }

void CloseConnection()
{
	//???can release without an SQLiteHandle?
	SAFE_RELEASE_SQL(SQLFindShader);
	SAFE_RELEASE_SQL(SQLGetObjFile);
	SAFE_RELEASE_SQL(SQLFindObjFileUsage);
	SAFE_RELEASE_SQL(SQLFindObjFile);
	SAFE_RELEASE_SQL(SQLFindObjFileByID);
	SAFE_RELEASE_SQL(SQLFindFreeObjFileRec);
	SAFE_RELEASE_SQL(SQLInsertNewObjFileRec);
	SAFE_RELEASE_SQL(SQLUpdateObjFileRec);
	SAFE_RELEASE_SQL(SQLReleaseObjFileRec);
	SAFE_RELEASE_SQL(SQLInsertNewShaderRec);
	SAFE_RELEASE_SQL(SQLUpdateShaderRec);
	SAFE_RELEASE_SQL(SQLClearDefines);
	SAFE_RELEASE_SQL(SQLInsertDefine);
	SAFE_RELEASE_SQL(SQLGetDefines);

	if (SQLiteHandle)
	{
		if (sqlite3_close(SQLiteHandle) != SQLITE_OK)
		{
			//Messages += "SQLite error: ";
			//Messages += sqlite3_errmsg(SQLiteHandle);
			//Messages += '\n';
		}
		SQLiteHandle = nullptr;
	}
}
//---------------------------------------------------------------------

bool FindShaderRecord(CShaderRecord& InOut)
{
	Data::CParams Params;
	Params.emplace(CStrID("Path"), InOut.SrcFile.Path);
	Params.emplace(CStrID("Type"), (int)InOut.ShaderType);
	Params.emplace(CStrID("Target"), (int)InOut.Target);
	Params.emplace(CStrID("Entry"), InOut.EntryPoint);

	CValueTable Shaders;
	if (!ExecuteStatement(SQLFindShader, &Shaders, &Params)) return false;

	Data::CParams DefineParams;

	bool Found = false;

	int Col_ID = Shaders.GetColumnIndex(CStrID("ID"));
	for (size_t ShIdx = 0; ShIdx < Shaders.GetRowCount(); ++ShIdx)
	{
		DefineParams.emplace(CStrID("ShaderID"), Shaders.Get<int>(Col_ID, ShIdx));

		CValueTable Defines;
		if (!ExecuteStatement(SQLGetDefines, &Defines, &DefineParams)) return false;

		size_t DBDefineCount = Defines.GetRowCount();
		size_t LocalDefineCount = InOut.Defines.size();
		if (DBDefineCount != LocalDefineCount)
		{
			Found = false;
			break;
		}

		Found = true;

		int Col_Name = Defines.GetColumnIndex(CStrID("Name"));
		int Col_Value = Defines.GetColumnIndex(CStrID("Value"));
		for (size_t DefIdx = 0; DefIdx < Defines.GetRowCount(); ++DefIdx)
		{
			const auto& DBDefineName = Defines.Get<std::string>(Col_Name, DefIdx);
			assert(!DBDefineName.empty());

			//???TODO: compare through INNER JOIN / WHERE?
			for (const auto& NewDefine : InOut.Defines)
			{
				if (NewDefine.first == DBDefineName)
				{
					Data::CData Value;
					Defines.GetValue(Col_Value, DefIdx, Value);
					const std::string DBDefineValue = Value.IsNull() ? std::string{} : Value.GetValue<std::string>();
					if (NewDefine.second != DBDefineValue)
					{
						Found = false;
					}
					break;
				}
			}

			if (!Found) break;
		}

		if (Found)
		{
			InOut.ID = Shaders.Get<int>(Col_ID, ShIdx);
			InOut.CompilerVersion = Shaders.Get<int>(CStrID("CompilerVersion"), ShIdx);
			InOut.CompilerFlags = Shaders.Get<int>(CStrID("CompilerFlags"), ShIdx);
			InOut.SrcModifyTimestamp = Shaders.Get<int>(CStrID("SrcModifyTimestamp"), ShIdx);
			InOut.SrcFile.Size = Shaders.Get<int>(CStrID("SrcSize"), ShIdx);
			InOut.SrcFile.CRC = Shaders.Get<int>(CStrID("SrcCRC"), ShIdx);
			InOut.ObjFile.ID = Shaders.Get<int>(CStrID("BinaryFileID"), ShIdx);
			InOut.InputSigFile.ID = Shaders.Get<int>(CStrID("InputSigFileID"), ShIdx);
			return true;
		}
	}

	InOut.ID = 0;
	InOut.CompilerVersion = 0;
	InOut.CompilerFlags = 0;
	InOut.ObjFile.ID = 0;
	InOut.ObjFile.Path.clear();
	InOut.InputSigFile.ID = 0;
	InOut.InputSigFile.Folder.clear();

	return false; // Not found
}
//---------------------------------------------------------------------

bool WriteShaderRecord(CShaderRecord& InOut)
{
	uint32_t ID;
	if (InOut.ID == 0)
	{
		if (!ExecuteStatement(SQLInsertNewShaderRec)) return false;
		ID = (uint32_t)sqlite3_last_insert_rowid(SQLiteHandle);
	}
	else ID = InOut.ID;

	Data::CParams Params;
	Params.emplace(CStrID("ID"), (int)ID);
	Params.emplace(CStrID("ShaderType"), (int)InOut.ShaderType);
	Params.emplace(CStrID("Target"), (int)InOut.Target);
	Params.emplace(CStrID("EntryPoint"), InOut.EntryPoint);
	Params.emplace(CStrID("CompilerVersion"), (int)InOut.CompilerVersion);
	Params.emplace(CStrID("CompilerFlags"), (int)InOut.CompilerFlags);
	Params.emplace(CStrID("SrcPath"), InOut.SrcFile.Path);
	Params.emplace(CStrID("SrcModifyTimestamp"), (int)InOut.SrcModifyTimestamp);
	Params.emplace(CStrID("SrcSize"), (int)InOut.SrcFile.Size);
	Params.emplace(CStrID("SrcCRC"), (int)InOut.SrcFile.CRC);
	Params.emplace(CStrID("BinaryFileID"), (int)InOut.ObjFile.ID);
	Params.emplace(CStrID("InputSigFileID"), (int)InOut.InputSigFile.ID);

	if (!ExecuteStatement(SQLUpdateShaderRec, nullptr, &Params)) return false;

	InOut.ID = ID;

	Params.clear();
	Params.emplace(CStrID("ShaderID"), (int)InOut.ID);
	if (!ExecuteStatement(SQLClearDefines, nullptr, &Params)) return false;

	// TODO: check if batch is better
	/*
	DELETE FROM Macros WHERE ShaderID = :ShaderID;

	INSERT INTO table1 (column1,column2 ,..)
	VALUES 
	   (value1,value2 ,...),
	   (value1,value2 ,...),
		...
	   (value1,value2 ,...);	
	*/
	for (const auto& Macro : InOut.Defines)
	{
		if (Macro.first.empty()) continue;
		Params.emplace(CStrID("Name"), Macro.first);
		Params.emplace(CStrID("Value"), Macro.second);
		if (!ExecuteStatement(SQLInsertDefine, nullptr, &Params)) return false;
	}

	return true;
}
//---------------------------------------------------------------------

bool FindSignatureRecord(CSignatureRecord& InOut, const char* pBasePath, const void* pBinaryData)
{
	InOut.ID = 0;
	InOut.Folder.clear();

	if (!InOut.Size) return false; // Not found

	Data::CParams Params;
	Params.emplace(CStrID("Size"), (int)InOut.Size);
	Params.emplace(CStrID("CRC"), (int)InOut.CRC);

	CValueTable Result;
	if (!ExecuteSQLQuery("SELECT ID, Folder FROM InputSignatures WHERE Size=:Size AND CRC=:CRC", &Result, &Params)) return false;

	const int Col_ID = Result.GetColumnIndex(CStrID("ID"));
	const int Col_Folder = Result.GetColumnIndex(CStrID("Folder"));
	for (size_t i = 0; i < Result.GetRowCount(); ++i)
	{
		const int ID = Result.Get<int>(Col_ID, i);
		const std::string Folder = Result.Get<std::string>(Col_Folder, i);

		// If binary data is passed in, compare bytewise
		if (pBinaryData)
		{
			auto Path = fs::path(Folder) / (std::to_string(ID) + ".sig");
			if (pBasePath) Path = fs::path(pBasePath) / Path;

			std::vector<char> Buffer;
			if (!ReadAllFile(Path.string().c_str(), Buffer)) continue;
			if (Buffer.size() != InOut.Size) continue;
			if (memcmp(pBinaryData, Buffer.data(), Buffer.size()) != 0) continue;
		}

		InOut.ID = ID;
		InOut.Folder = std::move(Folder);
		return true; // Found
	}

	return false; // Not found
}
//---------------------------------------------------------------------

bool WriteSignatureRecord(CSignatureRecord& InOut)
{
	if (InOut.Folder.empty() || !InOut.Size) return false;

	if (!InOut.ID)
	{
		// If new record, try to find free ID
		CValueTable Result;
		if (!ExecuteSQLQuery("SELECT ID FROM InputSignatures WHERE Size = 0 ORDER BY ID LIMIT 1", &Result)) return false;
		if (Result.GetRowCount())
			InOut.ID = Result.Get<int>(CStrID("ID"), 0);
	}

	Data::CParams Params;
	Params.emplace(CStrID("Folder"), InOut.Folder);
	Params.emplace(CStrID("Size"), (int)InOut.Size);
	Params.emplace(CStrID("CRC"), (int)InOut.CRC);

	if (!InOut.ID)
	{
		// No free ID, insert a new line
		if (!ExecuteSQLQuery("INSERT INTO InputSignatures (Folder, Size, CRC) VALUES (:Folder, :Size, :CRC)", nullptr, &Params)) return false;
		InOut.ID = (uint32_t)sqlite3_last_insert_rowid(SQLiteHandle);
		return InOut.ID != 0;
	}
	else
	{
		// We have ID, update existing row
		Params.emplace(CStrID("ID"), (int)InOut.ID);
		return ExecuteSQLQuery("UPDATE InputSignatures SET Folder=:Folder, Size=:Size, CRC=:CRC WHERE ID=:ID", nullptr, &Params);
	}
}
//---------------------------------------------------------------------

bool ReleaseSignatureRecord(uint32_t ID, std::string& OutPath)
{
	if (ID == 0) return false;

	Data::CParams Params;
	Params.emplace(CStrID("ID"), (int)ID);

	CValueTable Result;
	if (!ExecuteSQLQuery("SELECT ID FROM Shaders WHERE InputSigFileID=:ID", &Result, &Params)) return false;

	// References found, don't delete file
	if (Result.GetRowCount()) return false;

	// Get file path to delete
	Result.Clear();
	if (!ExecuteSQLQuery("SELECT Folder FROM InputSignatures WHERE ID=:ID", &Result, &Params)) return false;

	const auto& Folder = Result.Get<std::string>(CStrID("Folder"), 0);
	OutPath = (fs::path(Folder) / (std::to_string(ID) + ".sig")).string();

	return ExecuteSQLQuery("UPDATE InputSignatures SET Size=0 WHERE ID=:ID", nullptr, &Params);
}
//---------------------------------------------------------------------

bool FindBinaryRecord(CBinaryRecord& InOut, const void* pBinaryData, bool USM, EObjCompareMode Mode)
{
	InOut.ID = 0;
	InOut.Path.clear();

	if (!InOut.BytecodeSize) return false; // Not found

	Data::CParams Params;
	Params.emplace(CStrID("BytecodeSize"), (int)InOut.BytecodeSize);
	Params.emplace(CStrID("CRC"), (int)InOut.CRC);

	CValueTable Result;
	if (!ExecuteStatement(SQLFindObjFile, &Result, &Params)) return false;

	const int Col_ID = Result.GetColumnIndex(CStrID("ID"));
	const int Col_Path = Result.GetColumnIndex(CStrID("Path"));
	for (size_t i = 0; i < Result.GetRowCount(); ++i)
	{
		std::string Path = Result.Get<std::string>(Col_Path, i);

		// If binary data is passed in, compare bytewise
		if (pBinaryData)
		{
			std::ifstream File(Path);
			if (!File) continue;

			File.seekg(0, std::ios_base::end);
			size_t FileSize = static_cast<size_t>(File.tellg());
			assert(FileSize == File.tellg());

			if (Mode == EObjCompareMode::ShaderAndMetadata)
			{
				uint32_t Offset = USM ? 16 : 12;
				if (!File.seekg(Offset, std::ios_base::beg)) continue;
				FileSize -= Offset;
			}
			else if (Mode == EObjCompareMode::Shader)
			{
				uint32_t BytecodeOffset;
				if (!File.seekg(4, std::ios_base::beg)) continue;
				if (!File.read((char*)&BytecodeOffset, sizeof(BytecodeOffset))) continue;
				if (!File.seekg(BytecodeOffset, std::ios_base::beg)) continue;
				FileSize -= BytecodeOffset;
				if (FileSize != InOut.BytecodeSize) continue;
			}
			else
			{
				File.seekg(0, std::ios_base::beg);
			}

			if (!FileSize) continue;

			std::unique_ptr<char[]> Buffer(new char[FileSize]);
			if (!File.read(Buffer.get(), (size_t)FileSize)) continue;

			if (memcmp(pBinaryData, Buffer.get(), (size_t)FileSize) != 0) continue;
		}

		InOut.ID = Result.Get<int>(Col_ID, i);
		InOut.Path = std::move(Path);
		return true; // Found
	}

	return false; // Not found
}
//---------------------------------------------------------------------

bool WriteBinaryRecord(CBinaryRecord& InOut)
{
	if (InOut.Path.empty() || !InOut.Size) return false;

	if (!InOut.ID)
	{
		// If new record, try to find free ID
		CValueTable Result;
		if (!ExecuteStatement(SQLFindFreeObjFileRec, &Result)) return false;
		if (Result.GetRowCount())
			InOut.ID = Result.Get<int>(CStrID("ID"), 0);
	}

	Data::CParams Params;
	Params.emplace(CStrID("Path"), InOut.Path);
	Params.emplace(CStrID("Size"), (int)InOut.Size);
	Params.emplace(CStrID("BytecodeSize"), (int)InOut.BytecodeSize);
	Params.emplace(CStrID("CRC"), (int)InOut.CRC);

	if (!InOut.ID)
	{
		// No free ID, insert a new line
		if (!ExecuteSQLQuery("INSERT INTO ShaderBinaries (Path, Size, BytecodeSize, CRC) VALUES (:Path, :Size, :BytecodeSize, :CRC)", nullptr, &Params)) return false;
		InOut.ID = (uint32_t)sqlite3_last_insert_rowid(SQLiteHandle);
		return InOut.ID != 0;
	}
	else
	{
		// We have ID, update existing row
		Params.emplace(CStrID("ID"), (int)InOut.ID);
		return ExecuteStatement(SQLUpdateObjFileRec, nullptr, &Params);
	}
}
//---------------------------------------------------------------------

bool ReleaseBinaryRecord(uint32_t ID, std::string& OutPath)
{
	if (ID == 0) return false;

	Data::CParams Params;
	Params.emplace(CStrID("ID"), (int)ID);

	CValueTable Result;
	if (!ExecuteStatement(SQLFindObjFileUsage, &Result, &Params)) return false;

	// References found, don't delete file
	if (Result.GetRowCount()) return false;

	// Get file path to delete
	Result.Clear();
	if (!ExecuteStatement(SQLGetObjFile, &Result, &Params)) return false;
	OutPath = Result.Get<std::string>(CStrID("Path"), 0);

	return ExecuteStatement(SQLReleaseObjFileRec, nullptr, &Params);
}
//---------------------------------------------------------------------

bool FindObjFileByID(uint32_t ID, CBinaryRecord& Out)
{
	Data::CParams Params;
	Params.emplace(CStrID("ID"), (int)ID);

	CValueTable Result;
	if (!ExecuteStatement(SQLFindObjFileByID, &Result, &Params)) return false;
	if (!Result.GetRowCount()) return false;

	int Col_ID = Result.GetColumnIndex(CStrID("ID"));
	int Col_Path = Result.GetColumnIndex(CStrID("Path"));
	int Col_Size = Result.GetColumnIndex(CStrID("Size"));
	int Col_BytecodeSize = Result.GetColumnIndex(CStrID("BytecodeSize"));
	int Col_CRC = Result.GetColumnIndex(CStrID("CRC"));
	Out.ID = Result.Get<int>(Col_ID, 0);
	Out.Path = Result.Get<std::string>(Col_Path, 0);
	Out.Size = Result.Get<int>(Col_Size, 0);
	Out.BytecodeSize = Result.Get<int>(Col_BytecodeSize, 0);
	Out.CRC = Result.Get<int>(Col_CRC, 0);

	return true; // Found
}
//---------------------------------------------------------------------

}