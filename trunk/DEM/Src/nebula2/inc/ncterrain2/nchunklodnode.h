#ifndef N_CHUNKLODNODE_H
#define N_CHUNKLODNODE_H
//------------------------------------------------------------------------------
/**
    @class nChunkLodNode
    @ingroup NCTerrain2

    @brief A node in a quad tree of LOD chunks.

    (C) 2003 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "kernel/nref.h"
#include "ncterrain2/nchunklodmesh.h"
#include "mathlib/bbox.h"
#include "ncterrain2/nchunklodrenderparams.h"
#include "ncterrain2/nchunklodtree.h"

class nChunkLodTree;

//------------------------------------------------------------------------------
class nChunkLodNode
{
public:
    /// constructor
    nChunkLodNode();
    /// destructor
    virtual ~nChunkLodNode();
    /// recursively read chunk parameters from .chu file
    bool LoadParams(Data::CFileStream* file, int recurseCount, nChunkLodTree* tree);
    /// convert neighbour labels to pointers
    void LookupNeighbours(nChunkLodTree* tree);
    /// set pointer to parent chunk
    void SetParent(nChunkLodNode* p);
    /// get pointer to parent chunk
    nChunkLodNode* GetParent() const;
    /// set pointer to child
    void SetChild(uint index, nChunkLodNode* c);
    /// get pointer to child
    nChunkLodNode* GetChild(uint index);
    /// get quadtree level of the node
    int GetLevel() const;
    /// get x position in quadtree level
    int GetX() const;
    /// get z position in quadtree level
    int GetZ() const;
    /// set lod value
    void SetLod(ushort l);
    /// get lod value
    ushort GetLod() const;
    /// return true if node has children
    bool HasChildren() const;
    /// get the status of the split flag
    bool GetSplit() const;
    /// compute the bounding box of this node
    void ComputeBoundingBox(nChunkLodTree* tree, bbox3& box) const;
    /// clears the split values throughout the quad tree
    void ClearSplits();
    /// update geometry based on viewpoint
    void Update(nChunkLodTree* tree, const vector3& viewPoint);
    /// update textures based on viewpoint
    void UpdateTexture(nChunkLodTree* tree, const vector3& viewPoint);
    /// returns true when the data for this chunk is available
    bool IsValid() const;
    /// split this chunk (makes it valid for rendering)
    void DoSplit(nChunkLodTree* tree, const vector3& viewPoint);
    /// return true if this chunk can be split
    bool CanSplit(nChunkLodTree* tree);
    /// schedule this node's data for loading at the given priority
    void WarmUpData(nChunkLodTree* tree, float priority);
    /// request to load the data for this node at the given priority
    void RequestLoadData(nChunkLodTree* tree, float priority);
    /// request to load texture for this node
    void RequestLoadTexture(nChunkLodTree* tree);
    /// request unloading the data of this node
    void RequestUnloadData(nChunkLodTree* tree);
    /// request to unload texture of this node
    void RequestUnloadTexture(nChunkLodTree* tree);
    /// request unloading the chunk node and all child nodes
    void RequestUnloadSubtree(nChunkLodTree* tree);
    /// request to unload textures for this node and all its descendants
    void RequestUnloadTextures(nChunkLodTree* tree);
    /// render the chunk lod node
    int Render(nChunkLodRenderParams& renderParams, bbox3::ClipStatus clipStatus, bool textureBound);
    /// return true if contains valid texture
    bool HasValidTexture() const;
    /// get the chunk's texture
    nTexture2* GetTexture() const;
    /// get this chunk's texgen parameters
    void GetTexGen(nChunkLodTree* tree, nFloat4& texGenS, nFloat4& texGenT) const;
	/// get a chunklod mesh
	nChunkLodMesh* GetChunkLodMesh() const;

private:
    /// bind our texture 
    void RenderTexture(nChunkLodRenderParams& renderParams);

    nChunkLodNode* parent;          // pointer to parent chunk
    nChunkLodNode* children[4];     // pointer to children
    union
    {
        int label;
        nChunkLodNode* node;
    } neighbour[4];                 // labels / pointers to neighbor nodes

    ushort x;                       // chunk "address" (its position in the quadtree)
    ushort z;                    
    uchar level;

    short minY;                     // vertical bounds, for constructing bounding box
    short maxY;

    bool split;                     // true if this node should be rendered by descendents
    ushort lod;                     // LOD of this chunk.  high byte never changes; low byte is the morph parameter.

    int vertexDataPosition;
    nChunkLodMesh* chunkLodMesh;
    nTexture2* chunkLodTexture;
};

//------------------------------------------------------------------------------
/**
    Compute the bounding box for this node.
*/
inline
void
nChunkLodNode::ComputeBoundingBox(nChunkLodTree* tree, bbox3& box) const
{
    float levelFactor = float(1 << (tree->treeDepth - 1 - this->level));
    static vector3 center;
    static vector3 extent;
    const vector3& origin = tree->GetTerrainOrigin();

    center.x = ((this->x + 0.5f) * levelFactor * tree->baseChunkDimension) - origin.x;
    center.y = ((this->maxY + this->minY) * 0.5f * tree->verticalScale) - origin.y;
    center.z = ((this->z + 0.5f) * levelFactor * tree->baseChunkDimension) - origin.z;

    const float extraBoxSize = 1e-3f;   // this is to make chunks overlap by about a millimeter, to avoid cracks.
    extent.x = levelFactor * tree->baseChunkDimension * 0.5f + extraBoxSize;
    extent.y = (this->maxY + this->minY) * 0.5f * tree->verticalScale;
    extent.z = extent.x;

    box.set(center, extent);
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nChunkLodNode::GetLevel() const
{
    return this->level;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nChunkLodNode::GetX() const
{
    return this->x;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nChunkLodNode::GetZ() const
{
    return this->z;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nChunkLodNode::SetParent(nChunkLodNode* p)
{
    this->parent = p;
}

//------------------------------------------------------------------------------
/**
*/
inline
nChunkLodNode*
nChunkLodNode::GetParent() const
{
    return this->parent;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nChunkLodNode::SetChild(uint index, nChunkLodNode* c)
{
    n_assert(index < 4);
    this->children[index] = c;
}

//------------------------------------------------------------------------------
/**
*/
inline
nChunkLodNode*
nChunkLodNode::GetChild(uint index)
{
    n_assert(index < 4);
    return this->children[index];
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nChunkLodNode::SetLod(ushort l)
{
    this->lod = l;
}

//------------------------------------------------------------------------------
/**
*/
inline
ushort
nChunkLodNode::GetLod() const
{
    return this->lod;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nChunkLodNode::HasChildren() const
{
    return (this->children[0] != 0);
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nChunkLodNode::IsValid() const
{
    if (this->chunkLodMesh && (this->chunkLodMesh->IsValid()))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nChunkLodNode::GetSplit() const
{
    return this->split;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nChunkLodNode::HasValidTexture() const
{
    if (this->chunkLodTexture && (this->chunkLodTexture->IsValid()))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    Get the chunk's texture. May return 0 if texture is not valid!
*/
inline
nTexture2*
nChunkLodNode::GetTexture() const
{
    if (this->HasValidTexture())
    {
        return this->chunkLodTexture;
    }
    else
    {
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
nChunkLodMesh*
nChunkLodNode::GetChunkLodMesh() const
{
	if(this->IsValid())
		return this->chunkLodMesh;
	return 0;
}

//------------------------------------------------------------------------------
#endif

