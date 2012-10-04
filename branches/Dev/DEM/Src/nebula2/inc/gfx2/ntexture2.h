#ifndef N_TEXTURE2_H
#define N_TEXTURE2_H
//------------------------------------------------------------------------------
/**
    @class nTexture2
    @ingroup Gfx2

    Contains image data used by the gfx api's texture samplers. Textures
    are normal named, shared resources which are usually loaded from disk.
    Textures can also be declared as render targets.

    The following code snip shows the creation of 16bit empty texture and
    filling it with white pixel:
    @code
    tex = (nTexture2 *)refGfx2->NewTexture("mytexture");
    if (!tex->IsUnloaded())
    {
        int width, height;
        width = height = 128;
        // create a 16bit 2D empty texture.
        tex->SetUsage(nTexture2::CreateEmpty);
        tex->SetType(nTexture2::TEXTURE_2D);
        tex->SetWidth(width);
        tex->SetHeight(height);
        tex->SetFormat(nTexture2::A1R5G5B5);
        tex->Load();

        // fill the created texture with white pixel.
        struct nTexture2::LockInfo surf;
        if (tex->Lock(nTexture2::WriteOnly, 0, surf))
        {
            unsigned short *surface = (unsigned short *)surf.surfPointer;
            for (unsigned int pixelByte=0; pixelByte < width*height; pixelByte++)
                surface[pixelByte] = 0xffff;
            tex->Unlock(0);
        }
    }
    @endcode

    You also can read pixel of the texture like the following:
    @code
    struct nTexture2::LockInfo surf;
    tex->Lock(nTexture2::ReadOnly, 0, surf)
    ushort *surface = (ushort*)surf.surfPointer;
    ushort color = surface[x + y*surf.surfPitch];
    tex->Unlock(0);
    @endcode

    The following code shows that another way of copying the image data of the memory
    to the created texture:
    @code
    // create an empty texture like above code.
    ...

    // get the surface of the texture
    nSurface *surface;

    // get the surface of level 0.
    tex->GetSurfaceLevel("/tmp/surface", 0, &surface);

    // copy the imageData which is the source image to the texture
    surface->LoadFromMemory(imageData, dstFormat, width, height, imagePitch);
    ...
    @endcode

    (C) 2002 RadonLabs GmbH
*/
#include "resource/nresource.h"

namespace Data
{
	class CFileStream;
}

//------------------------------------------------------------------------------
class nTexture2 : public nResource
{
public:
    /// texture type
    enum Type
    {
        TEXTURE_NOTYPE,
        TEXTURE_2D,                 // 2-dimensional
        TEXTURE_3D,                 // 3-dimensional
        TEXTURE_CUBE,               // cube
    };

    // pixel formats
    enum Format
    {
        NOFORMAT,
        X8R8G8B8,
        A8R8G8B8,
        R5G6B5,
        A1R5G5B5,
        A4R4G4B4,
        P8,
        G16R16,
        DXT1,
        DXT2,
        DXT3,
        DXT4,
        DXT5,
        R16F,                       // 16 bit float, red only
        G16R16F,                    // 32 bit float, 16 bit red, 16 bit green
        A16B16G16R16F,              // 64 bit float, 16 bit rgba each
        R32F,                       // 32 bit float, red only
        G32R32F,                    // 64 bit float, 32 bit red, 32 bit green
        A32B32G32R32F,              // 128 bit float, 32 bit rgba each
        A8,
    };

    // file formats
    enum FileFormat
    {
        BMP,
        JPG,
        TGA,
        PNG,
        DDS,
        PPM,
        DIB,
        HDR,
        PFM,
    };

    // the sides of a cube map
    enum CubeFace
    {
        PosX = 0,
        NegX,
        PosY,
        NegY,
        PosZ,
        NegZ,
    };

    // usage flags
    enum Usage
    {
        CreateEmpty = (1<<0),               // don't load from disk, instead create empty texture
        CreateFromRawCompoundFile = (1<<1), // create from a compound file as raw ARGB pixel chunk
        CreateFromDDSCompoundFile = (1<<2), // create from dds file inside a compound file
        RenderTargetColor = (1<<3),         // is render target, has color buffer
        RenderTargetDepth = (1<<4),         // is render target, has depth buffer
        RenderTargetStencil = (1<<5),       // is render target, has stencil buffer
        Dynamic = (1<<6),                   // is a dynamic texture (for write access with CPU)
        Video = (1<<7)                      // is a Video
    };

    // lock types
    enum LockType
    {
        ReadOnly,       // cpu will only read from texture
        WriteOnly,      // cpu will only write to texture (an overwrite everything!)
    };

    // lock information
    struct LockInfo
    {
        void* surfPointer;
        int   surfPitch;
    };

    /// constructor
    nTexture2();
    /// destructor
    virtual ~nTexture2();
    /// set combination of usage flags
    void SetUsage(int useFlags);
    /// get usage flags combination
    int GetUsage() const;
    /// check usage flags if this is a render target
    bool IsRenderTarget() const;
    /// set compound file read data
    void SetCompoundFileData(Data::CFileStream* file, int filePos, int byteSize);
    /// set texture type (render target only!)
    void SetType(Type t);
    /// get texture type
    Type GetType() const;
    /// set texture's pixel format (render target only!)
    void SetFormat(Format f);
    /// get texture's pixel format
    Format GetFormat() const;
    /// set width (render target only!)
    void SetWidth(int w);
    /// get width
    int GetWidth() const;
    /// set height (render target only!)
    void SetHeight(int h);
    /// get height
    int GetHeight() const;
    /// set depth (render target only! oops: 3d render targets?)
    void SetDepth(int d);
    /// get depth
    int GetDepth() const;
    /// get number of mipmaps
    int GetNumMipLevels() const;
    /// get bytes per pixel (computed from pixel format)
    int GetBytesPerPixel() const;
    /// lock a 2D texture, returns pointer and pitch
    virtual bool Lock(LockType lockType, int level, LockInfo& lockInfo);
    /// unlock 2D texture
    virtual void Unlock(int level);
    /// lock a cube face
    virtual bool LockCubeFace(LockType lockType, CubeFace face, int level, LockInfo& lockInfo);
    /// unlock a cube face
    virtual void UnlockCubeFace(CubeFace face, int level);
    /// convert string to pixel format
    static Format StringToFormat(const char* str);
    /// convert pixel format to string
    static const char* FormatToString(Format fmt);
    /// convert string to type
    static Type StringToType(const char* str);
    /// convert type to string
    static const char* TypeToString(Type t);
    /// save data in texture into a file
    virtual bool SaveTextureToFile(const nString& filename, FileFormat fileFormat);

    virtual void GenerateMipMaps();

protected:
    /// set number of mipmaps
    void SetNumMipLevels(int num);

    Type type;
    Format format;
    ushort usage;
    ushort width;
    ushort height;
    ushort depth;
    ushort numMipMaps;
    Data::CFileStream* compoundFile;
    int compoundFilePos;
    int compoundFileDataSize;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nTexture2::SetCompoundFileData(Data::CFileStream* file, int filePos, int byteSize)
{
    n_assert(file);
    n_assert(byteSize > 0);
    if (this->compoundFile)
    {
        this->compoundFile = 0;
    }
    this->compoundFile = file;
    this->compoundFilePos = filePos;
    this->compoundFileDataSize = byteSize;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTexture2::SetUsage(int useFlags)
{
    this->usage = useFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nTexture2::GetUsage() const
{
    return this->usage;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nTexture2::IsRenderTarget() const
{
    return (0 != (this->usage & (RenderTargetColor | RenderTargetDepth | RenderTargetStencil)));
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTexture2::SetType(Type t)
{
    this->type = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
nTexture2::Type
nTexture2::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTexture2::SetFormat(Format f)
{
    this->format = f;
}

//------------------------------------------------------------------------------
/**
*/
inline
nTexture2::Format
nTexture2::GetFormat() const
{
    return this->format;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTexture2::SetWidth(int w)
{
    this->width = w;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nTexture2::GetWidth() const
{
    return this->width;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTexture2::SetHeight(int h)
{
    this->height = h;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nTexture2::GetHeight() const
{
    return this->height;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTexture2::SetDepth(int d)
{
    this->depth = d;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nTexture2::GetDepth() const
{
    return this->depth;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTexture2::SetNumMipLevels(int num)
{
    this->numMipMaps = num;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nTexture2::GetNumMipLevels() const
{
    return this->numMipMaps;
}

//------------------------------------------------------------------------------
/**
    Returns the bytes per pixel for the current pixel format. May be
    incorrect for compressed textures!

    - 17-Jun-05    kims    Added 'A8' to return its bytes per pixel.
*/
inline
int
nTexture2::GetBytesPerPixel() const
{
    switch (this->format)
    {
        case X8R8G8B8:
        case A8R8G8B8:
            return 4;

        case R5G6B5:
        case A1R5G5B5:
        case A4R4G4B4:
            return 2;

        case P8:
        case A8:
            return 1;

        case DXT1:
        case DXT2:
        case DXT3:
        case DXT4:
        case DXT5:
            n_error("nTexture2::GetBytesPerPixel(): compressed pixel format!");
            return 1;

        case R16F:
            return 2;

        case G16R16:
        case G16R16F:
            return 4;

        case A16B16G16R16F:
            return 8;

        case R32F:
            return 4;

        case G32R32F:
            return 8;

        case A32B32G32R32F:
            return 16;

        default:
            n_error("nTexture2::GetBytesPerPixel(): invalid pixel format!");
            return 1;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nTexture2::FormatToString(Format fmt)
{
    switch (fmt)
    {
        case NOFORMAT:      return "NOFORMAT";
        case X8R8G8B8:      return "X8R8G8B8";
        case A8R8G8B8:      return "A8R8G8B8";
        case R5G6B5:        return "R5G6B5";
        case A1R5G5B5:      return "A1R5G5B5";
        case A4R4G4B4:      return "A4R4G4B4";
        case P8:            return "P8";
        case G16R16:        return "G16R16";
        case DXT1:          return "DXT1";
        case DXT2:          return "DXT2";
        case DXT3:          return "DXT3";
        case DXT4:          return "DXT4";
        case DXT5:          return "DXT5";
        case R16F:          return "R16F";
        case G16R16F:       return "G16R16F";
        case A16B16G16R16F: return "A16B16G16R16F";
        case R32F:          return "R32F";
        case G32R32F:       return "G32R32F";
        case A32B32G32R32F: return "A32B32G32R32F";
        case A8:            return "A8";
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
nTexture2::Format
nTexture2::StringToFormat(const char* str)
{
    n_assert(str);
    if (0 == strcmp("NOFORMAT", str))           return NOFORMAT;
    else if (0 == strcmp("X8R8G8B8", str))      return X8R8G8B8;
    else if (0 == strcmp("A8R8G8B8", str))      return A8R8G8B8;
    else if (0 == strcmp("R5G6B5", str))        return R5G6B5;
    else if (0 == strcmp("A1R5G5B5", str))      return A1R5G5B5;
    else if (0 == strcmp("A4R4G4B4", str))      return A4R4G4B4;
    else if (0 == strcmp("P8", str))            return P8;
    else if (0 == strcmp("G16R16", str))        return G16R16;
    else if (0 == strcmp("DXT1", str))          return DXT1;
    else if (0 == strcmp("DXT2", str))          return DXT2;
    else if (0 == strcmp("DXT3", str))          return DXT3;
    else if (0 == strcmp("DXT4", str))          return DXT4;
    else if (0 == strcmp("DXT5", str))          return DXT5;
    else if (0 == strcmp("R16F", str))          return R16F;
    else if (0 == strcmp("G16R16F", str))       return G16R16F;
    else if (0 == strcmp("A16B16G16R16F", str)) return A16B16G16R16F;
    else if (0 == strcmp("R32F", str))          return R32F;
    else if (0 == strcmp("G32R32F", str))       return G32R32F;
    else if (0 == strcmp("A32B32G32R32F", str)) return A32B32G32R32F;
    else if (0 == strcmp("A8", str))            return A8;
    else
    {
        n_error("nTexture2::StringToFormat(): invalid string '%s'", str);
        return NOFORMAT;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nTexture2::TypeToString(Type t)
{
    switch (t)
    {
        case TEXTURE_NOTYPE:    return "NoType";
        case TEXTURE_2D:        return "2D";
        case TEXTURE_3D:        return "3D";
        case TEXTURE_CUBE:      return "CUBE";
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
nTexture2::Type
nTexture2::StringToType(const char* str)
{
    n_assert(str);
    if (0 == strcmp(str, "NoType")) return TEXTURE_NOTYPE;
    if (0 == strcmp(str, "2D")) return TEXTURE_2D;
    if (0 == strcmp(str, "3D")) return TEXTURE_3D;
    if (0 == strcmp(str, "CUBE")) return TEXTURE_CUBE;
    n_error("nTexture2::StringToType(): invalid string '%s'", str);
    return TEXTURE_NOTYPE;
}

//------------------------------------------------------------------------------
#endif

