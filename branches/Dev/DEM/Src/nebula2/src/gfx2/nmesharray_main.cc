//------------------------------------------------------------------------------
//  nmesharray_main.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/nmesharray.h"
#include "kernel/nkernelserver.h"

nNebulaClass(nMeshArray, "nresource");

//------------------------------------------------------------------------------
/**
*/
nMeshArray::nMeshArray() :
    elements(nGfxServer2::MaxVertexStreams)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nMeshArray::~nMeshArray()
{
    if (!this->IsUnloaded())
    {
        this->Unload();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
nMeshArray::LoadResource()
{
    int i;
    bool success = true;
    for (i = 0; success && (i < nGfxServer2::MaxVertexStreams); i++)
    {
        Element& curElm = this->elements[i];
        if ((!curElm.filename.IsEmpty()))
        {
            if (curElm.refMesh.isvalid() &&
                ((curElm.refMesh->GetFilename() != curElm.filename) ||
                 (curElm.refMesh->GetUsage() != curElm.usage)))
            {
                //discharge previous set mesh - filename or usage is not equal
                curElm.refMesh->Release();
                curElm.refMesh.invalidate();
            }

            if (!curElm.refMesh.isvalid())
            {
                curElm.refMesh = nGfxServer2::Instance()->NewMesh(curElm.filename);
            }

            curElm.refMesh->SetFilename(curElm.filename);
            curElm.refMesh->SetUsage(curElm.usage);
            success &= curElm.refMesh->Load();
        }
    }
    this->SetState(Valid);
    return success;
}

//------------------------------------------------------------------------------
/**
*/
void
nMeshArray::UnloadResource()
{
    if (this == nGfxServer2::Instance()->GetMeshArray())
    {
        nGfxServer2::Instance()->SetMeshArray(0);
    }

    // unloads sub meshes
    int i;
    for (i = 0; i < nGfxServer2::MaxVertexStreams; i++)
    {
        Element& curElm = this->elements[i];
        if (curElm.refMesh.isvalid())
        {
            curElm.refMesh->Release();
            curElm.refMesh.invalidate();
        }
    }
    this->SetState(Unloaded);
}
