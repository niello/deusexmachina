#pragma once
#include <string>
#include <vector>
#include <map>

class CValueTable;

namespace Data
{
	typedef std::map<class CStringID, class CData> CParams;
}

struct CSrcFileData
{
	uint32_t	ID;
	uint64_t	Size;
	uint32_t	CRC;
	std::string	Path;
};

struct CObjFileData
{
	uint32_t	ID;
	uint64_t	Size;
	uint64_t	BytecodeSize;
	uint32_t	CRC;
	std::string	Path;
};

struct CMacroDBRec
{
	const char*	Name;
	const char*	Value;

	//!!!sorting cmp operators!
};

struct CShaderDBRec
{
	uint32_t					ID;
	uint32_t					ShaderType;
	uint32_t					Target;
	uint32_t					CompilerFlags;
	std::string					EntryPoint;
	CSrcFileData				SrcFile;
	uint64_t					SrcModifyTimestamp;
	CObjFileData				ObjFile;
	CObjFileData				InputSigFile;
	std::vector<CMacroDBRec>	Defines;

	CShaderDBRec(): ID(0), Target(0) {}
};

enum EObjCompareMode
{
	Cmp_All,
	Cmp_ShaderAndMetadata,
	Cmp_Shader
};

bool		OpenDB(const char* pURI);
void		CloseDB();
bool		FindShaderRec(CShaderDBRec& InOut);
bool		WriteShaderRec(CShaderDBRec& InOut);
bool		FindObjFile(CObjFileData& InOut, const void* pBinaryData, uint32_t Target, EObjCompareMode Mode);
bool		FindObjFileByID(uint32_t ID, CObjFileData& Out);
uint32_t	CreateObjFileRecord();
bool		UpdateObjFileRecord(const CObjFileData& Record);
bool		ReleaseObjFile(uint32_t ID, std::string& OutPath);
bool		ExecuteSQLQuery(const char* pSQL, CValueTable* pOutTable = nullptr, const Data::CParams* pParams = nullptr);
