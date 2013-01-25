#include "GfxServer.h"

#include <Gfx/SceneResource.h>
#include <Gfx/CameraEntity.h>
#include <Gfx/ShapeEntity.h>
#include <Data/DataServer.h>
#include <resource/nresourceserver.h>
#include <scene/ntransformnode.h>
#include <gfx2/nd3d9server.h>
#include <scene/nsceneserver.h>
#include <variable/nvariableserver.h>

namespace Load
{
	bool StringTableFromExcelXML(const nString& FileName, Data::CTTable<nString>& Out,
								 LPCSTR pWorksheetName, bool FirstRowAsColNames, bool FirstColAsRowNames);
}

namespace Graphics
{
ImplementRTTI(CGfxServer, Core::CRefCounted);
__ImplementSingleton(CGfxServer);

const float CGfxServer::MaxLODDistThreshold = 10000.0f;
const float CGfxServer::MinLODSizeThreshold = 0.0f;

CGfxServer::CGfxServer(): _IsOpen(false), FrameID(0)
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CGfxServer::~CGfxServer()
{
	n_assert(!_IsOpen && !CurrLevel.isvalid());
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool ConvertStringToAnimInfoArray(const nString& In, nArray<CAnimInfo>& Out)
{
	// In is either just the name of an animation or a string of the form
	// attr=value; attr=value
	nArray<nString> Tokens;
	In.Tokenize(";", Tokens);

	int AnimCounter = 0;

	for (int i = 0; i < Tokens.Size(); i++)
	{
		nArray<nString> AttrValueTokens;
		Tokens[i].Tokenize("= \t", AttrValueTokens);
		if (AttrValueTokens.Size() > 1)
		{
			if (AttrValueTokens[0] == "anim")
			{
				n_assert(AttrValueTokens[1].IsValid());

				CAnimInfo& Info = Out.At(AnimCounter);
				Info.Name = AttrValueTokens[1];

				// Check if next token has a hot spot for this animation
				if (Tokens.Size() > (i + 1))
				{
					const nString& NextToken = Tokens.At(i + 1);
					nArray<nString> NextAttrValueTokens;
					NextToken.Tokenize("= \t", NextAttrValueTokens);
					if (NextAttrValueTokens[0] == "hotspot")
					{
						n_assert(NextAttrValueTokens[1].IsValid());
						Info.HotSpotTime = NextAttrValueTokens[1].AsFloat();
						n_assert(Info.HotSpotTime >= 0.0f);
						i++;
					}
				}

				AnimCounter++;
			}
			/*else if (AttrValueTokens[0] == "hotspot")
			{
				AnimInfo.At(ColIdx, RowIdx).HotSpotTime = AttrValueTokens[1].AsFloat();
			}*/
			else
			{
				n_printf("Invalid keyword in anim table: '%s'\n", In.Get());
				FAIL;
			}
		}
		else
		{
			// Old style string, just the animation name
			n_assert(Tokens[i].IsValid());
			Out.At(0).Name = Tokens[i];
		}
	}

	OK;
}
//---------------------------------------------------------------------

// Initialize the graphics subsystem and open the display.
bool CGfxServer::Open()
{
	n_assert(!_IsOpen);

	_IsOpen = true;

	gfxServer = n_new(nD3D9Server);
	gfxServer->AddRef();
	sceneServer = n_new(nSceneServer);
	sceneServer->AddRef();
	variableServer = n_new(nVariableServer);
	variableServer->AddRef();

	GfxRoot = nKernelServer::Instance()->New("nroot", "/res/gfx");

	if (FeatureSet.IsValid())
		nGfxServer2::Instance()->SetFeatureSetOverride(nGfxServer2::StringToFeatureSet(FeatureSet.Get()));

	if (RenderPath.IsValid()) nSceneServer::Instance()->SetRenderPathFilename(RenderPath);
	else nSceneServer::Instance()->SetRenderPathFilename("data:shaders/dx9hdr_renderpath.xml");

	// Open the scene server (will also open the display server)
	nSceneServer::Instance()->SetObeyLightLinks(true);
	if (!nSceneServer::Instance()->Open())
	{
		n_error("CGfxServer::Startup: Failed to open nSceneServer!");
		FAIL;
	}

	if (DataSrv->FileExists("data:tables/anims.xml"))
	{
		Data::CTTable<nString> TmpTable;
		if (!Load::StringTableFromExcelXML("data:tables/anims.xml", TmpTable, NULL, true, true)) FAIL;
		n_assert(TmpTable.GetColumnCount() >= 1 && TmpTable.GetRowCount() >= 1);
		n_assert(TmpTable.Convert(AnimTable, ConvertStringToAnimInfoArray));
	}
	else n_printf("CGfxServer::Open(): Warning, data:tables/anims.xml doesn't exist!\n");

	OK;
}
//---------------------------------------------------------------------

void CGfxServer::Close()
{
	n_assert(_IsOpen);

	nSceneServer::Instance()->Close();
	SetLevel(NULL);

	GfxRoot->Release();

	if (variableServer.isvalid()) variableServer->Release();
	if (sceneServer.isvalid()) sceneServer->Release();
	if (gfxServer.isvalid()) gfxServer->Release();

	n_assert(!GfxRoot.isvalid());
	_IsOpen = false;
}
//---------------------------------------------------------------------

// Trigger the graphics subsystem. This will not perform any rendering,
// instead, the Windows message pump will be "pumped" in order to
// process any outstanding window system messages.
void CGfxServer::Trigger()
{
	nGfxServer2::Instance()->Trigger();
}
//---------------------------------------------------------------------

bool CGfxServer::BeginRender()
{
	n_assert(_IsOpen);
	if (!CurrLevel.isvalid()) FAIL;

	// Don't render if currently playing video or display GDI dialog boxes
	if (nGfxServer2::Instance()->InDialogBoxMode()) FAIL;

	++FrameID;
	return CurrLevel->BeginRender();
}
//---------------------------------------------------------------------

// Render the current frame. This doesn't make the frame visible. You can do additional rendering after
// Render() returns. Finalize rendering by calling EndRender(), this will also make the frame visible.
void CGfxServer::Render()
{
	CurrLevel->Render();
}
//---------------------------------------------------------------------

void CGfxServer::RenderDebug()
{
	CurrLevel->RenderDebug();
}
//---------------------------------------------------------------------

// Finish rendering and present the scene.
void CGfxServer::EndRender()
{
	//CurrLevel->EndRender();
	nSceneServer::Instance()->PresentScene();
}
//---------------------------------------------------------------------

void CGfxServer::DragDropSelect(const vector3& lookAt, float angleOfView, float aspectRatio, nArray<Ptr<CEntity> >& entities)
{
	if (CurrLevel.isvalid())
	{
		CCameraEntity* cameraEntity = CurrLevel->GetCamera();
		matrix44 transform = cameraEntity->GetTransform();
		transform.lookatRh(lookAt, vector3(0.0f, 1.0f, 0.0f));
		nCamera2 camera = cameraEntity->GetCamera();
		camera.SetAngleOfView(angleOfView);
		camera.SetAspectRatio(aspectRatio);

		Ptr<CCameraEntity> dragDropCameraEntity = CCameraEntity::Create();
		dragDropCameraEntity->SetTransform(transform);
		dragDropCameraEntity->SetCamera(camera);

		//!!!uncomment & rewrite after new quadtree start working!

		//CurrLevel->GetRootCell()->ClearLinks(CEntity::PickupLink);
		//CurrLevel->GetRootCell()->UpdateLinks(dragDropCameraEntity, CEntity::GFXShape, CEntity::PickupLink);
		//for (int i = 0; i < dragDropCameraEntity->GetNumLinks(CEntity::PickupLink); i++)
		//	entities.PushBack(dragDropCameraEntity->GetLinkAt(CEntity::PickupLink, i));
	}
}
//---------------------------------------------------------------------

void CGfxServer::CreateGfxEntities(const nString& RsrcName, const matrix44& World, nArray<PShapeEntity>& OutEntities)
{
	n_assert(CurrLevel);

	PSceneResource Rsrc = CSceneResource::Create();
	Rsrc->Name = RsrcName;
	Rsrc->Load();

	nTransformNode* pModelNode = (nTransformNode*)Rsrc->GetNode()->Find("model");
	if (pModelNode)
	{
		n_assert(pModelNode);
		nArray<nRef<nTransformNode>> Segments;
		nClass* pTfmNodeClass = nKernelServer::Instance()->FindClass("ntransformnode");
		n_assert(pTfmNodeClass);
		nTransformNode* pCurrNode = (nTransformNode*)pModelNode->GetHead();
		for (; pCurrNode; pCurrNode = (nTransformNode*)pCurrNode->GetSucc())
			if (pCurrNode->IsA(pTfmNodeClass) && false) // && pCurrNode->HasHints(nSceneNode::LevelSegment))
				Segments.Append(pCurrNode);

		if (Segments.Size() > 0)
		{
			nTransformNode* pShadowNode = (nTransformNode*)Rsrc->GetNode()->Find("shadow");

			// Create one graphics entity for each segment
			for (int i = 0; i < Segments.Size(); i++)
			{
                nString SegmentRsrcName = RsrcName;
                SegmentRsrcName.TrimRight("/");
                SegmentRsrcName.Append("/model/");
                SegmentRsrcName.Append(Segments[i]->GetName());

				PShapeEntity GfxEnt = CShapeEntity::Create();
                GfxEnt->SetResourceName(SegmentRsrcName);
                GfxEnt->SetTransform(World);

                if (pShadowNode && pShadowNode->Find(Segments[i]->GetName()))
                    GfxEnt->SetShadowResourceName(RsrcName + "/shadow/" + Segments[i]->GetName());

				OutEntities.Append(GfxEnt);
			}
		}
	}

	// Fallthrough: don't create Segments
	PShapeEntity GfxEnt = CShapeEntity::Create();
	GfxEnt->SetResourceName(RsrcName);
	GfxEnt->SetTransform(World);
	OutEntities.Append(GfxEnt);
}
//---------------------------------------------------------------------

const CAnimInfo& CGfxServer::GetAnimationName(const nString& AnimSet, const nString& AnimName, bool Random)
{
	nArray<CAnimInfo>& AnimInfo = AnimTable.Cell(CStrID(AnimSet.Get()), CStrID(AnimName.Get()));
	return AnimInfo.At(Random ? (rand() % AnimInfo.Size()) : 0);	
}
//---------------------------------------------------------------------

void CGfxServer::GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const
{
	nD3D9Server::Instance()->GetRelativeXY(XAbs, YAbs, XRel, YRel);
}
//---------------------------------------------------------------------

} // namespace Graphics
