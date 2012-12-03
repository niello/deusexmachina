#include "Scene.h"

#include <Scene/SceneServer.h>

namespace Scene
{

void CScene::Init(const bbox3& Bounds)
{
	RootNode = SceneSrv->CreateSceneNode(CStrID::Empty);
	RootNode->Flags.Set(CSceneNode::OwnedByScene);

	SceneBBox = Bounds;
	vector3 Center = SceneBBox.center();
	vector3 Size = SceneBBox.size();
	SPS.Build(Center.x, Center.z, Size.x, Size.z, 5); //???where to get depth?
}
//---------------------------------------------------------------------

void CScene::Activate()
{
}
//---------------------------------------------------------------------

void CScene::Deactivate()
{
}
//---------------------------------------------------------------------

// NB: Nodes owned by external objects and their hierarchy will persist until owner objects are destroyed
void CScene::Clear()
{
	for (int i = 0; i < OwnedNodes.Size(); ++i)
		OwnedNodes[i]->Flags.Clear(CSceneNode::OwnedByScene);
	OwnedNodes.Clear();
	RootNode->Flags.Clear(CSceneNode::OwnedByScene);
	RootNode = NULL;
}
//---------------------------------------------------------------------

//???or always do externally?
void CScene::CreateDefaultCamera()
{
	if (!RootNode.isvalid() || RootNode->GetChild(CStrID("_DefaultCamera")).isvalid()) return;

	PSceneNode CameraNode = RootNode->CreateChild(CStrID("_DefaultCamera"));
	OwnNode(CameraNode);
	PCamera Camera = n_new(CCamera);
	CameraNode->AddAttr(*Camera);
	CurrCamera = Camera;
}
//---------------------------------------------------------------------

bool CScene::Render(PCamera Camera, CStrID FrameShaderID)
{
	if (!Camera.isvalid()) Camera = CurrCamera;
	if (!Camera.isvalid())
	{
		VisibleMeshes.Clear();
		VisibleLights.Clear();
		FAIL;
	}

	// Get frame shader here, if ID is empty, use default frame shader

	//!!!FrameShader OPTIONS!
	bool FrameShaderUsesLights = true;
	//!!!filters (ShadowCasters etc)!

	const matrix44& ViewProj = Camera->GetViewProjMatrix();

	//!!!filter flags (from frame shader - or-sum of pass flags, each pass will check requirements inside itself)
	nArray<CLight*>* pVisibleLights = FrameShaderUsesLights ? &VisibleLights : NULL;
	SPSCollectVisibleObjects(SPS.GetRootNode(), ViewProj, &VisibleMeshes, pVisibleLights);

	// Run all passes

	// RT = Camera.GetRenderTarget() ? Camera.GetRenderTarget() : Pass.GetRenderTarget();
	// Renderer.SetRenderTarget(RT);

//!!!OPTIONAL! Some passes may not clear render target
	// Camera->ClearFlags (override)
	// Color = Camera.GetClearColor() ? Camera.GetClearColor() : Pass.GetClearColor();
	// Depth = Camera.GetClearDepth() ? Camera.GetClearDepth() : Pass.GetClearDepth();
	// Stencil = Camera.GetClearStencil() ? Camera.GetClearStencil() : Pass.GetClearStencil();
	// Renderer.ClearRenderTarget(Color, Depth, Stencil); //???or may be inside the renderer? set overrides from camera?

	// Renderer.SetView(Camera.GetView());
	// Renderer.SetProjection(Camera.GetProjection());

// SEPARATE PASS:
	// Pass may request occlusion culling after the depth buffer is filled
	//???Occlusion culling as separate pass, that marks shapes and lights as occluded?

// SEPARATE PASS:
	// Pass may request shadow rendering (recurse with camera from light, fill shadow texture)
	// The best way is to dedicate a separate pass for shadow rendering, and in later passes use

// Dependent cameras:
	// Some shapes may request textures that are RTs of specific cameras
	// These textures must be rendered before shapes are rendered
	// Good way is to collect all cameras and recurse, filling required textures
	// Pass may disable recursing into cameras to prevent infinite recursion
	// Generally, only FrameBuffer pass should recurse
	// Camera shouldn't render its texture more than once per frame (in won't change)!
	//???as "RenderDependentCameras" flag in Camera/Pass? maybe even filter by dependent camera type
	// Collect - check all meshes to be rendered, check all their textures, select textures rendered from camera RTs
	// Non-mesh dependent textures (shadows etc) must be rendered in one of the previous passes

// Somewhere at here scene ends and renderer begins:

	// Shapes are sorted by shader, by distance (None, FtB or BtF, depending on shader requirements),
	// by geometry (for the instancing), may be by lights that affect them
	// For the front-to-back sorting, can sort once on first request, always FtB, and when BtF is needed,
	// iterate through the array from the end reversely

	// For each shader (batch):
	// Shader is set
	// Light params are updated, if it is light pass
	// Shader params are updated
	// Shapes are rendered, instanced, when possible

//!!!NOTE: meshes visible from the light's camera are the meshes in light's range!
// can avoid collecting visible meshes on shadow pass
// can render non-shadow-receiving objects without shadow mapping (another technique)
// can render to SM only casters with extruded shadow box visible from main camera

// Rendering must be like this:
	// - begin frame shader
	// - Renderer: apply frame shader constant render states
	// - Renderer: set View and Projection
	// - determine visible meshes [and lights, if lighting is enabled ?in one of passes?]
	// - for each pass, render scene pass, occlusion pass, shadow pass (for shadow-casting lights) or posteffect pass
	// - end frame shader
	//
	// Scene pass:
	// - begin pass
	// - if pass renders dependent textures
	//   - collect them and their cameras (check if already up-to-date this frame)
	//   - if recurse to rendering call with another camera and frame shader
	//   - now our textures from cameras are ready to use
	// - Renderer: set RT
	// - Renderer: optionally clear RT
	// - Renderer: apply pass shader (divide this pass to some phases/batches like opaque, atest, alpha etc inside, mb before a renderer...)
	// - Renderer: set pass technique
	// - Renderer: pass meshes [and lights]
	// - Renderer: render to RT
	//    (link meshes and lights(here?), sort meshes, batch instances, select lighting code,
	//     set shared state of instance sets)
	// - end pass
	//
	// Occlusion pass:
	// - begin pass
	// - issue occlusion query for AABBs of all visible meshes
	// - remove occluded shapes [and lights] from visible shape [and light] array[s]
	// - end pass
	//
	// Shadow pass:
	// - begin pass
	// - select shadow-casting light(s)
	// - instead of collecting meshes visible from the light's camera, we collect ones in the light range
	// - for directional lights and PSSM this process will differ, may be extruded shadow boxes will be necessary
	// - collect ONLY shadow casters
	// - do not modify the main mesh list
	// - note: some caster meshes invisible from the main camera can have visible shadows. Anyway they are
	//         casting shadows on all visible receivers as their shadows are rendered to shadow buffer texture
	// - render casters to the shadow buffer texture
	// - end pass
	//
	// Posteffect path:
	// - begin pass
	// - Renderer: set posteffect shader
	// - Renderer: set source textures and other params
	// - Renderer: render fullscreen quad
	// - end pass
	//
	//
	// After this some UI, text and debug shapes can be rendered
	// Nebula treats all them as different batches or render plugins, it is good idea maybe...
	// Then backbuffer is present

	VisibleMeshes.Clear();
	VisibleLights.Clear();

	OK;
}
//---------------------------------------------------------------------

void CScene::SPSCollectVisibleObjects(CSPSNode* pNode, const matrix44& ViewProj,
									  nArray<CMesh*>* OutMeshes, nArray<CLight*>* OutLights,
									  EClipStatus Clip)
{
	if (!pNode->GetTotalObjCount() || (!OutMeshes && !OutLights)) return;

	if (Clip == Clipped)
	{
		bbox3 NodeBox;
		pNode->GetBounds(NodeBox);
		NodeBox.vmin.y = SceneBBox.vmin.y;
		NodeBox.vmax.y = SceneBBox.vmax.y;
		Clip = NodeBox.clipstatus(ViewProj);
		if (Clip == Outside) return;
	}

	if (OutMeshes && pNode->Data.Meshes.Size())
	{
		nArray<CSPSRecord>::iterator ItMesh = pNode->Data.Meshes.Begin();
		if (Clip == Inside)
		{
			CMesh** ppMesh = OutMeshes->Reserve(pNode->Data.Meshes.Size());
			for (; ItMesh != pNode->Data.Meshes.End(); ++ItMesh, ++ppMesh)
				*ppMesh = (CMesh*)ItMesh->pAttr;
		}
		else // Clipped
		{
			//???test against global box or transform to model space and test against local box?
			for (; ItMesh != pNode->Data.Meshes.End(); ++ItMesh)
				if (ItMesh->GlobalBox.clipstatus(ViewProj) != Outside)
					OutMeshes->Append((CMesh*)ItMesh->pAttr);
		}
	}

	if (OutLights && pNode->Data.Lights.Size())
	{
		nArray<CSPSRecord>::iterator ItLight = pNode->Data.Lights.Begin();
		if (Clip == Inside)
		{
			CLight** ppLight = OutLights->Reserve(pNode->Data.Lights.Size());
			for (; ItLight != pNode->Data.Lights.End(); ++ItLight, ++ppLight)
				*ppLight = (CLight*)ItLight->pAttr;
		}
		else // Clipped
		{
			//???test against global box or transform to model space and test against local box?
			for (; ItLight != pNode->Data.Lights.End(); ++ItLight)
				if (ItLight->GlobalBox.clipstatus(ViewProj) != Outside)
					OutLights->Append((CLight*)ItLight->pAttr);
		}
	}

	if (pNode->HasChildren())
		for (DWORD i = 0; i < 4; i++)
			SPSCollectVisibleObjects(pNode->GetChild(i), ViewProj, OutMeshes, OutLights, Clip);
}
//---------------------------------------------------------------------

}