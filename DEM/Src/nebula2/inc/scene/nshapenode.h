#ifndef N_SHAPENODE_H
#define N_SHAPENODE_H
//------------------------------------------------------------------------------
/**
    @class nShapeNode
    @ingroup SceneNodes

    @brief A shape node is the simplest actually visible object in the
    scene node class hierarchy.

    It is derived from nMaterialNode, and thus inherits transform and
    shader information. It adds a simple mesh which it can render.

    See also @ref N2ScriptInterface_nshapenode

    (C) 2002 RadonLabs GmbH
*/
#include "scene/nmaterialnode.h"
#include "gfx2/nmesh2.h"

class nShapeNode : public nMaterialNode
{
protected:

    nRef<nMesh2> refMesh;
    nString meshName;

	bool LoadMesh();
    void UnloadMesh();

public:

	int meshUsage;
    int groupIndex;

	nShapeNode(): groupIndex(0), meshUsage(nMesh2::WriteOnce) {}

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

    virtual bool LoadResources();
    virtual void UnloadResources();

	virtual bool HasGeometry() const { return true; }
    virtual bool ApplyGeometry(nSceneServer* sceneServer);
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext);

	void SetMesh(const nString& name) { n_assert(name.IsValid()); UnloadMesh(); meshName = name; }
	const nString& GetMesh() const { return meshName; }
	nMesh2* GetMeshObject() { if (!refMesh.isvalid()) { n_assert(LoadMesh()); } return refMesh.get(); }
};

#endif

