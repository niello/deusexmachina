#ifndef N_D3D9SHADERINCLUDE_H
#define N_D3D9SHADERINCLUDE_H

#include "util/nstring.h"
#include <d3dx9.h>

// Override default include handling in D3DX FX files.
// (C) 2004 RadonLabs GmbH

class CD3DXNebula2Include: public ID3DXInclude
{
private:

	nString shaderDir;

public:

	CD3DXNebula2Include(const nString& sDir): shaderDir(sDir + "/") {}
	STDMETHOD(Open)(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
	STDMETHOD(Close)(LPCVOID pData) { n_free((void*)pData); return S_OK; }
};

#endif
