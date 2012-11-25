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
#include "scene/ntransformnode.h"
#include "kernel/nref.h"
#include "gfx2/nshader2.h"

class nShader2;
class nGfxServer2;

namespace Data
{
	class CBinaryReader;
}

class nMaterialNode: public nTransformNode //nMaterialNode
{
private:

    struct CTextureNode
    {
        nShaderState::Param shaderParameter;
        nString texName;
        nRef<nTexture2> refTexture;

		CTextureNode(): shaderParameter(nShaderState::InvalidParameter) {}
		CTextureNode(nShaderState::Param shaderParam, const char* name): shaderParameter(shaderParam), texName(name) {}
    };

    nString shaderName;
    int shaderIndex;
    nRef<nShader2> refShader;

	nArray<CTextureNode> texNodeArray;
    nShaderParams shaderParams;

	bool LoadShader();
    void UnloadShader();
	bool LoadTexture(int index);
    void UnloadTexture(int index);
	bool IsTextureUsed(nShaderState::Param param) { return refShader.isvalid() && refShader->IsParameterUsed(param); }

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

    void SetInt(nShaderState::Param param, int val);
    int GetInt(nShaderState::Param param) const;
    void SetBool(nShaderState::Param param, bool val);
    bool GetBool(nShaderState::Param param) const;
    void SetFloat(nShaderState::Param param, float val);
    float GetFloat(nShaderState::Param param) const;
    void SetVector(nShaderState::Param param, const vector4& val);
    const vector4& GetVector(nShaderState::Param param) const;

	void SetTexture(nShaderState::Param param, const char* texName);
    const char* GetTexture(nShaderState::Param param) const;

	nShaderParams& GetShaderParams() { return shaderParams; }
	bool HasParam(nShaderState::Param param) { return shaderParams.IsParameterValid(param); }

	int GetNumTextures() const;
    const char* GetTextureAt(int index) const;
    nShaderState::Param GetTextureParamAt(int index) const;

    int GetNumParams() const;
    const char* GetParamNameByIndex(int index) const;
    const char* GetParamTypeByIndex(int index) const;
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

//------------------------------------------------------------------------------
/**
*/
inline
void
nMaterialNode::SetInt(nShaderState::Param param, int val)
{
    // silently ignore invalid parameters
    if (nShaderState::InvalidParameter == param)
    {
        n_printf("WARNING: invalid shader parameter in object '%s'\n", this->GetName());
        return;
    }
    this->shaderParams.SetArg(param, nShaderArg(val));
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMaterialNode::GetInt(nShaderState::Param param) const
{
    return this->shaderParams.GetArg(param).GetInt();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMaterialNode::SetBool(nShaderState::Param param, bool val)
{
    // silently ignore invalid parameters
    if (nShaderState::InvalidParameter == param)
    {
        n_printf("WARNING: invalid shader parameter in object '%s'\n", this->GetName());
        return;
    }
    this->shaderParams.SetArg(param, nShaderArg(val));
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nMaterialNode::GetBool(nShaderState::Param param) const
{
    return this->shaderParams.GetArg(param).GetBool();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMaterialNode::SetFloat(nShaderState::Param param, float val)
{
    // silently ignore invalid parameters
    if (nShaderState::InvalidParameter == param)
    {
        n_printf("WARNING: invalid shader parameter in object '%s'\n", this->GetName());
        return;
    }
    this->shaderParams.SetArg(param, nShaderArg(val));
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nMaterialNode::GetFloat(nShaderState::Param param) const
{
    return this->shaderParams.GetArg(param).GetFloat();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nMaterialNode::SetVector(nShaderState::Param param, const vector4& val)
{
    // silently ignore invalid parameters
    if (nShaderState::InvalidParameter == param)
    {
        n_printf("WARNING: invalid shader parameter in object '%s'\n", this->GetName());
        return;
    }
    this->shaderParams.SetArg(param, nShaderArg(val));
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector4&
nMaterialNode::GetVector(nShaderState::Param param) const
{
    return this->shaderParams.GetArg(param).GetVector4();
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMaterialNode::GetNumTextures() const
{
    return this->texNodeArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nMaterialNode::GetTextureAt(int index) const
{
    return this->texNodeArray[index].texName.Get();
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderState::Param
nMaterialNode::GetTextureParamAt(int index) const
{
    return this->texNodeArray[index].shaderParameter;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nMaterialNode::GetNumParams() const
{
    return this->shaderParams.GetNumValidParams();
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nMaterialNode::GetParamNameByIndex(int index) const
{
  return nShaderState::ParamToString(this->shaderParams.GetParamByIndex(index));
}
//------------------------------------------------------------------------------
/**
*/
inline
const char*
nMaterialNode::GetParamTypeByIndex(int index) const
{
  return nShaderState::TypeToString(this->shaderParams.GetArgByIndex(index).GetType());
}

#endif

