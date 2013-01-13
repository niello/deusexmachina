#ifndef N_D3D9SHADERINCLUDE_H
#define N_D3D9SHADERINCLUDE_H

#include "util/nstring.h"
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3dx9.h>

// Override default include handling in D3DX FX files.
// (C) 2004 RadonLabs GmbH

class CD3DXNebula2Include: public ID3DXInclude
{
private:

	nString ShaderDir;
	nString ShaderRootDir;

public:

	CD3DXNebula2Include(const nString& ShdDir, const nString& ShdRootDir);

	STDMETHOD(Open)(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
	STDMETHOD(Close)(LPCVOID pData) { n_free((void*)pData); return S_OK; }
};

inline CD3DXNebula2Include::CD3DXNebula2Include(const nString& ShdDir, const nString& ShdRootDir):
	ShaderDir(ShdDir),
	ShaderRootDir(ShdRootDir)
{
	ShaderDir.StripTrailingSlash();
	ShaderDir += "/";
	ShaderRootDir.StripTrailingSlash();
	ShaderRootDir += "/";
}
//---------------------------------------------------------------------

#endif
