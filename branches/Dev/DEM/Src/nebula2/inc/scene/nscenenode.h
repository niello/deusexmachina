#ifndef N_SCENENODE_H
#define N_SCENENODE_H
//------------------------------------------------------------------------------
/**
    @class nSceneNode
    @ingroup Scene

    @brief The nSceneNode is the base class of all objects which can be attached
    to a scene managed by the nSceneServer class. A scene node object
    may provide transform, geometry, shader and volume information.

    See also @ref N2ScriptInterface_nscenenode

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/nroot.h"
#include "util/narray.h"
#include "mathlib/matrix.h"
#include "mathlib/bbox.h"
#include "kernel/ndynautoref.h"
#include "gfx2/nshaderparams.h"
#include "gfx2/ninstancestream.h"
#include "gfx2/nlight.h"
#include <Data/Params.h>

class nSceneServer;
class nRenderContext;
class nGfxServer2;
class nAnimator;
class nVariableServer;

namespace Data
{
	class CBinaryReader;
}

//-------------------------------------------------------------------------------
class nSceneNode : public nRoot
{
protected:

	bbox3 localBox;
	nArray<nDynAutoRef<nAnimator> > animatorArray;
	Data::CParams attrs;
	int renderPri;
	bool resourcesValid;
	ushort hints;

	/// recursively append instance parameters to provided instance stream declaration
	virtual void UpdateInstStreamDecl(nInstanceStream::Declaration& decl);

public:
    /// scene node hints
    enum
    {
        HierarchyNode = (1<<0),     // this was exported as hierarchy nodes from Maya
        LodNode = (1<<1),           // this was exported as LOD node from Maya
        LevelSegment = (1<<2)       // this was exported as level segment from Maya
    };
    /// constructor
    nSceneNode();

	/// release object
    virtual bool Release();

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

    /// load resources for this object
    virtual bool LoadResources();
    /// unload resources for this object
    virtual void UnloadResources();
    /// return true if resources for this object are valid
	bool AreResourcesValid() const { return resourcesValid; }
    /// recursively preload resources
    void PreloadResources();
    /// set one or more hint flags
	void AddHints(ushort h) { this->hints |= h; }
    /// clear one or more hint flags
	void ClearHints(ushort h) { this->hints &= ~h; }
    /// get all hints
	ushort GetHints() const { return this->hints; }
    /// return true if hint is set
	bool HasHints(ushort h) const { return h == (this->hints & h); }
    /// called by app when new render context has been created for this object
    virtual void RenderContextCreated(nRenderContext* renderContext);
    /// called by app when render context is going to be released
    virtual void RenderContextDestroyed(nRenderContext* renderContext);
    /// called by nSceneServer when object is attached to scene
    virtual void Attach(nSceneServer* sceneServer, nRenderContext* renderContext);
    /// return true if node provides transformation
    virtual bool HasTransform() const;
    /// return true if node provides geometry
    virtual bool HasGeometry() const;
    /// return true if node provides shader
    virtual bool HasShader() const;
    /// return true if node provides lighting information
    virtual bool HasLight() const;
    /// return true if node provides shadow
    virtual bool HasShadow() const;
    /// return true if node is camera
    virtual bool HasCamera() const;
    /// render transformation
    virtual bool RenderTransform(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& parentMatrix);
    /// perform pre-instancing rendering of geometry
    virtual bool ApplyGeometry(nSceneServer* sceneServer);
    /// perform per-instance-rendering of geometry
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext);
    /// render debug information
    virtual void RenderDebug(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& modelMatrix);
    /// perform pre-instancing rending of shader
    virtual bool ApplyShader(nSceneServer* sceneServer);
    /// perform per-instance-rendering of shader
    virtual bool RenderShader(nSceneServer* sceneServer, nRenderContext* renderContext);
    /// set per-light states
    virtual const nLight& ApplyLight(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& lightTransform, const vector4& shadowLightMask);
    /// set per-shape-instance light states
    virtual const nLight& RenderLight(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& lightTransform);
    /// perform pre-instancing rendering of shadow
    virtual bool ApplyShadow(nSceneServer* sceneServer);
    /// perform per-instance-rendering of shadow
    virtual bool RenderShadow(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& modelMatrix);
    /// render the scene from the provided camera
    virtual bool RenderCamera(const matrix44& modelWorldMatrix, const matrix44& viewMatrix, const matrix44& projectionMatrix);
    /// set the local bounding box
	void SetLocalBox(const bbox3& b) { this->localBox = b; }
    /// get the node's bounding box
	const bbox3& GetLocalBox() const { return this->localBox; }
    /// set render priority
	void SetRenderPri(int pri) { n_assert((pri >= -127) && (pri <= 127)); renderPri = pri; }
    /// get render priority
	int GetRenderPri() const { return this->renderPri; }
    /// add an animator object
    void AddAnimator(const char* path);
    /// remove an animator object
    void RemoveAnimator(const char* path);
    /// get number of animator objects
    int GetNumAnimators() const;
    /// get animator object at index
    const char* GetAnimatorAt(int index);
    /// invoke all animators
    void InvokeAnimators(int animatorType, nRenderContext* renderContext);
    /// return true if attribute exists
	bool HasAttr(CStrID name) const { return attrs.Has(name); }
    /// generic read access to attribute
	const Data::CParam& GetAttr(CStrID name) const { return this->attrs[name]; }
	template<class T> const T& GetAttr(CStrID name) const { return attrs[name].GetValue<T>(); }
    /// set int custom attribute
	template<class T> void SetAttr(CStrID name, const T& val) { this->attrs.Set(name, val); }
};

//------------------------------------------------------------------------------
/**
nSceneNode::SetLocalBox(const bbox3& b)
    Define the local bounding box. Shape node compute their bounding
    box automatically at load time. This method can be used to define
    bounding boxes for other nodes. This may be useful for higher level
    code like game frameworks. Nebula itself only uses bounding boxes
    defined on shape nodes.
*/

//------------------------------------------------------------------------------
/**
inline void nSceneNode::SetRenderPri(int pri)
    Set the render priority. This should be a number between -127 and +127,
    the default is 0. Smaller numbers will render first.
*/

#endif
