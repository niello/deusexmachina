//------------------------------------------------------------------------------
//  nmesh2_main.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/nmesh2.h"
#include "kernel/nkernelserver.h"
#include "gfx2/nn3d2loader.h"
#include "gfx2/nnvx2loader.h"

nNebulaClass(nMesh2, "nresource");

#ifndef NGAME
bool nMesh2::optimizeMesh = false;
#endif //NGAME

//------------------------------------------------------------------------------
/**
*/
nMesh2::nMesh2() :
    vertexUsage(WriteOnce),
    indexUsage(WriteOnce),
    vertexComponentMask(0),
    vertexWidth(0),
    numVertices(0),
    numIndices(0),
    numGroups(0),
    groups(0),
    vertexBufferByteSize(0),
    indexBufferByteSize(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Unload everything.
*/
void
nMesh2::UnloadResource()
{
    n_assert(Unloaded != this->GetState());

    // make sure we're not set in the gfx server
    // this is kind of rude...
    nGfxServer2::Instance()->SetMesh(0, 0);

    if (this->groups)
    {
        n_delete_array(this->groups);
        this->groups = 0;
    }

    // NOTE: do not clear numVertices, numIndices, vertexWidth,
    // etc. Those values may be needed in a following call to Load()!
    this->vertexBufferByteSize = 0;
    this->indexBufferByteSize  = 0;

	this->SetState(Unloaded);
}

//------------------------------------------------------------------------------
/**
    This method is either called directly from the nResource::Load() method
    (in synchronous mode), or from the loader thread (in asynchronous mode).
    The method must try to validate its resources, set the valid and pending
    flags, and return a success code.
    This method may be called from a thread.
*/
bool
nMesh2::LoadResource()
{
    n_assert(Unloaded == this->GetState());

    nString filename(this->GetFilename());
    
	n_assert(filename.IsValid());

	nMeshLoader* meshLoader = NULL;

    // select mesh loader
    if (filename.CheckExtension("nvx2")) meshLoader = n_new(nNvx2Loader);
    else if (filename.CheckExtension("n3d2")) meshLoader = n_new(nN3d2Loader);
    else n_error("nMesh2::LoadResource: filetype not supported!\n");

    if (!meshLoader) return false;

	bool success = LoadFile(meshLoader);

	if (success) SetState(Valid);

    n_delete(meshLoader);
	return success;
}

//------------------------------------------------------------------------------

bool nMesh2::CreateNew(DWORD VtxCount, DWORD IdxCount, int Usage, int VtxComponents)
{
    SetUsage(Usage);
    SetNumVertices(VtxCount);
    SetNumIndices(IdxCount);
    SetVertexComponents(VtxComponents);

	if (GetNumVertices() > 0)
	{
		int verticesByteSize = GetNumVertices() * GetVertexWidth() * sizeof(float);
		SetVertexBufferByteSize(verticesByteSize);
		CreateVertexBuffer();
	}

	if (this->GetNumIndices() > 0)
	{
		int indicesByteSize = GetNumIndices() * sizeof(ushort);
		SetIndexBufferByteSize(indicesByteSize);
		CreateIndexBuffer();
	}

	SetState(Valid);
	return true;
}
//---------------------------------------------------------------------

/**
    Use the provided nMeshLoader class to read the data.

    @param meshLoader   the meshloader to load the file.
*/
bool
nMesh2::LoadFile(nMeshLoader* meshLoader)
{
    n_assert(meshLoader);
    n_assert(this->IsUnloaded());

    // configure a mesh loader and load header
    meshLoader->SetFilename(this->GetFilename());
    meshLoader->SetIndexType(nMeshLoader::Index16);
    if (!meshLoader->Open())
    {
        n_error("nMesh2: could not open file '%s'!\n", this->GetFilename().Get());
        return false;
    }

    // transfer header data
    this->SetNumGroups(meshLoader->GetNumGroups());
    this->SetNumVertices(meshLoader->GetNumVertices());
    this->SetVertexComponents(meshLoader->GetVertexComponents());
    this->SetNumIndices(meshLoader->GetNumIndices());

    int groupIndex;
    int numGroups = meshLoader->GetNumGroups();
    for (groupIndex = 0; groupIndex < numGroups; groupIndex++)
    {
        nMeshGroup& group = this->Group(groupIndex);
        group = meshLoader->GetGroupAt(groupIndex);
    }
    n_assert(this->GetVertexWidth() == meshLoader->GetVertexWidth());

    // allocate vertex and index buffers
    int vbSize = meshLoader->GetNumVertices() * meshLoader->GetVertexWidth() * sizeof(float);
    int ibSize = meshLoader->GetNumIndices() * sizeof(ushort);
    this->SetVertexBufferByteSize(vbSize);
    this->SetIndexBufferByteSize(ibSize);
    this->CreateVertexBuffer();
    this->CreateIndexBuffer();

    // load vertex buffer and index buffer
    float* vertexBufferPtr = this->LockVertices();
    ushort* indexBufferPtr = this->LockIndices();
    bool res = meshLoader->ReadVertices(vertexBufferPtr, vbSize);
    n_assert(res);
    res = meshLoader->ReadIndices(indexBufferPtr, ibSize);
    n_assert(res);
    this->UpdateGroupBoundingBoxes(vertexBufferPtr, indexBufferPtr);

    #ifndef NGAME
    if (optimizeMesh)
    #endif
    {
        if (GetUsage() == WriteOnce)
            OptimizeMesh(nMesh2::AllOptimizations, vertexBufferPtr, this->GetNumVertices(), indexBufferPtr, this->GetNumIndices());
    }

    this->UnlockIndices();
    this->UnlockVertices();

    // close the mesh loader
    meshLoader->Close();

    return true;
}

//------------------------------------------------------------------------------
/**
    Update the group bounding boxes. This is a slow operation (since the
    vertex buffer must read). It should only be called once after loading.
    NOTE that the vertex and index buffer must be locked while calling
    this method!
*/
void
nMesh2::UpdateGroupBoundingBoxes(float* vertexBufferData, ushort* indexBufferData)
{
    bbox3 groupBox;
    int groupIndex;
    for (groupIndex = 0; groupIndex < this->numGroups; groupIndex++)
    {
        groupBox.begin_extend();
        nMeshGroup& group = this->Group(groupIndex);
        ushort* indexPointer = indexBufferData + group.FirstIndex;
        for (int i = 0; i < group.NumIndices; i++)
        {
            float* vertexPointer = vertexBufferData + (indexPointer[i] * this->vertexWidth);
            groupBox.extend(vertexPointer[0], vertexPointer[1], vertexPointer[2]);
        }
        group.Box = groupBox;
    }
}
