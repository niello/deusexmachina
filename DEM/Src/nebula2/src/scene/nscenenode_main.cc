#include "scene/nscenenode.h"
#include "scene/nrendercontext.h"
#include "gfx2/ngfxserver2.h"
#include <Data/BinaryReader.h>

nNebulaClass(nSceneNode, "nroot");

bool nSceneNode::LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader)
{
	switch (FourCC)
	{
		case 'XOBL': // LBOX
		{
			vector3 Mid, Ext;
			DataReader.Read(Mid);
			DataReader.Read(Ext);
			SetLocalBox(bbox3(Mid, Ext));
			OK;
		}
		case 'TNIH': // HINT
		{
			DataReader.Read<int>();
			OK;
		}
		default:
		{
			char FourCCStr[5];
			memcpy(FourCCStr, (char*)&FourCC, 4);
			FourCCStr[4] = 0;
			//n_printf("Processing object '%s', FOURCC '%s' failed\n", name.Get(), FourCCStr);
			n_message("Processing object '%s', FOURCC '%s' failed\n", name.Get(), FourCCStr);
			FAIL;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Recursively preload required resources. Call this method after loading
    or creation and before the first rendering. It will load all required
    resources (textures, meshes, animations, ...) from disk and thus
    prevent stuttering during rendering.
*/
void nSceneNode::PreloadResources()
{
    if (!AreResourcesValid()) LoadResources();
    for (nSceneNode* pCurrChild = (nSceneNode*)GetHead(); pCurrChild; pCurrChild = (nSceneNode*) pCurrChild->GetSucc())
        pCurrChild->PreloadResources();
}

//------------------------------------------------------------------------------
/**
    Called by the client app when a new render context has been created for
    this scene node hierarchy. Scene node hierarchies must not contain
    frame-persistent data, since one hierarchy can be reused multiple times
    per frame. All frame-persistent data must be stored in nRenderContext
    objects, which must be communicated to the scene node hierarchy when
    it is rendered. nRenderContext objects are the primary way to
    communicate data from the client app to a scene node hierarchy (i.e.
    time stamps, velocity, etc...).

    The RenderContextCreated() method should be called when a new
    'game object' which needs rendering has been created by the application.

    @param  renderContext   pointer to a nRenderContext object
*/
void nSceneNode::RenderContextCreated(nRenderContext* renderContext)
{
    n_assert(renderContext);
    for (nSceneNode* pCurrChild = (nSceneNode*)GetHead(); pCurrChild; pCurrChild = (nSceneNode*) pCurrChild->GetSucc())
        pCurrChild->RenderContextCreated(renderContext);
}

//------------------------------------------------------------------------------
/**
    Called by the client app when a render context for this scene node
    hierarchy should be destroyed. This is usually the case when the
    game object associated with this scene node hierarchy goes away.

    The method will be invoked recursively on all child and depend nodes
    of the scene node object.

    @param  renderContext   pointer to a nRenderContext object

    - 20-Jul-04     floh    oops, recursive routine was calling ClearLocalVars!
*/
void nSceneNode::RenderContextDestroyed(nRenderContext* renderContext)
{
    n_assert(renderContext);
    for (nSceneNode* pCurrChild = (nSceneNode*)GetHead(); pCurrChild; pCurrChild = (nSceneNode*) pCurrChild->GetSucc())
        pCurrChild->RenderContextDestroyed(renderContext);
}

//------------------------------------------------------------------------------
/**
    Attach the object to the scene if necessary. This method is either
    called by the nSceneServer, or by another nSceneNode object at
    scene construction time. If the nSceneNode needs rendering it should
    call the appropriate nSceneServer method to attach itself to the scene.

    The method will be invoked recursively on all child and depend nodes
    of the scene node object.

    @param  sceneServer     pointer to the nSceneServer object
    @param  renderContext   pointer to the nRenderContext object
*/
void nSceneNode::Attach(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    n_assert(sceneServer && renderContext);
    for (nSceneNode* pCurrChild = (nSceneNode*)GetHead(); pCurrChild; pCurrChild = (nSceneNode*) pCurrChild->GetSucc())
        pCurrChild->Attach(sceneServer, renderContext);
}
