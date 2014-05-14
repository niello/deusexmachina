// Loads texture from supported image file using D3DX texture loader
// Use function declaration instead of header file where you want to call this loader.

#include <Render/RenderServer.h>
#include <IO/Streams/FileStream.h>
#include <Data/Buffer.h>

namespace Render
{

bool LoadTextureUsingD3DX(IO::CStream& In, PTexture OutTexture)
{
	if (!OutTexture.IsValid()) FAIL;

	DWORD DataSize = In.GetSize();
	Data::CBuffer Buffer(DataSize);
	n_assert(In.Read(Buffer.GetPtr(), DataSize) == DataSize);

	D3DXIMAGE_INFO ImageInfo = { 0 };
	if (FAILED(D3DXGetImageInfoFromFileInMemory(Buffer.GetPtr(), DataSize, &ImageInfo))) FAIL;

	if (ImageInfo.ResourceType == D3DRTYPE_TEXTURE)
	{
		IDirect3DTexture9* pTex = NULL;
		if (FAILED(D3DXCreateTextureFromFileInMemory(RenderSrv->GetD3DDevice(), Buffer.GetPtr(), DataSize, &pTex))) FAIL;
		return OutTexture->Setup((IDirect3DBaseTexture9*)pTex, CTexture::Texture2D);
	}
	else if (ImageInfo.ResourceType == D3DRTYPE_VOLUMETEXTURE)
	{
		IDirect3DVolumeTexture9* pTex = NULL;
		if (FAILED(D3DXCreateVolumeTextureFromFileInMemory(RenderSrv->GetD3DDevice(), Buffer.GetPtr(), DataSize, &pTex))) FAIL;
		return OutTexture->Setup((IDirect3DBaseTexture9*)pTex, CTexture::Texture3D);
	}
	else if (ImageInfo.ResourceType == D3DRTYPE_CUBETEXTURE)
	{
		IDirect3DCubeTexture9* pTex = NULL;
		if (FAILED(D3DXCreateCubeTextureFromFileInMemory(RenderSrv->GetD3DDevice(), Buffer.GetPtr(), DataSize, &pTex))) FAIL;
		return OutTexture->Setup((IDirect3DBaseTexture9*)pTex, CTexture::TextureCube);
	}

	Core::Log("LoadTextureUsingD3DX() -> Unknown texture type!");
	FAIL;
}
//---------------------------------------------------------------------

bool LoadTextureUsingD3DX(const CString& FileName, PTexture OutTexture)
{
	IO::CFileStream File;
	return File.Open(FileName, IO::SAM_READ, IO::SAP_SEQUENTIAL) &&
		LoadTextureUsingD3DX(File, OutTexture);
}
//---------------------------------------------------------------------

}