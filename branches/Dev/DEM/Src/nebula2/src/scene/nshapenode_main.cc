//------------------------------------------------------------------------------
//  nshapenode_main.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nshapenode.h"
#include "gfx2/nmesh2.h"
#include "gfx2/ngfxserver2.h"
#include <Render/RenderServer.h>
#include <Data/BinaryReader.h>

nNebulaClass(nShapeNode, "nmaterialnode");

bool nShapeNode::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'HSEM': // MESH
		{
			char Value[512];
			if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
			SetMesh(Value);

			Mesh = RenderSrv->MeshMgr.GetTypedResource(CStrID(Value));

			OK;
		}
		case 'RGSM': // MSGR
		{
			DataReader.Read(groupIndex);
			OK;
		}
		default: return nMaterialNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

void nShapeNode::UnloadMesh()
{
	Mesh = NULL;

	if (refMesh.isvalid())
	{
		refMesh->Release();
		refMesh.invalidate();
	}
}
//---------------------------------------------------------------------

// Load new mesh, release old one if valid. Also initializes the groupIndex member.
bool nShapeNode::LoadMesh()
{
    if (!refMesh.isvalid() && meshName.IsValid())
    {
        // append mesh usage to mesh resource name
        nString resourceName;
        resourceName.Format("%s_%d", meshName.Get(), meshUsage);

        // get a new or shared mesh
        nMesh2* mesh = nGfxServer2::Instance()->NewMesh(resourceName);
        n_assert(mesh);
        if (!mesh->IsLoaded())
        {
            mesh->SetFilename(meshName);
            mesh->SetUsage(meshUsage);

            if (!mesh->Load())
            {
                n_printf("nMeshNode: Error loading mesh '%s'\n", meshName.Get());
                mesh->Release();
                return false;
            }
        }
        refMesh = mesh;
        SetLocalBox(refMesh->Group(groupIndex).Box);
    }

	return true;
}
//---------------------------------------------------------------------

bool nShapeNode::LoadResources()
{
	return nMaterialNode::LoadResources() && LoadMesh();
}
//---------------------------------------------------------------------

void nShapeNode::UnloadResources()
{
	nMaterialNode::UnloadResources();
	UnloadMesh();
}
//---------------------------------------------------------------------

// Perform pre-instancing actions needed for rendering geometry. This is called once
// before multiple instances of this shape node are actually rendered.
bool nShapeNode::ApplyGeometry(nSceneServer* /*sceneServer*/)
{
	n_assert(refMesh->IsValid());
	nGfxServer2::Instance()->SetMesh(refMesh, refMesh);
	const nMeshGroup& curGroup = refMesh->Group(groupIndex);
	nGfxServer2::Instance()->SetVertexRange(curGroup.FirstVertex, curGroup.NumVertices);
	nGfxServer2::Instance()->SetIndexRange(curGroup.FirstIndex, curGroup.NumIndices);
	return true;
}
//---------------------------------------------------------------------

bool nShapeNode::RenderGeometry(nSceneServer* /*sceneServer*/, nRenderContext* /*renderContext*/)
{
	nGfxServer2::Instance()->DrawIndexedNS(nGfxServer2::TriangleList);
	return true;
}
//---------------------------------------------------------------------
