#ifndef N_TERRAINNODE_H
#define N_TERRAINNODE_H
//------------------------------------------------------------------------------
/**
    @class nTerrainNode
    @ingroup NCTerrain2

    @brief Scene graph node for the ncterrain2 package.

    (C) 2003 RadonLabs GmbH
*/
#include "scene/nmaterialnode.h"
#include "ncterrain2/nchunklodtree.h"

class nResourceServer;

namespace Data
{
	class CBinaryReader;
}

//------------------------------------------------------------------------------
class nTerrainNode : public nMaterialNode
{
public:
    /// constructor
    nTerrainNode();
    /// destructor
    virtual ~nTerrainNode();

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	/// load resources
    virtual bool LoadResources();
    /// unload resources
    virtual void UnloadResources();

    /// indicate to scene server that we offer geometry for rendering
    virtual bool HasGeometry() const;
    /// render geometry
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext);

    /// get pointer to chunk lod tree object
    nChunkLodTree* GetChunkLodTree();
    /// set filename of .chu file
    void SetChunkFile(const char* name);
    /// get filename of .chu file
    const char* GetChunkFile() const;
    /// set texture quad file
    void SetTexQuadFile(const char* name);
    /// get texture quad file
    const char* GetTexQuadFile() const;
    /// set maximum pixel error
    void SetMaxPixelError(float e);
    /// get maximum pixel error
    float GetMaxPixelError() const;
    /// set maximum texel error
    void SetMaxTexelSize(float s);
    /// get maximum texel error
    float GetMaxTexelSize() const;
    /// set terrain scale
    void SetTerrainScale(float s);
    /// get terrain scale
    float GetTerrainScale() const;
    /// set terrain origin
    void SetTerrainOrigin(const vector3& orig);
    /// get terrain origin
    const vector3& GetTerrainOrigin() const;

protected:
    nString chunkFilename;
    nString texQuadFile;
    nRef<nChunkLodTree> refChunkLodTree;
    float maxPixelError;
    float maxTexelSize;
    float terrainScale;
    vector3 terrainOrigin;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nTerrainNode::SetTerrainScale(float s)
{
    this->terrainScale = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nTerrainNode::GetTerrainScale() const
{
    return this->terrainScale;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTerrainNode::SetTerrainOrigin(const vector3& orig)
{
    this->terrainOrigin = orig;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
nTerrainNode::GetTerrainOrigin() const
{
    return this->terrainOrigin;
}

//------------------------------------------------------------------------------
/**
*/
inline
nChunkLodTree*
nTerrainNode::GetChunkLodTree()
{
    if (!this->AreResourcesValid())
    {
        this->LoadResources();
    }
    n_assert(this->refChunkLodTree.isvalid());
    return this->refChunkLodTree.get();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTerrainNode::SetChunkFile(const char* name)
{
    this->chunkFilename = name;
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nTerrainNode::GetChunkFile() const
{
    return this->chunkFilename.Get();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTerrainNode::SetTexQuadFile(const char* name)
{
    this->texQuadFile = name;
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nTerrainNode::GetTexQuadFile() const
{
    return this->texQuadFile.Get();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTerrainNode::SetMaxPixelError(float e)
{
    this->maxPixelError = e;
    if (this->refChunkLodTree.isvalid())
    {
        this->refChunkLodTree->SetMaxPixelError(e);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nTerrainNode::GetMaxPixelError() const
{
    return this->maxPixelError;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nTerrainNode::SetMaxTexelSize(float s)
{
    this->maxTexelSize = s;
    if (this->refChunkLodTree.isvalid())
    {
        this->refChunkLodTree->SetMaxTexelSize(s);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nTerrainNode::GetMaxTexelSize() const
{
    return this->maxTexelSize;
}

//------------------------------------------------------------------------------
#endif

