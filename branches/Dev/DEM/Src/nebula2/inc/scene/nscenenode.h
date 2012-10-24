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
	Data::CParams attrs;
	int renderPri;
	bool resourcesValid;
	ushort hints;

public:
    /// scene node hints
    enum
    {
        LevelSegment = (1<<2)       // this was exported as level segment from Maya
    };

	nSceneNode(): resourcesValid(false), renderPri(0), hints(0) {}

	virtual bool Release() { if (RefCount == 1) UnloadResources(); return nRoot::Release(); }

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	virtual bool LoadResources() { resourcesValid = true; return true; }
	virtual void UnloadResources() { resourcesValid = false; }
	bool AreResourcesValid() const { return resourcesValid; }
    void PreloadResources();
	void AddHints(ushort h) { this->hints |= h; }
	void ClearHints(ushort h) { this->hints &= ~h; }
	ushort GetHints() const { return this->hints; }
	bool HasHints(ushort h) const { return h == (this->hints & h); }
    virtual void RenderContextCreated(nRenderContext* renderContext);
    virtual void RenderContextDestroyed(nRenderContext* renderContext);
    virtual void Attach(nSceneServer* sceneServer, nRenderContext* renderContext);
    virtual bool HasTransform() const { return false; }
    virtual bool HasGeometry() const { return false; }
    virtual bool HasShader() const { return false; }
    virtual bool HasLight() const { return false; }
    virtual bool HasShadow() const { return false; }
    virtual bool HasCamera() const { return false; }
    virtual bool RenderTransform(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& parentMatrix) { return false; }
    virtual bool ApplyGeometry(nSceneServer* sceneServer) { return false; }
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext) { return false; }
    virtual void RenderDebug(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& modelMatrix) { }
    virtual bool ApplyShader(nSceneServer* sceneServer) { return false; }
    virtual bool RenderShader(nSceneServer* sceneServer, nRenderContext* renderContext) { return false; }
    virtual const nLight& ApplyLight(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& lightTransform, const vector4& shadowLightMask);
    virtual const nLight& RenderLight(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& lightTransform);
    virtual bool ApplyShadow(nSceneServer* sceneServer) { return false; }
    virtual bool RenderShadow(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& modelMatrix) { return false; }
	virtual bool RenderCamera(const matrix44& modelWorldMatrix, const matrix44& viewMatrix, const matrix44& projectionMatrix) { return false; }
	void SetLocalBox(const bbox3& b) { this->localBox = b; }
	const bbox3& GetLocalBox() const { return this->localBox; }
	void SetRenderPri(int pri) { n_assert((pri >= -127) && (pri <= 127)); renderPri = pri; }
	int GetRenderPri() const { return this->renderPri; }
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

//------------------------------------------------------------------------------
/**
inline void nSceneNode::SetRenderPri(int pri)
    Set the render priority. This should be a number between -127 and +127,
    the default is 0. Smaller numbers will render first.
*/

#endif
