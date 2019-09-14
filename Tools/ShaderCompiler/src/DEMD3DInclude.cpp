#include "DEMD3DInclude.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// trim from end (in place)
static inline void rtrim(std::string& s, const std::string& whitespace)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [&whitespace](int ch)
	{
		return whitespace.find(ch) == std::string::npos;
	}).base(), s.end());
}

CDEMD3DInclude::CDEMD3DInclude(const std::string& ShdDir, const std::string& ShdRootDir):
	ShaderDir(ShdDir),
	ShaderRootDir(ShdRootDir)
{
	rtrim(ShaderDir, " \r\n\t\\/");
	ShaderDir += '/';
	rtrim(ShaderRootDir, " \r\n\t\\/");
	ShaderRootDir += '/';
}
//---------------------------------------------------------------------

HRESULT CDEMD3DInclude::Open(THIS_ D3D_INCLUDE_TYPE IncludeType, const char* pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
{
	// Try absolute path first
	std::ifstream File(pFileName);

	// Try in shader dir
	if (!File && !ShaderDir.empty())
	{
		File.open(ShaderDir + pFileName);

		// Try in shader root dir
		if (!File && !ShaderRootDir.empty())
		{
			File.open(ShaderRootDir + pFileName);
		}

		if (!File)
		{
			//Sys::Log("CDEMD3DInclude: could not open include file '%s' nor\n\t'%s' nor\n\t'%s'!\n",
			//	pFileName, (ShaderDir + pFileName).c_str(), (ShaderRootDir + pFileName).c_str());
			return E_FAIL;
		}
	}
	
	std::string Contents(static_cast<std::stringstream const&>(std::stringstream() << File.rdbuf()).str());

	File.close();

	void* pBuf = malloc(Contents.size());
	if (!pBuf) return E_FAIL;

	memcpy(pBuf, Contents.c_str(), Contents.size());

	*ppData = pBuf;
	*pBytes = Contents.size();

	return S_OK;
}
//---------------------------------------------------------------------

HRESULT CDEMD3DInclude::Close(THIS_ LPCVOID pData)
{
	free((void*)pData);
	return S_OK;
}
//---------------------------------------------------------------------
