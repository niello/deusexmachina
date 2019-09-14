#pragma once
#include <string>
#define WIN32_LEAN_AND_MEAN
#include <d3dcompiler.h>

// Overrides default include handling for D3D shader compiler

class CDEMD3DInclude: public ID3DInclude
{
private:

	std::string ShaderDir;
	std::string ShaderRootDir;

public:

	CDEMD3DInclude(const std::string& ShdDir, const std::string& ShdRootDir);

    STDMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE IncludeType, const char* pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes);
    STDMETHOD(Close)(THIS_ LPCVOID pData);
};
