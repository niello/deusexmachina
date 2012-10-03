#ifndef N_D3D9SHADERINCLUDE_H
#define N_D3D9SHADERINCLUDE_H
//------------------------------------------------------------------------------
/**
    @class nD3D9ShaderInclude
    @ingroup Gfx2

    Override default include handling in D3DX FX files.

    (C) 2004 RadonLabs GmbH
*/
#include "util/nstring.h"
#include <d3dx9.h>

//------------------------------------------------------------------------------
class nD3D9ShaderInclude : public ID3DXInclude
{
public:
    /// constructor
    nD3D9ShaderInclude(const nString& sDir);
    /// open an include file and read its contents
    STDMETHOD(Open)(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
    /// close an include file
    STDMETHOD(Close)(LPCVOID pData);

private:
    nString shaderDir;
};
//------------------------------------------------------------------------------
#endif
