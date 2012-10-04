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

//------------------------------------------------------------------------------
class nMaterialNode : public nAbstractShaderNode
{
public:
    /// constructor
    nMaterialNode();

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	/// object persistency
    //virtual bool SaveCmds(nPersistServer* ps);
    /// load resources
    virtual bool LoadResources();
    /// unload resources
    virtual void UnloadResources();

    /// indicate to scene graph that we provide a surface shader
    virtual bool HasShader() const;
    /// perform pre-instancing rendering of shader
    virtual bool ApplyShader(nSceneServer* sceneServer);
    /// perform per-instance-rendering of shader
    virtual bool RenderShader(nSceneServer* sceneServer, nRenderContext* renderContext);

    /// set shader resource name
    void SetShader(const nString& name);
    /// get shader resource name
    const nString& GetShader() const;
    /// get bucket index of shader
    int GetShaderIndex();
    /// get pointer to shader object
    nShader2* GetShaderObject();
    /// set maya shader name
    void SetMayaShaderName(nString name);
    /// get maya shader name
    nString GetMayaShaderName() const;

protected:
    /// recursively append instance parameters to provided instance stream declaration
    virtual void UpdateInstStreamDecl(nInstanceStream::Declaration& decl);

private:
    /// load the shader resource
    bool LoadShader();
    /// unload the shader resource
    void UnloadShader();
    /// checks if shader uses texture passed in param
    virtual bool IsTextureUsed(nShaderState::Param param);

    nString mayaShaderName;
    nString shaderName;
    int shaderIndex;
    nRef<nShader2> refShader;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nMaterialNode::SetShader(const nString& name)
{
    n_assert(name.IsValid());
    this->shaderName = name;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nMaterialNode::GetShader() const
{
    return this->shaderName;
}

//------------------------------------------------------------------------------
/**
*/
inline
nShader2*
nMaterialNode::GetShaderObject()
{
    if (!this->AreResourcesValid())
    {
        this->LoadResources();
    }
    return this->refShader.get_unsafe();
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMaterialNode::GetShaderIndex()
{
    if (!this->AreResourcesValid())
    {
        this->LoadResources();
    }
    return this->shaderIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMaterialNode::SetMayaShaderName(nString name)
{
    this->mayaShaderName = name;
}

//------------------------------------------------------------------------------
/**
*/
inline
nString
nMaterialNode::GetMayaShaderName() const
{
    return this->mayaShaderName;
}


//------------------------------------------------------------------------------
#endif

