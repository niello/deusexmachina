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

class nTerrainNode : public nMaterialNode
{
protected:
    
	nRef<nChunkLodTree> refChunkLodTree;
    float maxPixelError;
    float maxTexelSize;

public:

	nString ChunkFileName;
    nString TQTFileName;
    float TerrainScale;
    vector3 TerrainOrigin;

	nTerrainNode(): maxPixelError(5.0f), maxTexelSize(1.0f), TerrainScale(1.0f) {}
	virtual ~nTerrainNode() { if (AreResourcesValid()) UnloadResources(); }

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

    virtual bool LoadResources();
    virtual void UnloadResources();
	virtual void RenderContextCreated(nRenderContext* renderContext);
	virtual bool HasGeometry() const { return true; }
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext);

    nChunkLodTree* GetChunkLodTree();
    void SetMaxPixelError(float e);
	float GetMaxPixelError() const { return maxPixelError; }
    void SetMaxTexelSize(float s);
    float GetMaxTexelSize() const { return maxTexelSize; }
};

inline nChunkLodTree* nTerrainNode::GetChunkLodTree()
{
    if (!AreResourcesValid()) LoadResources();
    n_assert(this->refChunkLodTree.isvalid());
    return this->refChunkLodTree.get();
}
//---------------------------------------------------------------------

inline void nTerrainNode::SetMaxPixelError(float e)
{
    this->maxPixelError = e;
    if (this->refChunkLodTree.isvalid())
        this->refChunkLodTree->SetMaxPixelError(e);
}
//---------------------------------------------------------------------

inline void nTerrainNode::SetMaxTexelSize(float s)
{
    this->maxTexelSize = s;
    if (this->refChunkLodTree.isvalid())
        this->refChunkLodTree->SetMaxTexelSize(s);
}
//---------------------------------------------------------------------

#endif

