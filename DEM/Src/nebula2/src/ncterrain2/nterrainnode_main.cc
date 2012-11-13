//------------------------------------------------------------------------------
//  nterrainnode_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "ncterrain2/nterrainnode.h"
#include "gfx2/ngfxserver2.h"
#include "resource/nresourceserver.h"
#include "scene/nrendercontext.h"
#include <Data/BinaryReader.h>

nNebulaClass(nTerrainNode, "nmaterialnode");

bool nTerrainNode::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'UHCF': // FCHU
		{
			char Value[512];
			if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
			ChunkFileName = Value;
			OK;
		}
		case 'TQTF': // FTQT
		{
			char Value[512];
			if (!DataReader.ReadString(Value, sizeof(Value))) FAIL;
			TQTFileName = Value;
			OK;
		}
		case 'REPM': // MPER
		{
			SetMaxPixelError(DataReader.Read<float>());
			OK;
		}
		case 'ZSTM': // MTSZ
		{
			SetMaxTexelSize(DataReader.Read<float>());
			OK;
		}
		case 'LCST': // TSCL
		{
			return DataReader.Read<float>(TerrainScale);
		}
		case 'IROT': // TORI
		{
			return DataReader.Read<vector3>(TerrainOrigin);
		}
		default: return nMaterialNode::LoadDataBlock(FourCC, DataReader);
	}
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Load resources needed by this object.
*/
bool
nTerrainNode::LoadResources()
{
    if (nMaterialNode::LoadResources())
    {
        if ((!this->refChunkLodTree.isvalid()) && (!this->ChunkFileName.IsEmpty()))
        {
            nChunkLodTree* tree = (nChunkLodTree*) nResourceServer::Instance()->NewResource("nchunklodtree", 0, nResource::Other);
            n_assert(tree);
            if (!tree->IsValid())
            {
                tree->SetFilename(this->ChunkFileName);
                tree->SetTqtFilename(this->TQTFileName.Get());
                tree->SetDisplayMode(nGfxServer2::Instance()->GetDisplayMode());
                tree->SetMaxPixelError(this->maxPixelError);
                tree->SetMaxTexelSize(this->maxTexelSize);
                tree->SetTerrainScale(this->TerrainScale);
                tree->SetTerrainOrigin(this->TerrainOrigin);
                if (!tree->Load())
                {
                    n_printf("nTerrainNode: Error loading .chu file %s\n", this->ChunkFileName.Get());
                    tree->Release();
                    return false;
                }
            }
            this->refChunkLodTree = tree;
            this->SetLocalBox(tree->GetBoundingBox());
        }
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Unload resources.
*/
void
nTerrainNode::UnloadResources()
{
    nMaterialNode::UnloadResources();
    if (this->refChunkLodTree.isvalid())
    {
        this->refChunkLodTree->Release();
        this->refChunkLodTree.invalidate();
    }
}
//---------------------------------------------------------------------

void nTerrainNode::RenderContextCreated(nRenderContext* renderContext)
{
	nMaterialNode::RenderContextCreated(renderContext);
	renderContext->SetFlag(nRenderContext::DoOcclusionQuery, false);
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Update geometry, set as current mesh in the gfx server and
    call nGfxServer2::DrawIndexed().

    - 15-Jan-04     floh    AreResourcesValid()/LoadResource() moved to scene server
*/
bool
nTerrainNode::RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    n_assert(sceneServer);
    n_assert(renderContext);

    //nWatched watchNumMeshesRendered("terrainNumMeshesRendered", DATA_TYPE(int));
    //nWatched watchNumTexturesRendered("terrainNumTexturesRendered", DATA_TYPE(int));
    //nWatched watchNumMeshesAllocated("terrainNumMeshesAllocated", DATA_TYPE(int));
    //nWatched watchNumTexturesAllocated("terrainNumTexturesAllocated", DATA_TYPE(int));

    // only render if resource is available (may not be 
    // available yet if async resource loading enabled)
    if (!this->refChunkLodTree->IsValid())
    {
        return false;
    }

    // update and render the chunk lod tree
    const matrix44& view = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvView);
    this->refChunkLodTree->Update(view.pos_component());

    // render in 2 passes, far and near because of limited z buffer precision
    const float farZ = 160000.0f;
    nCamera2 camera = nGfxServer2::Instance()->GetCamera();
    nCamera2 origCamera = camera;

    // render far pass and near pass with different near/far planes to
    // workaround zbuffer precision problems
    nChunkLodTree* chunkLodTree = this->refChunkLodTree.get();
    chunkLodTree->BeginRender();
    camera.SetNearPlane(origCamera.GetFarPlane() * 0.9f);
    camera.SetFarPlane(farZ);
    nGfxServer2::Instance()->SetCamera(camera);
    chunkLodTree->Render(nGfxServer2::Instance()->GetShader(), nGfxServer2::Instance()->GetTransform(nGfxServer2::Projection), nGfxServer2::Instance()->GetTransform(nGfxServer2::ModelViewProjection));

    nGfxServer2::Instance()->Clear(nGfxServer2::DepthBuffer, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0);
    camera.SetNearPlane(origCamera.GetNearPlane());
    camera.SetFarPlane(origCamera.GetFarPlane());
    nGfxServer2::Instance()->SetCamera(camera);
    chunkLodTree->Render(nGfxServer2::Instance()->GetShader(), nGfxServer2::Instance()->GetTransform(nGfxServer2::Projection), nGfxServer2::Instance()->GetTransform(nGfxServer2::ModelViewProjection));
    chunkLodTree->EndRender();

    //watchNumMeshesRendered->SetValue(chunkLodTree->GetNumMeshesRendered());
    //watchNumTexturesRendered->SetValue(chunkLodTree->GetNumTexturesRendered());
    //watchNumMeshesAllocated->SetValue(chunkLodTree->GetNumMeshesAllocated());
    //watchNumTexturesAllocated->SetValue(chunkLodTree->GetNumTexturesAllocated());

    nGfxServer2::Instance()->SetCamera(origCamera);
    return true;
}
