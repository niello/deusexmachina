//------------------------------------------------------------------------------
//  nchunklodnode.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "ncterrain2/nchunklodnode.h"
#include "ncterrain2/nchunklodtree.h"
#include "resource/nresourceserver.h"
#include "gfx2/nmesh2.h"
#include "gfx2/ngfxserver2.h"

//------------------------------------------------------------------------------
/**
*/
nChunkLodNode::nChunkLodNode() :
    parent(0),
    x(0),
    z(0),
    level(0),
    split(false),
    lod(0),
    minY(0),
    maxY(0),
    vertexDataPosition(0),
    chunkLodMesh(0),
    chunkLodTexture(0)
{
    int i;
    for (i = 0; i < 4; i++)
    {
        this->neighbour[i].label = 0;
        this->children[i] = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
nChunkLodNode::~nChunkLodNode()
{
    if (this->chunkLodMesh)
    {
        this->chunkLodMesh->Release();
        this->chunkLodMesh = 0;
    }
    if (this->chunkLodTexture)
    {
        this->chunkLodTexture->Release();
        this->chunkLodTexture = 0;
    }
}

//------------------------------------------------------------------------------
/**
    Recursively reads the chunk structure (not the actual mesh data)
    from the chu file and constructs the tree of chunk nodes.
*/
bool
nChunkLodNode::LoadParams(Data::CFileStream* file, int recurseCount, nChunkLodTree* tree)
{
    n_assert(file);
    n_assert(tree);

    // read the chunk's label
    int chunkLabel;
	file->Read(&chunkLabel, sizeof(int));
    tree->SetChunkByLabel(chunkLabel, this);

    // read neighbour labels
    for (int i = 0; i < 4; i++)
		file->Read(&neighbour[i].label, sizeof(int));

    // read chunk address
	file->Read(&level, sizeof(char));
	file->Read(&x, sizeof(ushort));
	file->Read(&z, sizeof(ushort));
	file->Read(&minY, sizeof(short));
	file->Read(&maxY, sizeof(short));

    // read vertex data file offset
	file->Read(&vertexDataPosition, sizeof(int));

    // recurse into child chunks
    if (recurseCount > 0)
    {
        for (int i = 0; i < 4; i++)
        {
            this->children[i] = &(tree->chunks[tree->chunksAllocated++]);
            this->children[i]->SetParent(this);
            this->children[i]->SetLod(this->lod + 0x100);
            this->children[i]->LoadParams(file, recurseCount - 1, tree);
        }
    }

    return true;
}

//------------------------------------------------------------------------------
/**
    Converts the neighbour chunk labels to pointers, recurse into child
    chunks.
*/
void
nChunkLodNode::LookupNeighbours(nChunkLodTree* tree)
{
    n_assert(tree);
    int i;
    for (i = 0; i < 4; i++)
    {
        if (this->neighbour[i].label == -1)
        {
            this->neighbour[i].node = 0;
        }
        else
        {
            this->neighbour[i].node = tree->GetChunkByLabel(this->neighbour[i].label);
        }
    }

    if (this->HasChildren())
    {
        for (i = 0; i < 4; i++)
        {
            this->children[i]->LookupNeighbours(tree);
        }
    }
}

//------------------------------------------------------------------------------
/**
    This clears the "split" node members throughout the quad tree.
    If this node is not split, then the recursion does not continue to the 
    child nodes, since their m_split values should be false.

    Do this before calling Update().
*/
void
nChunkLodNode::ClearSplits()
{
    // ???
    n_assert(this->IsValid());
    if (this->split)
    {
        this->split = false;
        if (this->HasChildren())
        {
            int i;
            for (i = 0; i < 4; i++)
            {
                this->children[i]->ClearSplits();
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Computes 'lod' and split values for this chunk and its subtree,
    based on the given camera parameters and the parameters stored in
    'base'.  Traverses the tree and forces neighbor chunks to a valid
    LOD, and updates its contained edges to prevent cracks.

    Invariant: if a node has m_split == true, then all its ancestors
    have m_split == true.
    
    Invariant: if a node has m_data != NULL, then all its ancestors
    have m_data != NULL.

    !!!  For correct results, the tree must have been clear()ed before
    calling update() !!!
*/
void
nChunkLodNode::Update(nChunkLodTree* tree, const vector3& viewPoint)
{
    static bbox3 box;
    this->ComputeBoundingBox(tree, box);

    ushort desiredLod = tree->ComputeLod(box, viewPoint);

    if (this->HasChildren() && (desiredLod > (this->lod | 0xff)) && this->CanSplit(tree))
    {
        this->DoSplit(tree, viewPoint);
        
        // recurse to children
        int i;
        for (i = 0; i < 4; i++)
        {
            this->children[i]->Update(tree, viewPoint);
        }
    }
    else
    {
		// We're good... this chunk can represent its region within the max error tolerance.
        if ((this->lod & 0xff00) == 0)
        {
			// Root chunk -- make sure we have valid morph value.
            this->lod = n_iclamp(desiredLod, this->lod & 0xff00, this->lod | 0xff);
            n_assert((this->lod >> 8) == this->level);
        }

		// Request residency for our children, and request our
		// grandchildren and further descendents be unloaded.
        if (this->HasChildren())
        {
            float priority = 0.0f;
            if (desiredLod > (this->lod & 0xff00))
            {
                priority = (this->lod & 0xff) / 255.0f;
            }
            if (priority < 0.5f)
            {
                int i;
                for (i = 0; i < 4; i++)
                {
                    this->children[i]->RequestUnloadSubtree(tree);
                }
            }
            else
            {
                int i;
                for (i = 0; i < 4; i++)
                {
                    this->children[i]->WarmUpData(tree, priority);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Decides when to load & release textures for this node and its
    descendents.

    Invariant: if a node has a non-zero chunkLodTexture, then all its
    ancestors have non-zero chunkLodTexture.

    !!! You must do things in this order: clear(), then update(), then texture_update(). !!!

    Note that we don't take the viewpoint direction into account when
    loading textures, only the viewpoint distance & mag factor.  That's
    intentional: it means we'll load a lot of textures that are beside
    or behind the frustum, but the viewpoint can rotate rapidly, so we
    really want them available.  The penalty is ~4x texture RAM, for a
    90-degree FOV, compared to only loading what's in the frustum.
    Interestingly, this approach ceases to work as the view angle gets
    smaller.  LOD in general has real problems as we approach an
    isometric view.  For games this isn't a huge concern, (except for
    sniper-scope views, where it's a real issue, but then there are
    compensating factors there too...).
*/
void
nChunkLodNode::UpdateTexture(nChunkLodTree* tree, const vector3& viewPoint)
{
    n_assert(tree->texQuadTree);

    if (this->level >= tree->texQuadTree->GetTreeDepth())
    {
        // No texture tiles at this level, so don't bother
        // thinking about them.
        n_assert(0 == this->chunkLodTexture);
        return;
    }

    static bbox3 box;
    this->ComputeBoundingBox(tree, box);
    int desiredTexLevel = tree->ComputeTextureLod(box, viewPoint);

    if (this->HasValidTexture())
    {
        // texture currently loaded
        n_assert((0 == this->parent) || (0 != this->parent->chunkLodTexture));

        // Decide if we should release our texture.
        if ((0 == this->chunkLodMesh) || (desiredTexLevel < this->level))
        {
            this->RequestUnloadTextures(tree);
        }
        else
        {
            // keep status quo for this node, and recurse to children
            if (this->HasChildren())
            {
                int i;
                for (i = 0; i < 4; i++)
                {
                    this->children[i]->UpdateTexture(tree, viewPoint);
                }
            }
        }
    }
    else
    {
        // currently no texture loaded
        // decide if we should load our texture
        if ((desiredTexLevel >= this->level) && this->chunkLodMesh)
        {
            // yes, we would like to load
            this->RequestLoadTexture(tree);
        }
        else
        {
            // no need to load anything, or to check children
        }

        // check to make sure children have no textures
        #ifdef DEBUG
        if (this->HasChildren())
        {
            int i;
            for (i = 0; i < 4; i++)
            {
                n_assert(0 == this->children[i]->chunkLodTexture);
            }
        }
        #endif
    }
}

//------------------------------------------------------------------------------
/**
    Enable this chunk.  Use the given viewpoint to decide what morph
    level to use.
*/
void
nChunkLodNode::DoSplit(nChunkLodTree* tree, const vector3& viewPoint)
{
    if (false == this->split)
    {
        n_assert(this->CanSplit(tree));
        n_assert(this->IsValid());
        
        this->split = true;

        if (this->HasChildren())
        {
			// Make sure children have a valid lod value.
            int i;
            for (i = 0; i < 4; i++)
            {
                nChunkLodNode* child = this->children[i];
                static bbox3 box;
                child->ComputeBoundingBox(tree, box);
                ushort desiredLod = tree->ComputeLod(box, viewPoint);
                child->lod = n_iclamp(desiredLod, child->lod & 0xff00, child->lod | 0xff);
            }
        }

        // make sure ancestors are split...
        nChunkLodNode* p;
        for (p = this->parent; p && (false == p->split); p = p->parent)
        {
            p->DoSplit(tree, viewPoint);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Return true if this chunk can be split.  Also, requests the
    necessary data for the chunk children and its dependents.

    A chunk won't be able to be split if it doesn't have vertex data,
    or if any of the dependent chunks don't have vertex data.
*/
bool
nChunkLodNode::CanSplit(nChunkLodTree* tree)
{
    if (this->split)
    {
		// Already split.  Also our data & dependents' data is already
		// freshened, so no need to request it again.
        return true;
    }

    if (false == this->HasChildren())
    {
		// Can't ever split.  No data to request.
		return false;
	}

    bool canSplit = true;

	// Check the data of the children.
    int i;
    for (i = 0; i < 4; i++)
    {
        nChunkLodNode* child = this->children[i];
        if (false == child->IsValid())
        {
            child->RequestLoadData(tree, 1.0f);
            canSplit = false;
        }
    }

    // make sure ancestors have data
    nChunkLodNode* p;
    for (p = this->parent; p && (false == p->split); p = p->parent)
    {
        if (false == p->CanSplit(tree))
        {
            canSplit = false;
        }
    }

	// Make sure neighbors have data at a close-enough level in the tree.
    for (i = 0; i < 4; i++)
    {
        nChunkLodNode* n = this->neighbour[i].node;
		const int maxAllowedNeighbourDifference = 2;    // allow up to two levels of difference between chunk neighbors.
        int count;
        for (count = 0; n && (count < maxAllowedNeighbourDifference); count++)
        {
            n = n->parent;
        }
        if (n && (false == n->CanSplit(tree)))
        {
            canSplit = false;
        }
    }
    return canSplit;
}

//------------------------------------------------------------------------------
/**
    Schedule this node's data for loading at the given priority.  Also,
    schedule our child/descendent nodes for unloading.
*/
void
nChunkLodNode::WarmUpData(nChunkLodTree* tree, float priority)
{
    if (!this->IsValid())
    {
        // request our data
        this->RequestLoadData(tree, priority);
    }

	// Request unload.  Skip a generation if our priority is
	// fairly high.
    if (this->HasChildren())
    {
        if (priority < 0.5f)
        {
			// Dump our child nodes' data.
            int i;
            for (i = 0; i < 4; i++)
            {
                this->children[i]->RequestUnloadSubtree(tree);
            }
        }
        else
        {
			// Fairly high priority; leave our children
			// loaded, but dump our grandchildren's data.
            int i;
            for (i = 0; i < 4; i++)
            {
                nChunkLodNode* child = this->children[i];
                if (child->HasChildren())
                {
                    int j;
                    for (j = 0; j < 4; j++)
                    {
                        child->children[j]->RequestUnloadSubtree(tree);
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    If we have any data, request that it be unloaded.  Make the same
    request of our descendants.
*/
void
nChunkLodNode::RequestUnloadSubtree(nChunkLodTree* tree)
{
    if (this->IsValid())
    {
		// Put descendents in the queue first, so they get
		// unloaded first.
        if (this->HasChildren())
        {
            int i;
            for (i = 0; i < 4; i++)
            {
                this->children[i]->RequestUnloadSubtree(tree);
            }
        }
        this->RequestUnloadData(tree);
    }
}

//------------------------------------------------------------------------------
/**
    If we have a texture, request that it be unloaded.  Make the same
    request of our descendants.
*/
void
nChunkLodNode::RequestUnloadTextures(nChunkLodTree* tree)
{
    if (this->chunkLodTexture)
    {
        if (this->HasChildren())
        {
            int i;
            for (i = 0; i < 4; i++)
            {
                this->children[i]->RequestUnloadTextures(tree);
            }
        }
        this->RequestUnloadTexture(tree);
    }
}

//------------------------------------------------------------------------------
/**
    Request to load the data for this node.
*/
void
nChunkLodNode::RequestLoadData(nChunkLodTree* tree, float /*priority*/)
{
    // load request already pending?
    if (this->chunkLodMesh)
    {
        return;
    }
	this->chunkLodMesh = (nChunkLodMesh*) nResourceServer::Instance()->NewResource("nchunklodmesh", 0, nResource::Other);
    n_assert(this->chunkLodMesh);
    
    // fill chunk mesh parameters
    static bbox3 box;
    this->ComputeBoundingBox(tree, box);
    this->chunkLodMesh->SetFile(tree->GetFile());
    this->chunkLodMesh->SetFileLocation(this->vertexDataPosition);

    vector3 scale = box.extents();
    scale.x /= float(1<<14);
    scale.y = tree->verticalScale;
    scale.z /= float(1<<14);
    vector3 offset = box.center();
    offset.y = 0.0f;

    this->chunkLodMesh->SetVertexScale(scale);
    this->chunkLodMesh->SetVertexOffset(offset);
    this->chunkLodMesh->SetAsyncEnabled(true);

    this->chunkLodMesh->Load();

    tree->numMeshesAllocated++;

    tree->PutEvent(nCLODEventHandler::RequestLoadData, this);

// n_printf("Loaded mesh %s\n", this->chunkLodMesh->GetMesh()->GetName());
}

//------------------------------------------------------------------------------
/**
    Request to load a texture into this node.
*/
void
nChunkLodNode::RequestLoadTexture(nChunkLodTree* tree)
{
    if (this->chunkLodTexture)
    {
        // we are already pending
        return;
    }

    n_assert(tree && tree->texQuadTree);
    n_assert(this->level < tree->texQuadTree->GetTreeDepth());

    this->chunkLodTexture = tree->texQuadTree->LoadTexture(this->level, this->x, this->z);
    n_assert(this->chunkLodTexture);

    tree->numTexturesAllocated++;

//    n_printf("Loaded texture %s\n", this->chunkLodTexture->GetName());
}

//------------------------------------------------------------------------------
/**
    Request to unload the data for this node.
*/
void
nChunkLodNode::RequestUnloadData(nChunkLodTree* tree)
{
    n_assert(this->chunkLodMesh);
    tree->PutEvent(nCLODEventHandler::UnloadData, this);
//    n_printf("Unload mesh %s\n", this->chunkLodMesh->GetMesh()->GetName());
    this->chunkLodMesh->Release();
    this->chunkLodMesh = 0;
    tree->numMeshesAllocated--;
}

//------------------------------------------------------------------------------
/**
    Request to unload the texture for this node.
*/
void
nChunkLodNode::RequestUnloadTexture(nChunkLodTree* tree)
{
    n_assert(this->chunkLodTexture);
//    n_printf("Unload texture %s\n", this->chunkLodTexture->GetName());
    this->chunkLodTexture->Release();
    this->chunkLodTexture = 0;
    tree->numTexturesAllocated--;
}

//------------------------------------------------------------------------------
/**
    Get the texgen parameters for this node.
*/
void
nChunkLodNode::GetTexGen(nChunkLodTree* tree, nFloat4& texGenS, nFloat4& texGenT) const
{
    n_assert(tree);
    static bbox3 box;
    this->ComputeBoundingBox(tree, box);
    vector3 center = box.center();
    vector3 extent = box.extents();

    // setup texgen parameters
    float xSize = extent.x * 2.0f * (257.0f / 256.0f);
    float zSize = extent.z * 2.0f * (257.0f / 256.0f);
    float x0 = center.x - extent.x - (xSize / 256.0f) * 0.5f;
    float z0 = center.z - extent.z - (xSize / 256.0f) * 0.5f;
    texGenS.x = 1.0f / xSize;
    texGenS.y = 0.0f;
    texGenS.z = 0.0f;
    texGenS.w = -x0 / xSize;
    texGenT.x = 0.0f;
    texGenT.y = 0.0f;
    texGenT.z = 1.0f / zSize;
    texGenT.w = -z0 / zSize;
}

//------------------------------------------------------------------------------
/**
    Bind the given texture to the current shader and set up texgen so that it 
    stretches over the x-z extent of the given box.
*/
void
nChunkLodNode::RenderTexture(nChunkLodRenderParams& renderParams)
{
    if (!this->HasValidTexture())
    {
        return;
    }

    nFloat4 texGenS, texGenT;
    this->GetTexGen(renderParams.tree, texGenS, texGenT);

    if (renderParams.shader->IsParameterUsed(nShaderState::TexGenS))
    {
        renderParams.shader->SetFloat4(nShaderState::TexGenS, texGenS);
    }

    if (renderParams.shader->IsParameterUsed(nShaderState::TexGenT))
    {
        renderParams.shader->SetFloat4(nShaderState::TexGenT, texGenT);
    }

    if (renderParams.shader->IsParameterUsed(nShaderState::TextureTransform0))
    {
        matrix44 invModelView = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvModelView);
        matrix44 texGenMatrix;

        texGenMatrix.M11 = texGenS.x;
        texGenMatrix.M21 = texGenS.y;
        texGenMatrix.M31 = texGenS.z;
        texGenMatrix.M41 = texGenS.w;

        texGenMatrix.M12 = texGenT.x;
        texGenMatrix.M22 = texGenT.y;
        texGenMatrix.M32 = texGenT.z;
        texGenMatrix.M42 = texGenT.w;

        texGenMatrix = invModelView * texGenMatrix;
        renderParams.shader->SetMatrix(nShaderState::TextureTransform0, texGenMatrix);
    }

    if (renderParams.shader->IsParameterUsed(nShaderState::TextureTransform1))
    {
        matrix44 invModelView = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvModelView);
        matrix44 texGenMatrix;

        texGenMatrix.M11 = texGenS.x;
        texGenMatrix.M21 = texGenS.y;
        texGenMatrix.M31 = texGenS.z;
        texGenMatrix.M41 = texGenS.w;

        texGenMatrix.M12 = texGenT.x;
        texGenMatrix.M22 = texGenT.y;
        texGenMatrix.M32 = texGenT.z;
        texGenMatrix.M42 = texGenT.w;

        texGenMatrix.scale(vector3(100.0f, 100.0f, 100.0f));

        texGenMatrix = invModelView * texGenMatrix;
        renderParams.shader->SetMatrix(nShaderState::TextureTransform1, texGenMatrix);
    }

    // finally bind texture
	if (renderParams.shader->IsParameterUsed(nShaderState::AmbientMap0))
    {
		renderParams.shader->SetTexture(nShaderState::AmbientMap0, this->chunkLodTexture);
    }

    renderParams.numTexturesRendered++;
}

//------------------------------------------------------------------------------
/**
    Recursively render the chunk lod node. Returns number of triangles.
*/
int
nChunkLodNode::Render(nChunkLodRenderParams& renderParams, 
                      bbox3::ClipStatus clipStatus,
                      bool textureBound)
{
    n_assert(this->IsValid());
    n_assert(clipStatus != bbox3::Outside);

    // view frustum culling (we don't need to check for culling if one
    // of our parents was already fully inside)
    if (bbox3::Clipped == clipStatus)
    {
        // cull against view frustum
        static bbox3 box;
        this->ComputeBoundingBox(renderParams.tree, box);
        clipStatus = box.clipstatus(renderParams.modelViewProj);
        if (bbox3::Outside == clipStatus)
        {
            // fully outside, return and break recursion
            return 0;
        }
    }

    // texture handling
    if (!textureBound)
    {
        // decide whether to bind a texture
        if (this->HasValidTexture())
        {
            // If there's no possibility of binding a
            // texture further down the tree, then bind
            // now.
            if (!this->GetSplit() ||
                (!this->children[0]->HasValidTexture()) ||
                (!this->children[1]->HasValidTexture()) ||
                (!this->children[2]->HasValidTexture()) ||
                (!this->children[3]->HasValidTexture()))
            {
                this->RenderTexture(renderParams);
                textureBound = true;
            }
        }
    }

    int triCount = 0;
    if (this->GetSplit())
    {
        n_assert(this->HasChildren());

        // a split node, recurse to children
        int i;
        for (i = 0; i < 4; i++)
        {
            triCount += this->children[i]->Render(renderParams, clipStatus, textureBound);
        }
    }
    else
    {
        // not split, display our own data
        triCount += this->chunkLodMesh->Render(renderParams);
        renderParams.tree->PutEvent(nCLODEventHandler::RenderNode, this);
        renderParams.numMeshesRendered++;
    }
    return triCount;
}
