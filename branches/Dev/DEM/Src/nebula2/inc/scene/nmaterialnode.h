#ifndef N_MATERIALNODE_H
#define N_MATERIALNODE_H
//------------------------------------------------------------------------------
/**
    @class nMaterialNode
    @ingroup Scene

    @brief A material node defines a shader resource and associated shader
    variables. A shader resource is an external file (usually a text file)
    which defines a surface shader.

    Material nodes themselves cannot render anything useful, they can just
    adjust render states as preparation of an actual rendering process.
    Thus, subclasses should be derived which implement some sort of
    shape rendering.

    - 27-Jan-05 floh    removed shader FourCC code stuff

    (C) 2002 RadonLabs GmbH
*/
#include "scene/nabstractshadernode.h"
#include "kernel/nref.h"

class nShader2;
class nGfxServer2;

namespace Data
{
	class CBinaryReader;
}

class nMaterialNode: public nAbstractShaderNode
{
private:

    nString shaderName;
    int shaderIndex;
    nRef<nShader2> refShader;

	bool LoadShader();
    void UnloadShader();
	virtual bool IsTextureUsed(nShaderState::Param param) { return refShader.isvalid() && refShader->IsParameterUsed(param); }

public:

	nMaterialNode(): shaderIndex(-1) {}

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	virtual bool LoadResources();
    virtual void UnloadResources();

	virtual bool HasShader() const { return true; }

	virtual bool ApplyShader(nSceneServer* sceneServer);
    virtual bool RenderShader(nSceneServer* sceneServer, nRenderContext* renderContext);

	virtual bool ApplyGeometry(nSceneServer* sceneServer) { return false; }
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext) { return false; }

	void SetShader(const nString& name) { n_assert(name.IsValid()); shaderName = name; }
	const nString& GetShader() const { return shaderName; }
    int GetShaderIndex();
    nShader2* GetShaderObject();
};

inline nShader2* nMaterialNode::GetShaderObject()
{
	if (!AreResourcesValid()) LoadResources();
	return refShader.get_unsafe();
}
//---------------------------------------------------------------------

inline int nMaterialNode::GetShaderIndex()
{
	if (!AreResourcesValid()) LoadResources();
	return shaderIndex;
}
//---------------------------------------------------------------------

#endif

