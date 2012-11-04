#ifndef N_CHUNKLODTREE_H
#define N_CHUNKLODTREE_H
//------------------------------------------------------------------------------
/**
    @class nChunkLodTree
    @ingroup NCTerrain2

    @brief A tree of ChunkLOD nodes. Takes the path to a <tt>.chu</tt> file
    and constructs a dynamically updated quadtree from it.

    (C) 2003 RadonLabs GmbH
*/
#include "resource/nresource.h"
#include "gfx2/DisplayMode.h"
#include "ncterrain2/ntexturequadtree.h"
#include "util/narray.h"
#include "ncterrain2/nclodeventhandler.h"

class nResourceServer;
class nGfxServer2;
class nShader2;

//------------------------------------------------------------------------------
class nChunkLodTree : public nResource
{
public:
    /// constructor
    nChunkLodTree();
    /// destructor
    virtual ~nChunkLodTree();
    /// set terrain scale
    void SetTerrainScale(float s);
    /// get terrain scale
    float GetTerrainScale() const;
    /// set terrain origin (post-scale)
    void SetTerrainOrigin(const vector3& orig);
    /// get terrain origin (post-scale)
    const vector3& GetTerrainOrigin() const;
    /// update the quad tree, call this when view matrix changes
    void Update(const vector3& viewPoint);
    /// begin rendering, resets render stats
    void BeginRender();
    /// render tree using the provided shader
    int Render(nShader2* shader, const matrix44& projection, const matrix44& modelViewProj);
    /// finish rendering
    void EndRender();
    /// set display parameters
    void SetDisplayMode(const CDisplayMode& mode);
    /// get display parameters
    const CDisplayMode& GetDisplayMode() const;
    /// set the filename of a texture quad tree file
    void SetTqtFilename(const char* filename);
    /// get the filename of the texture quad tree file
    const char* GetTqtFilename() const;
    /// set max pixel error
    void SetMaxPixelError(float e);
    /// get max pixel error
    float GetMaxPixelError() const;
    /// set max texel size
    void SetMaxTexelSize(float s);
    /// get max texel size
    float GetMaxTexelSize() const;
    /// get the bounding box
    const bbox3& GetBoundingBox() const;
    /// compute a desired lod level from bounding box and view point
    ushort ComputeLod(const bbox3& box, const vector3& viewPoint) const;
    /// compute desired texture level from bounding box and view point
    int ComputeTextureLod(const bbox3& box, const vector3& viewPoint) const;
    /// get pointer to open chu file
    Data::CFileStream* GetFile();
    /// get pointer to texture which is responsible for a given chunk coordinate
    nTexture2* GetBaseLevelTexture(int col, int row, nFloat4& texGenS, nFloat4& texGenT);
    /// get the base chunk dimension
    float GetBaseChunkDimension() const;
    /// get tree depth
    int GetTreeDepth() const;
    /// get number of meshes rendered
    int GetNumMeshesRendered() const;
    /// get number of textures rendered
    int GetNumTexturesRendered() const;
    /// get number of meshes currently allocated
    int GetNumMeshesAllocated() const;
    /// get number of textures currently allocated
    int GetNumTexturesAllocated() const;
    /// add event handler, incrs refs of event handler
    void AddEventHandler(nCLODEventHandler* handler);
    /// remove event handler, decrs ref of event handler
    void RemEventHandler(nCLODEventHandler* handler);
    /// get a chunk index from its quadtree address
    int GetChunkIndex(int level, int col, int row) const;
    /// get number of nodes in a level, including its child nodes
    int GetNumNodes(int level) const;
	/// get a root chunklod node
	nChunkLodNode* GetRootChunkLodNode() const;

protected:
    friend class nChunkLodNode;
    friend class nChunkLodMesh;

    /// load .chu file and create quad tree
    virtual bool LoadResource();
    /// unload internal nMesh2 object
    virtual void UnloadResource();
    /// associate a chunk pointer with its chunk label
    void SetChunkByLabel(int chunkLabel, nChunkLodNode* chunk);
    /// get a chunk by its label
    nChunkLodNode* GetChunkByLabel(int chunkLabel);
    /// compute the bounding box
    void UpdateBoundingBox();
    /// update user dependent parameters, clear paramsDirty flag
    void UpdateParams();
    /// distrubute an event to the event handlers
    void PutEvent(nCLODEventHandler::Event event, nChunkLodNode* node);

    float terrainScale;
    vector3 terrainOrigin;
    nString tqtFilename;
    nTextureQuadTree* texQuadTree;
    int chunksAllocated;
    nChunkLodNode* chunks;                  // root node of chunk tree
    short treeDepth;                          // tree depth, from .chu file
    float errorLodMax;                      // from .chu file
    float distanceLodMax;                   // computed from .chu file and set parameters
    float textureDistanceLodMax;            // computed from .chu file and set parameters
    float verticalScale;                    // from .chu file
    float baseChunkDimension;               // x/z size of root chunk
    int chunkCount;
	Data::CFileStream* chuFile;                         // open chunk file
    nArray<nChunkLodNode*> chunkTable;      // table of chunks, with chunk label as index

    CDisplayMode displayMode;
    float maxPixelError;
    float maxTexelSize;
    bool paramsDirty;

    int numTexturesRendered;
    int numMeshesRendered;
    int numTexturesAllocated;
    int numMeshesAllocated;

    bbox3 boundingBox;                          // computed bounding box

    nArray<nCLODEventHandler*> eventHandlers;   // array of event handlers
};

//------------------------------------------------------------------------------
/**
    Computes number of nodes in a level, including its child nodes.
*/
inline
int
nChunkLodTree::GetNumNodes(int level) const
{
    return 0x55555555 & ((1 << level * 2) - 1);
}

//------------------------------------------------------------------------------
/**
    Computes a linear chunk index for a chunk address consisting of 
    level, col and row. Index can be used to access the chunkTable.
*/
inline
int
nChunkLodTree::GetChunkIndex(int level, int col, int row) const
{
    n_assert((col >= 0) && (col < (1 << level)));
    n_assert((row >= 0) && (row < (1 << level)));
    return this->GetNumNodes(level) + (row << level) + col;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nChunkLodTree::GetBaseChunkDimension() const
{
    return this->baseChunkDimension;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nChunkLodTree::GetTreeDepth() const
{
    return this->treeDepth;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nChunkLodTree::SetTerrainOrigin(const vector3& orig)
{
    this->terrainOrigin = orig;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3& 
nChunkLodTree::GetTerrainOrigin() const
{
    return this->terrainOrigin;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nChunkLodTree::SetTerrainScale(float s)
{
    this->terrainScale = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nChunkLodTree::GetTerrainScale() const
{
    return this->terrainScale;
}

//------------------------------------------------------------------------------
/**
*/
inline
const bbox3&
nChunkLodTree::GetBoundingBox() const
{
    return this->boundingBox;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nChunkLodTree::SetTqtFilename(const char* filename)
{
    this->tqtFilename = filename;
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nChunkLodTree::GetTqtFilename() const
{
    return this->tqtFilename.Get();
}

//------------------------------------------------------------------------------
/**
*/
inline
Data::CFileStream*
nChunkLodTree::GetFile()
{
    n_assert(this->chuFile);
    return this->chuFile;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nChunkLodTree::SetDisplayMode(const CDisplayMode& mode)
{
    this->displayMode = mode;
    this->paramsDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
const CDisplayMode&
nChunkLodTree::GetDisplayMode() const
{
    return this->displayMode;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nChunkLodTree::SetMaxPixelError(float e)
{
    this->maxPixelError = e;
    this->paramsDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nChunkLodTree::GetMaxPixelError() const
{
    return this->maxPixelError;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nChunkLodTree::SetMaxTexelSize(float s)
{
    this->maxTexelSize = s;
    this->paramsDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nChunkLodTree::GetMaxTexelSize() const
{
    return this->maxTexelSize;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nChunkLodTree::SetChunkByLabel(int label, nChunkLodNode* chunk)
{
    this->chunkTable[label] = chunk;
}

//------------------------------------------------------------------------------
/**
*/
inline
nChunkLodNode*
nChunkLodTree::GetChunkByLabel(int label)
{
    return this->chunkTable[label];
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nChunkLodTree::GetNumMeshesRendered() const
{
    return this->numMeshesRendered;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nChunkLodTree::GetNumTexturesRendered() const
{
    return this->numTexturesRendered;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nChunkLodTree::GetNumMeshesAllocated() const
{
    return this->numMeshesAllocated;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nChunkLodTree::GetNumTexturesAllocated() const
{
    return this->numTexturesAllocated;
}

//------------------------------------------------------------------------------
/**
*/
inline
nChunkLodNode*
nChunkLodTree::GetRootChunkLodNode() const
{
	return this->chunks;
}

//------------------------------------------------------------------------------
#endif

