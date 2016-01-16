#pragma once
#ifndef __DEM_TOOLS_D3D_INCLUDE_H__
#define __DEM_TOOLS_D3D_INCLUDE_H__

//#include <Render/D3D9/D3D9Fwd.h>
#include <Data/String.h>

#define WIN32_LEAN_AND_MEAN
#include <d3dcompiler.h>

// Overrides default include handling for D3D shader compiler

class CDEMD3DInclude: public ID3DInclude
{
private:

	CString ShaderDir;
	CString ShaderRootDir;

public:

	CDEMD3DInclude(const CString& ShdDir, const CString& ShdRootDir);

    STDMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE IncludeType, const char* pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes);
    STDMETHOD(Close)(THIS_ LPCVOID pData) { n_free((void*)pData); return S_OK; }
};

#endif
