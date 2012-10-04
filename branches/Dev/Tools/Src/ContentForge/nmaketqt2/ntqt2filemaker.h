#ifndef N_TQT2FILEMAKER_H
#define N_TQT2FILEMAKER_H
//------------------------------------------------------------------------------
/**
    @class nTqt2FileMaker
    @ingroup NCTerrain2Tools

    @brief Create a TQT2 Texture Quadtree file from a large tga image.

    Differences to Thatcher Ulrich's orginal tqt fileformat:
     - Replace the jpeg compression with an uncompressed format (3 or 4 bytes
       per pixel), or a compressed DDS format (done in a second pass).
    
    (C) 2003 RadonLabs GmbH
*/
#include "kernel/nkernelserver.h"
#include "Data/Streams/FileStream.h"
#include <IL/il.h>
#include <IL/ilu.h>
#include <stdarg.h>

//------------------------------------------------------------------------------
class nTqt2FileMaker
{
public:
    /// constructor
    nTqt2FileMaker(nKernelServer* ks);
    /// destructor
    ~nTqt2FileMaker();
    /// set file name of source texture
    void SetSourceFile(const char* name);
    /// get file name of source texture
    const char* GetSourceFile() const;
    /// set file name of target tqt file
    void SetTargetFile(const char* name);
    /// get file name of target tqt file
    const char* GetTargetFile() const;
    /// set the tile size (must be power of 2)
    void SetTileSize(int s);
    /// get the tile size
    int GetTileSize() const;
    /// set the tree depth (between 1 and 12)
    void SetTreeDepth(int d);
    /// get the tree depth
    int GetTreeDepth() const;
    /// set the error string
    void __cdecl SetError(const char* msg, ...);
    /// get error string
    const char* GetError() const;
    /// run the file maker
    bool Run();

private:
    /// open files
    bool OpenFiles();
    /// close files
    void CloseFiles();
    /// computes number of quadtree nodes in a tree of given depth
    int CountNodes(int depth);
    /// compute node index by depth, row and col in quadtree
    int GetNodeIndex(int depth, int col, int row);
    /// recursively generate tiles
    ILuint RecurseGenerateTiles(int level, int col, int row);
    /// copy image to another image
    void CopyImage(ILuint srcImage, ILuint dstImage, int dstX, int dstY);

    /// a table of contents entry
    struct TocEntry
    {
        int pos;
        int size;
    };

    nKernelServer* kernelServer;
    nArray<TocEntry> toc;
    nString srcFileName;
    nString tarFileName;
    Data::CFileStream* tarFile;
    Data::CFileStream* tmpFile;
    int tileSize;
    int treeDepth;
    nString error;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nTqt2FileMaker::SetSourceFile(const char* name)
{
    this->srcFileName = name;
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nTqt2FileMaker::GetSourceFile() const
{
    return this->srcFileName.Get();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTqt2FileMaker::SetTargetFile(const char* name)
{
    this->tarFileName = name;
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nTqt2FileMaker::GetTargetFile() const
{
    return this->tarFileName.Get();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTqt2FileMaker::SetTileSize(int size)
{
    this->tileSize = size;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nTqt2FileMaker::GetTileSize() const
{
    return this->tileSize;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTqt2FileMaker::SetTreeDepth(int d)
{
    this->treeDepth = d;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nTqt2FileMaker::GetTreeDepth() const
{
    return this->treeDepth;
}

//------------------------------------------------------------------------------
/**
*/
inline
void __cdecl
nTqt2FileMaker::SetError(const char* msg, ...)
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
nTqt2FileMaker::GetError() const
{
    return this->error.Get();
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nTqt2FileMaker::CountNodes(int depth)
{
    // wow, neat trick...
    return 0x55555555 & ((1 << depth * 2) - 1);
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nTqt2FileMaker::GetNodeIndex(int level, int col, int row)
{
    n_assert((col >= 0) && (col < (1 << level)));
    n_assert((row >= 0) && (row < (1 << level)));
    return this->CountNodes(level) + (row << level) + col;
}

//------------------------------------------------------------------------------
#endif
