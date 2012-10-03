//------------------------------------------------------------------------------
//  nscenenode_main.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nscenenode.h"
#include "scene/nanimator.h"
#include "scene/nrendercontext.h"
#include "gfx2/ngfxserver2.h"
#include <Data/BinaryReader.h>

nNebulaClass(nSceneNode, "nroot");

//------------------------------------------------------------------------------
/**
*/
nSceneNode::nSceneNode() :
    animatorArray(1, 4),
    resourcesValid(false),
    renderPri(0),
    hints(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    This calls UnloadResources() if the object is going to die (this can't
    be put into the destructor, because virtual methods don't work when
    called from the destructor).
*/
bool nSceneNode::Release()
{
    if (RefCount == 1) UnloadResources();
    return nRoot::Release();
}

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
			AddHints(DataReader.Read<int>());
			OK;
		}
		default:
		{
			char FourCCStr[5];
			memcpy(FourCCStr, (char*)&FourCC, 4);
			FourCCStr[4] = 0;
			n_printf("Processing object '%s', FOURCC '%s' failed\n", name.Get(), FourCCStr);
			FAIL;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    This method makes sure that all resources needed by this object
    are loaded. The method does NOT recurse into its children.

    Subclasses should expect that the LoadResources() method can be
    called on them although some or all of their resources are valid.
    Thus, a check should exist, whether the resource really needs to
    be reloaded.

    @return     true, if resource loading was successful
*/
bool
nSceneNode::LoadResources()
{
#ifdef _DEBUG
    // char buf[N_MAXPATH];
    // n_printf("-> Loading resources for scene node '%s'\n", this->GetFullName(buf, sizeof(buf)));
#endif
    this->resourcesValid = true;
    return true;
}

//------------------------------------------------------------------------------
/**
    This method makes sure that all resources used by this object are
    unloaded. The method does NOT recurse into its children.

    If you ovverride this method, be sure to call the overridden version
    in your destructor.

    @return     true, if resources have actually been unloaded
*/
void
nSceneNode::UnloadResources()
{
#ifdef _DEBUG
    // char buf[N_MAXPATH];
    // n_printf("-> Unloading resources for scene node '%s'\n", this->GetFullName(buf, sizeof(buf)));
#endif
    /*
    if (this->refInstanceStream.isvalid())
    {
        this->refInstanceStream->Release();
        n_assert(!this->refInstanceStream.isvalid());
    }
    */
    this->resourcesValid = false;
}

//------------------------------------------------------------------------------
/**
    Recursively preload required resources. Call this method after loading
    or creation and before the first rendering. It will load all required
    resources (textures, meshes, animations, ...) from disk and thus
    prevent stuttering during rendering.
*/
void
nSceneNode::PreloadResources()
{
    if (!AreResourcesValid()) LoadResources();

    // recurse...
    nSceneNode* curChild;
    for (curChild = (nSceneNode*) this->GetHead();
         curChild;
         curChild = (nSceneNode*) curChild->GetSucc())
    {
        curChild->PreloadResources();
    }
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
void
nSceneNode::RenderContextCreated(nRenderContext* renderContext)
{
    n_assert(renderContext);

    nSceneNode* curChild;
    for (curChild = (nSceneNode*) this->GetHead();
         curChild;
         curChild = (nSceneNode*) curChild->GetSucc())
    {
        curChild->RenderContextCreated(renderContext);
    }
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
void
nSceneNode::RenderContextDestroyed(nRenderContext* renderContext)
{
    n_assert(renderContext);

    nSceneNode* curChild;
    for (curChild = (nSceneNode*) this->GetHead();
         curChild;
         curChild = (nSceneNode*) curChild->GetSucc())
    {
        curChild->RenderContextDestroyed(renderContext);
    }
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
void
nSceneNode::Attach(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    n_assert(sceneServer);
    n_assert(renderContext);

    nSceneNode* curChild;
    for (curChild = (nSceneNode*) this->GetHead();
         curChild;
         curChild = (nSceneNode*) curChild->GetSucc())
    {
        curChild->Attach(sceneServer, renderContext);
    }
}

//------------------------------------------------------------------------------
/**
    Render the node's transformtion. This should be implemented by a subclass.
    The method will only be called by nSceneServer if the method
    HasTransform() returns true.
*/
bool
nSceneNode::RenderTransform(nSceneServer* /*sceneServer*/, nRenderContext* /*renderContext*/, const matrix44& /*parentMatrix*/)
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Perform pre-instance rendering of geometry. This method will be
    called once at the beginning of rendering different instances
    of the same scene node. Use this method to setup geometry attributes
    which are constant for a complete instance set.
*/
bool
nSceneNode::ApplyGeometry(nSceneServer* /*sceneServer*/)
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Perform per-instance-rendering of geometry. This method will be
    called after ApplyGeometry() once for each instance of the
    node.
*/
bool
nSceneNode::RenderGeometry(nSceneServer* /*sceneServer*/, nRenderContext* /*renderContext*/)
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Perform debug-rendering. This method will be called by nSceneServer
    on each shape node right after RenderGeometry() if debug visualization
    is enabled.
*/
void
nSceneNode::RenderDebug(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& modelMatrix)
{
    // override in subclass
}

//------------------------------------------------------------------------------
/**
    Perform pre-instance rendering of the shader. This method will be
    called once at the beginning of rendering different instances
    of the same scene node. Use this method to setup shader attributes
    which are constant for a complete instance set.
*/
bool
nSceneNode::ApplyShader(nSceneServer* /*sceneServer*/)
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Perform per-instance-rendering of the shader. This method will
    be called after ApplyShader() once for each rendered instance.
    Use this method to set shader attributes which vary from instance
    to instance.
*/
bool
nSceneNode::RenderShader(nSceneServer* /*sceneServer*/, nRenderContext* /*renderContext*/)
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Perform per-light of the light source. This method will
    be called for each light.
*/
const nLight&
nSceneNode::ApplyLight(nSceneServer* /*sceneServer*/, nRenderContext* /*renderContext*/, const matrix44& /*lightTransform*/, const vector4& /*shadowLightIndex*/)
{
    static nLight staticLight;
    return staticLight;
}

//------------------------------------------------------------------------------
/**
    Perform per-instance-rendering of the light source. This method will
    be called once for each scene node which is influenced by this light.
*/
const nLight&
nSceneNode::RenderLight(nSceneServer* /*sceneServer*/, nRenderContext* /*renderContext*/, const matrix44& /*lightTransform*/)
{
    static nLight staticLight;
    return staticLight;
}

//------------------------------------------------------------------------------
/**
    Perform pre-instance rendering of shadow. This method will be
    called once at the beginning of rendering different instances
    of the same scene node. Use this method to setup shadow geometry attributes
    which are constant for a complete instance set.
*/
bool
nSceneNode::ApplyShadow(nSceneServer* /*sceneServer*/)
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Perform per-instance-rendering of shadow geometry. This method will be
    called after ApplyShadow() once for each instance of the
    node.
*/
bool
nSceneNode::RenderShadow(nSceneServer* /*sceneServer*/, nRenderContext* /*renderContext*/, const matrix44& /*modelMatrix*/)
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Return true if this node provides transformation. Should overriden
    by subclasses.
*/
bool
nSceneNode::HasTransform() const
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Return true if this node provides geometry. Should overriden
    by subclasses.
*/
bool
nSceneNode::HasGeometry() const
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Return true if this node provides a shader.
*/
bool
nSceneNode::HasShader() const
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Return true if this node provides shadow. Should be
    overriden by subclasses.
*/
bool
nSceneNode::HasLight() const
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Return true if this node provides light information. Should be
    overriden by subclasses.
*/
bool
nSceneNode::HasShadow() const
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Return true if this node is a camera. Should be
    overriden by subclasses.
*/
bool
nSceneNode::HasCamera() const
{
    return false;
}

//------------------------------------------------------------------------------
/**
    Add an animator object to this scene node.
*/
void
nSceneNode::AddAnimator(const char* relPath)
{
    n_assert(relPath);

    nDynAutoRef<nAnimator> newDynAutoRef;
    newDynAutoRef.set(relPath);
    this->animatorArray.Append(newDynAutoRef);
}

//------------------------------------------------------------------------------
/**
    Remove an animator object from this scene node.

    -23-Nov-06  kims  Changed GetName() to getname() cause it should retrieve 
                      relevant path not just name of an animator.
*/
void
nSceneNode::RemoveAnimator(const char* relPath)
{
    n_assert(relPath);

    const int numAnimators = this->GetNumAnimators();
    int i;
    for (i = 0; i < numAnimators; i++)
    {
        if (!strcmp(relPath, this->animatorArray[i].getname()))
        {
            this->animatorArray[i].set(0); //unset/clear the nDynAutoRef
            this->animatorArray.Erase(i);  //delete the nDynAutoRef from array
            break;
        }
    }
}

//------------------------------------------------------------------------------
/**
    Get number of animator objects.
*/
int
nSceneNode::GetNumAnimators() const
{
    return this->animatorArray.Size();
}

//------------------------------------------------------------------------------
/**
    Get path to animator object at given index.
*/
const char*
nSceneNode::GetAnimatorAt(int index)
{
    return this->animatorArray[index].getname();
}

//------------------------------------------------------------------------------
/**
    Invoke all shader animators. This method should be called classes
    which implement the RenderShader() method from inside this method.
*/
void
nSceneNode::InvokeAnimators(int type, nRenderContext* renderContext)
{
    int numAnimators = this->GetNumAnimators();
    if (numAnimators > 0)
    {
        n_assert(renderContext);

        nKernelServer::Instance()->PushCwd(this);
        int i;
        for (i = 0; i < numAnimators; i++)
        {
            nAnimator* animator = this->animatorArray[i].get();
            if (type == animator->GetAnimatorType())
            {
                animator->Animate(this, renderContext);
            }
        }
        nKernelServer::Instance()->PopCwd();
    }
}

//------------------------------------------------------------------------------
/**
    Returns a valid instance stream object for this scene node hierarchy.
    If no instance stream object exists yet, it will be created and stored.
    The instance stream declaration will be built from all shaders in the
    hierarchy by recursively calling UpdateInstStreamDecl().
*/
/*
nInstanceStream*
nSceneNode::GetInstanceStream()
{
    if (!this->refInstanceStream.isvalid())
    {
        nInstanceStream* instStream = nGfxServer2::Instance()->NewInstanceStream(0);
        n_assert(instStream);
        n_assert(!instStream->IsValid());

        // build an instance stream declaration from the hierarchy
        nInstanceStream::Declaration decl;
        this->UpdateInstStreamDecl(decl);

        instStream->SetDeclaration(decl);
        bool success = instStream->Load();
        n_assert(success);
        this->refInstanceStream = instStream;
    }
    return this->refInstanceStream.get();
}
*/

//------------------------------------------------------------------------------
/**
    Recursively build an instance stream declaration from the shaders in
    the scene node hierarchy. Override this method in subclasses with
    shader handling.
*/
void
nSceneNode::UpdateInstStreamDecl(nInstanceStream::Declaration& decl)
{
    nSceneNode* curChild;
    for (curChild = (nSceneNode*) this->GetHead();
         curChild;
         curChild = (nSceneNode*) curChild->GetSucc())
    {
        curChild->UpdateInstStreamDecl(decl);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
nSceneNode::RenderCamera(const matrix44& /*modelWorldMatrix*/, const matrix44& /*viewMatrix*/, const matrix44& /*projectionMatrix*/)
{
    // empty
    return false;
}
