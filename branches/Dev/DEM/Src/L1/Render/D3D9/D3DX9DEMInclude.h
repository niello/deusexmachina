//#ifndef N_D3D9SHADERINCLUDE_H
//#define N_D3D9SHADERINCLUDE_H
//
//#include <Render/D3D9/D3D9Fwd.h>
//#include <Data/String.h>
//
//#define WIN32_LEAN_AND_MEAN
//#define D3D_DISABLE_9EX
//#include <d3dx9.h>
//
//// Override default include handling in D3DX FX files.
//// (C) 2004 RadonLabs GmbH
//
//class CD3DX9DEMInclude: public ID3DXInclude
//{
//private:
//
//	CString ShaderDir;
//	CString ShaderRootDir;
//
//public:
//
//	CD3DX9DEMInclude(const CString& ShdDir, const CString& ShdRootDir);
//
//	STDMETHOD(Open)(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
//	STDMETHOD(Close)(LPCVOID pData) { n_free((void*)pData); return S_OK; }
//};
//
//inline CD3DX9DEMInclude::CD3DX9DEMInclude(const CString& ShdDir, const CString& ShdRootDir):
//	ShaderDir(ShdDir),
//	ShaderRootDir(ShdRootDir)
//{
//	ShaderDir.StripTrailingSlash();
//	ShaderDir += "/";
//	ShaderRootDir.StripTrailingSlash();
//	ShaderRootDir += "/";
//}
////---------------------------------------------------------------------
//
//#endif
