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
#include "gfx2/nshaderparams.h"
#include <Data/Params.h>

class nSceneServer;
class nRenderContext;

namespace Data
{
	class CBinaryReader;
}

class nSceneNode: public nRoot
{
protected:

	bbox3 localBox;
	Data::CParams attrs;
	bool resourcesValid;

public:

	nSceneNode(): resourcesValid(false) {}

	virtual bool Release() { if (RefCount == 1) UnloadResources(); return nRoot::Release(); }

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	virtual bool LoadResources() { resourcesValid = true; return true; }
	virtual void UnloadResources() { resourcesValid = false; }
	bool AreResourcesValid() const { return resourcesValid; }
    void PreloadResources();
    virtual void RenderContextCreated(nRenderContext* renderContext);
    virtual void RenderContextDestroyed(nRenderContext* renderContext);
    virtual void Attach(nSceneServer* sceneServer, nRenderContext* renderContext);

	virtual bool HasTransform() const { return false; }
    virtual bool HasGeometry() const { return false; }
    virtual bool HasShader() const { return false; }
    virtual bool HasLight() const { return false; }

	virtual void RenderDebug(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& modelMatrix) { }

	void SetLocalBox(const bbox3& b) { this->localBox = b; }
	const bbox3& GetLocalBox() const { return this->localBox; }
	bool HasAttr(CStrID name) const { return attrs.Has(name); }
	const Data::CParam& GetAttr(CStrID name) const { return this->attrs[name]; }
	template<class T> const T& GetAttr(CStrID name) const { return attrs[name].GetValue<T>(); }
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

#endif
