#pragma once
#include <string>
#include <vector>
#include <map>

class CValueTable;

namespace Data
{
	typedef std::vector<std::pair<class CStringID, class CData>> CParams;
}

namespace DB
{

struct CSourceRecord
{
	uint32_t	ID;
	uint64_t	Size;
	uint32_t	CRC;
	std::string	Path;
};

struct CBinaryRecord
{
	uint32_t	ID;
	uint64_t	Size;
	uint64_t	BytecodeSize;
	uint32_t	CRC;
	std::string	Path;
};

struct CSignatureRecord
{
	uint32_t	ID;
	uint64_t	Size;
	uint32_t	CRC;
	std::string	Folder; // Signature file name is based on ID and can be restored from ID+Folder
};

struct CShaderRecord
{
	uint32_t			ID = 0;
	uint32_t			ShaderType;
	uint32_t			Target = 0;
	uint32_t			CompilerVersion;
	uint32_t			CompilerFlags;
	std::string			EntryPoint;
	CSourceRecord		SrcFile;
	uint64_t			SrcModifyTimestamp;
	CBinaryRecord		ObjFile;
	CSignatureRecord	InputSigFile;
	std::map<std::string, std::string> Defines;
};

bool		OpenConnection(const char* pURI);
void		CloseConnection();

bool		FindShaderRecord(CShaderRecord& InOut);
bool		WriteShaderRecord(CShaderRecord& InOut);

bool		FindSignatureRecord(CSignatureRecord& InOut, const char* pBasePath, const void* pBinaryData);
bool		WriteSignatureRecord(CSignatureRecord& InOut);
bool		ReleaseSignatureRecord(uint32_t ID, std::string& OutPath);

bool		FindBinaryRecord(CBinaryRecord& InOut, const char* pBasePath, const void* pBinaryData, bool USM);
bool		WriteBinaryRecord(CBinaryRecord& InOut);
bool		ReleaseBinaryRecord(uint32_t ID, std::string& OutPath);

bool		FindObjFileByID(uint32_t ID, CBinaryRecord& Out);

bool		ExecuteSQLQuery(const char* pSQL, CValueTable* pOutTable = nullptr, const Data::CParams* pParams = nullptr);

}