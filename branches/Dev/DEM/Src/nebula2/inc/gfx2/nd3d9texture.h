#ifndef N_D3D9TEXTURE_H
#define N_D3D9TEXTURE_H
//------------------------------------------------------------------------------
/**
    @class nD3D9Texture
    @ingroup Gfx2

    Direct3D9 subclass for nTexture2.

    (C) 2003 RadonLabs GmbH
*/
#include "gfx2/ntexture2.h"
#include "gfx2/nd3d9server.h"

//------------------------------------------------------------------------------
class nD3D9Texture : public nTexture2
{
public:
    /// constructor
    nD3D9Texture();
    /// destructor
    virtual ~nD3D9Texture();
    /// supports async resource loading
    virtual bool CanLoadAsync() const;
    /// lock a 2D texture, returns pointer and pitch
    virtual bool Lock(LockType lockType, int level, LockInfo& lockInfo);
    /// unlock 2d texture
    virtual void Unlock(int level);
    /// lock a cube face
    virtual bool LockCubeFace(LockType lockType, CubeFace face, int level, LockInfo& lockInfo);
    /// unlock a cube face
    virtual void UnlockCubeFace(CubeFace face, int level);
    /// get an estimated byte size of the resource data (for memory statistics)
    virtual int GetByteSize();
    /// save texture to file
    virtual bool SaveTextureToFile(const nString &filename, FileFormat fileFormat);

    /// filters mipmap levels of a texture.
    virtual void GenerateMipMaps();

protected:
    /// load texture resource (create render target if render target resource)
    virtual bool LoadResource();
    /// unload texture resource
    virtual void UnloadResource();
    /// called when contained resource may become lost
    virtual void OnLost();
    /// called when contained resource may be restored
    virtual void OnRestored();

private:
    friend class nD3D9Server;
    friend class nD3D9Shader;
    friend class nD3D9Surface;
    friend class nAllocatorPresenter;

    /// get pD3D9 base texture interface
    IDirect3DBaseTexture9* GetBaseTexture();
    /// get render target surface (returns 0 if no render target)
    IDirect3DSurface9* GetRenderTarget();
    /// get optional render target depth stencil surface
    IDirect3DSurface9* GetDepthStencil();
    /// get pD3D9 texture interface
    IDirect3DTexture9* GetTexture2D();
    /// verify pixelformat of render target
    bool CheckRenderTargetFormat(IDirect3D9* pD3D9, IDirect3DDevice9* pD3D9Device, DWORD usage, D3DFORMAT pixelFormat);
    /// convert file format to D3DX file format
    static D3DXIMAGE_FILEFORMAT FileFormatToD3DX(FileFormat fileFormat);
    /// convert nTexture2 format to D3D9FORMAT.
    static D3DFORMAT FormatToD3DFormat(nTexture2::Format format);

    /// create a render target texture
    bool CreateRenderTarget();
    /// create an empty 2d or cube texture
    bool CreateEmptyTexture();
    /// load texture through D3DX
    bool LoadD3DXFile(bool genMipMaps);
    /// load from a raw compound file
    bool LoadFromRawCompoundFile();
    /// load from dds compound file
    bool LoadFromDDSCompoundFile();
    /// get attributes from d3d texture and update my own attributes from them
    void QueryD3DTextureAttributes();
    /// loads an .ogg video file
    bool LoadOGGFile();

    IDirect3DBaseTexture9* baseTexture;
    IDirect3DTexture9* texture2D;
    IDirect3DCubeTexture9* textureCube;
    IDirect3DSurface9* renderTargetSurface;
    IDirect3DSurface9* depthStencilSurface;

    //CVideoPlayer* videoPlayer;
};

//------------------------------------------------------------------------------
/**
*/
inline
IDirect3DBaseTexture9*
nD3D9Texture::GetBaseTexture()
{
    n_assert(this->IsLoaded());
    return this->baseTexture;
}

//------------------------------------------------------------------------------
/**
*/
inline
IDirect3DTexture9*
nD3D9Texture::GetTexture2D()
{
    n_assert(this->IsLoaded());
    return this->texture2D;
}

//------------------------------------------------------------------------------
/**
*/
inline
IDirect3DSurface9*
nD3D9Texture::GetRenderTarget()
{
    n_assert(this->IsLoaded());
    return this->renderTargetSurface;
}

//------------------------------------------------------------------------------
/**
*/
inline
IDirect3DSurface9*
nD3D9Texture::GetDepthStencil()
{
    n_assert(this->IsLoaded());
    return this->depthStencilSurface;
}

//------------------------------------------------------------------------------
#endif

