#ifndef N_TQT2COMPRESSOR_H
#define N_TQT2COMPRESSOR_H
//------------------------------------------------------------------------------
/**
    @class nTqt2Compressor
    @ingroup NCTerrain2Tools

    @brief Compress a TQT2 file from RAW to DDS format. Uses D3DX for
    compressing texture data.
    
    (C) 2003 RadonLabs GmbH
*/
#include "kernel/nkernelserver.h"
#include "Data/Streams/FileStream.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3d9.h>
#include <d3dx9.h>

//------------------------------------------------------------------------------
class nTqt2Compressor
{
public:
    /// compression mode
    enum Mode
    {
        DXT1,       // ignore alpha
        DXT5,       // encode alpha
    };
    /// constructor
    nTqt2Compressor(nKernelServer* ks);
    /// destructor
    ~nTqt2Compressor();
    /// set filename of source tqt2 file (in RAW ARGB format)
    void SetSourceFile(const char* name);
    /// get filename of source tqt2 file
    const char* GetSourceFile() const;
    /// set filename of target tqt2 file (converted to DDS format)
    void SetTargetFile(const char* name);
    /// get filename of target tqt2 file
    const char* GetTargetFile() const;
    /// set compression mode
    void SetMode(Mode m);
    /// get compression mode
    Mode GetMode() const;
    /// set the error string
    void __cdecl SetError(const char* msg, ...);
    /// get error string
    const char* GetError() const;
    /// run the compressor
    bool Run();

private:
    /// compute number of nodes from quad tree depth
    int GetNumNodes(int level) const;
    /// open source file
    bool OpenSourceFile();
    /// close source file
    void CloseSourceFile();
    /// open target file
    bool OpenTargetFile();
    /// close target file
    void CloseTargetFile();
    /// compress one tile
    bool CompressTile(int index);

    /// a table of contents entry
    struct TocEntry
    {
        int pos;
        int size;
    };

    nKernelServer* kernelServer;
    nString sourceFileName;
    nString targetFileName;
    nString error;
    IDirect3D9* d3d9;                           // pointer to D3D9 object
    IDirect3DDevice9* d3d9Dev;                  // pointer to D3D9 reference device object
    Data::CFileStream* sourceFile;
    Data::CFileStream* targetFile;
    int treeDepth;
    int tileSize;
    nArray<TocEntry> sourceToc;
    nArray<TocEntry> targetToc;
    Mode mode;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nTqt2Compressor::SetMode(Mode m)
{
    this->mode = m;
}

//------------------------------------------------------------------------------
/**
*/
inline
nTqt2Compressor::Mode
nTqt2Compressor::GetMode() const
{
    return this->mode;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTqt2Compressor::SetSourceFile(const char* name)
{
    this->sourceFileName = name;
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nTqt2Compressor::GetSourceFile() const
{
    return this->sourceFileName.Get();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTqt2Compressor::SetTargetFile(const char* name)
{
    this->targetFileName = name;
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nTqt2Compressor::GetTargetFile() const
{
    return this->targetFileName.Get();
}

//------------------------------------------------------------------------------
/**
*/
inline
void __cdecl
nTqt2Compressor::SetError(const char* msg, ...)
{
    va_list argList;
    va_start(argList, msg);
    char buf[1024];
    vsnprintf(buf, sizeof(buf), msg, argList);
    va_end(argList);
    this->error = buf;
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nTqt2Compressor::GetError() const
{
    return this->error.Get();
}

//------------------------------------------------------------------------------
#endif

