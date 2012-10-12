//------------------------------------------------------------------------------
//  nchunklodtree_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "ncterrain2/nchunklodtree.h"
#include "ncterrain2/nchunklodnode.h"
#include "gfx2/nshader2.h"
#include "ncterrain2/nchunklodrenderparams.h"
#include "ncterrain2/nclodeventhandler.h"

nNebulaClass(nChunkLodTree, "nresource");

//------------------------------------------------------------------------------
/**
*/
nChunkLodTree::nChunkLodTree() :
    terrainScale(1.0f),
    terrainOrigin(0.0f, 0.0f, 0.0f),
    texQuadTree(0),
    chunksAllocated(0),
    chunks(0),
    treeDepth(0),
    errorLodMax(0.0f),
    distanceLodMax(0.0f),
    textureDistanceLodMax(0.0f),
    verticalScale(0.0f),
    baseChunkDimension(0.0),
    chunkCount(0),
    chuFile(0),
    paramsDirty(true),
    numMeshesRendered(0),
    numTexturesRendered(0),
    numMeshesAllocated(0),
    numTexturesAllocated(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nChunkLodTree::~nChunkLodTree()
{
    // clear remaining event handlers
    int num = this->eventHandlers.Size();
    int i;
    for (i = 0; i < num; i++)
    {
        this->eventHandlers[i]->Release();
        this->eventHandlers[i] = 0;
    }

    // unload resource if valid
    if (this->IsValid())
    {
        this->Unload();
    }
}

//------------------------------------------------------------------------------
/**
    Add an event handler object to the nChunkLodTree. Event handlers are
    notified about internal events like loading, unloading or rendering
    a node in the quad tree. Derive your event handler subclass from
    nCLODEventHandler and attach to a nChunkLodTree object. The
    refcount of the event handler object will be incremented.
*/
void
nChunkLodTree::AddEventHandler(nCLODEventHandler* handler)
{
    // make sure the handler is not already added
    n_assert(!this->eventHandlers.Find(handler));
    this->eventHandlers.Append(handler);
    handler->AddRef();
}

//------------------------------------------------------------------------------
/**
    Remove an event handler from the nChunkLodTree. The refcount of the
    event handler object will be decremented.
*/
void
nChunkLodTree::RemEventHandler(nCLODEventHandler* handler)
{
    // make sure the handler is found
    nArray<nCLODEventHandler*>::iterator iter = this->eventHandlers.Find(handler);
    n_assert(iter);
    this->eventHandlers.Erase(iter);
    handler->Release();
}

//------------------------------------------------------------------------------
/**
    Distribute event to event handlers.
*/
void
nChunkLodTree::PutEvent(nCLODEventHandler::Event event, nChunkLodNode* node)
{
    int num = this->eventHandlers.Size();
    int i;
    for (i = 0; i < num; i++)
    {
        this->eventHandlers[i]->OnEvent(event, node);
    }
}

//------------------------------------------------------------------------------
/**
    Update the user dependend parameters. Must be called in LoadResource()
    and when paramsDirty is set to true.

    - 05-Dec-03     floh    no longer display resolution dependent, set fixed
                            resolution size to 640
*/
void
nChunkLodTree::UpdateParams()
{
    n_assert(this->texQuadTree);
    n_assert(this->paramsDirty);
    this->paramsDirty = false;

    // compute user setting dependend stuff
    const float tanHalfFov = tanf(n_deg2rad(this->camera.GetAngleOfView() * 0.5f));
    const float K = 640.0f / tanHalfFov;

	// distance_LODmax is the distance below which we need to be
	// at the maximum LOD.  It's used in compute_lod(), which is
	// called by the chunks during update().
    this->distanceLodMax = (this->errorLodMax / this->maxPixelError) * K;
        
	// m_texture_distance_LODmax is the distance below which we
	// need to be at the leaf texture resolution.  It's used in
	// compute_texture_lod(), which is called by the chunks during
	// update_texture() to decide when to load textures.
    n_assert(this->texQuadTree->GetTileSize() > 1);
    float texelSizeLodMax = this->baseChunkDimension / (this->texQuadTree->GetTileSize() - 1); // 1 texel used up by the border(?)
    this->textureDistanceLodMax = (texelSizeLodMax / this->maxTexelSize) * K;
}

//------------------------------------------------------------------------------
/**
    This will open the .chu file and read the quadtree structure from it,
    but not the actual mesh data.
*/
bool
nChunkLodTree::LoadResource()
{
    n_assert(!this->IsValid());
    n_assert(0 == this->chunks);
    n_assert(0 == this->chuFile);
    n_assert(!this->tqtFilename.IsEmpty());

    // initialize the texture quad tree
	this->texQuadTree = n_new(nTextureQuadTree());
    if (!this->texQuadTree->Open(this->tqtFilename.Get()))
    {
        n_error("nChunkLodTree: Could not open .tqt2 file '%s'\n", this->tqtFilename.Get());
        return false;
    }

    // open the .chu file
    this->chuFile = n_new(Data::CFileStream);
	if (!this->chuFile->Open(this->GetFilename().Get(), Data::SAM_READ))
    {
        n_error("nChunkLodTree: Could not open .chu file '%s'\n", this->GetFilename().Get());
        return false;
    }

    // make sure it's a .chu file
    int magic;
	chuFile->Read(&magic, sizeof(int));
    if (magic != (('C') | ('H' << 8) | ('U' << 16)))
    {
        n_error("nChunkLodTree: File '%s' is not a .chu file!\n", this->GetFilename().Get());
        return false;
    }

    // format version
    short formatVersion;
	chuFile->Read(&formatVersion, sizeof(short));
    if (formatVersion != 9)
    {
        n_error("nChunkLodTree: unsupported .chu file format version %d!\n", this->GetFilename().Get(), formatVersion);
        return false;
    }

    // read global .chu file attributes
	chuFile->Read(&treeDepth, sizeof(short));
	chuFile->Read(&errorLodMax, sizeof(float));
	chuFile->Read(&verticalScale, sizeof(float));
	chuFile->Read(&baseChunkDimension, sizeof(float));
    this->verticalScale      *= terrainScale;
    this->baseChunkDimension *= terrainScale;
	chuFile->Read(&chunkCount, sizeof(int));
    this->chunkTable.SetFixedSize(this->chunkCount);
    this->chunkTable.Fill(0, this->chunkCount, 0);

    // load the chunk tree (not the actual data)
    this->chunks = n_new_array(nChunkLodNode, this->chunkCount);
    this->chunksAllocated = 1;
    this->chunks[0].SetParent(0);
    this->chunks[0].SetLod(0);
    this->chunks[0].LoadParams(this->chuFile, this->treeDepth - 1, this);
    n_assert(this->chunksAllocated == this->chunkCount);
    this->chunks[0].LookupNeighbours(this);

    // distribute chunk create events
    int i;
    for (i = 0; i < this->chunkCount; i++)
    {
        this->PutEvent(nCLODEventHandler::CreateNode, &(this->chunks[i]));
    }

    // update user dependent parameters
    this->paramsDirty = true;
    this->UpdateParams();

    // update the root bounding box
    this->UpdateBoundingBox();

    this->SetState(nResource::Valid);
    return true;
}

//------------------------------------------------------------------------------
/**
    Unload everything.
*/
void
nChunkLodTree::UnloadResource()
{
    n_assert(this->IsValid());

    // delete file object
    if (this->chuFile)
    {
        this->chuFile->Close();
        n_delete(chuFile);
        this->chuFile = 0;
    }

    // delete tree of chunks
    if (this->chunks)
    {
        // distribute destroy event
        int i;
        for (i = 0; i < this->chunkCount; i++)
        {
            this->PutEvent(nCLODEventHandler::DestroyNode, &(this->chunks[i]));
        }
        n_delete_array(this->chunks);
        this->chunks = 0;
    }

    // delete texture quad tree
    if (this->texQuadTree)
    {
        this->texQuadTree->Close();
        n_delete(this->texQuadTree);
        this->texQuadTree = 0;
    }

    //this->SetState(nResource::Valid);
	this->SetState(nResource::Unloaded);
}

//------------------------------------------------------------------------------
/**
    Compute the bounding box for the tree.
*/
void
nChunkLodTree::UpdateBoundingBox()
{
    this->chunks[0].ComputeBoundingBox(this, this->boundingBox);
}
    
//------------------------------------------------------------------------------
/**
    Given an AABB and the viewpoint, this function computes a desired
    LOD level, based on the distance from the viewpoint to the nearest
    point on the box.  So, desired LOD is purely a function of
    distance and the chunk tree parameters.
*/
ushort
nChunkLodTree::ComputeLod(const bbox3& box, const vector3& viewPoint) const
{
    static vector3 disp;
    static vector3 center;
    static vector3 extent;
    
    center = box.center();
    extent = box.extents();

    disp = viewPoint - center;
	disp.x = n_max(0.f, fabsf(disp.x) - extent.x);
	disp.y = n_max(0.f, fabsf(disp.y) - extent.y);
	disp.z = n_max(0.f, fabsf(disp.z) - extent.z);

    float d = disp.len();
    ushort lod = n_iclamp(((this->treeDepth << 8) - 1) - int(n_log2(n_max(1.f, d / this->distanceLodMax)) * 256), 0, 0xffff);

    return lod;
}

//------------------------------------------------------------------------------
/**
    Given an AABB and the viewpoint, this function computes a desired
    texture LOD level, based on the distance from the viewpoint to the
    nearest point on the box.  So, desired LOD is purely a function of
    distance and the texture tree & chunk tree parameters.
*/
int
nChunkLodTree::ComputeTextureLod(const bbox3& box, const vector3& viewPoint) const
{
    static vector3 disp;
    static vector3 center;
    static vector3 extent;

    center = box.center();
    extent = box.extents();
    
    disp = viewPoint - center;
	disp.x = n_max(0.f, fabsf(disp.x) - extent.x);
	disp.y = n_max(0.f, fabsf(disp.y) - extent.y);
	disp.z = n_max(0.f, fabsf(disp.z) - extent.z);

    float d = disp.len();
	return (this->treeDepth - 1 - int(n_log2(n_max(1.f, d / this->textureDistanceLodMax))));
}

//------------------------------------------------------------------------------
/**
    Updates the quad tree, must be called when the view matrix changes 
    before rendering.
*/
void
nChunkLodTree::Update(const vector3& viewPoint)
{
    // update user dependent parameters?
    if (this->paramsDirty)
    {
        this->UpdateParams();
    }

    if (!this->chunks[0].IsValid())
    {
        // get root node data
        this->chunks[0].RequestLoadData(this, 1.0f);
    }

    // clear the split status in the tree
    if (this->chunks[0].GetSplit())
    {
        this->chunks[0].ClearSplits();
    }

    // update geometry
    this->chunks[0].Update(this, viewPoint);

    // update texture (AFTER updating geometry!)
    this->chunks[0].UpdateTexture(this, viewPoint);
}

//------------------------------------------------------------------------------
/**
    Begin rendering, this resets the render stats.
*/
void
nChunkLodTree::BeginRender()
{
    this->numMeshesRendered = 0;
    this->numTexturesRendered = 0;
}

//------------------------------------------------------------------------------
/**
    Render the quadtree. Returns number of triangles rendered.
*/
int
nChunkLodTree::Render(nShader2* shader, const matrix44& projection, const matrix44& modelViewProj)
{
	if (shader->IsParameterUsed(nShaderState::ModelViewProjection))
    {
        shader->SetMatrix(nShaderState::ModelViewProjection, modelViewProj);
    }

	if (shader->IsParameterUsed(nShaderState::Projection))
    {
        shader->SetMatrix(nShaderState::Projection, projection);
    }
  
    // fill a render params structure
    nChunkLodRenderParams renderParams;
    renderParams.tree          = this;
    renderParams.gfxServer     = nGfxServer2::Instance();
    renderParams.shader        = shader;
    renderParams.modelViewProj = modelViewProj;
    renderParams.numMeshesRendered = this->numMeshesRendered;
    renderParams.numTexturesRendered = this->numTexturesRendered;

    int triCount = 0;
    if (this->chunks[0].IsValid())
    {
        triCount += this->chunks[0].Render(renderParams, bbox3::Clipped, false);
    }
    this->numMeshesRendered = renderParams.numMeshesRendered;
    this->numTexturesRendered = renderParams.numTexturesRendered;
    return triCount;
}

//------------------------------------------------------------------------------
/**
    Finish rendering.
*/
void
nChunkLodTree::EndRender()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Returns the texture which is responsible for rendering a given 
    chunk coordinate. May return a higher level texture, or even a null
    pointer if the right texture is not available yet.
*/
nTexture2*
nChunkLodTree::GetBaseLevelTexture(int col, int row, nFloat4& texGenS, nFloat4& texGenT)
{
    // find the chunk, and walk towards root node until
    // a valid texture is found
    int chunkIndex = this->GetChunkIndex(this->treeDepth - 1, col, row);
    nChunkLodNode* chunk = this->chunkTable[chunkIndex];
    while (chunk && (!chunk->HasValidTexture()))
    {
        chunk = chunk->GetParent();
    }
    if (chunk)
    {
        n_assert(chunk->HasValidTexture());
        chunk->GetTexGen(this, texGenS, texGenT);
        return chunk->GetTexture();
    }
    return 0;
}
