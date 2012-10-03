#ifndef N_ABSTRACTSHADERNODE_H
#define N_ABSTRACTSHADERNODE_H
//------------------------------------------------------------------------------
/**
    @class nAbstractShaderNode
    @ingroup Scene

    @brief This is the base class for all shader related scene node classes
    (for instance material and light nodes).

    All those classes need to hold named, typed shader variables, as well
    as texture resource management.

    See also @ref N2ScriptInterface_nabstractshadernode

    (C) 2003 RadonLabs GmbH
*/
#include "scene/ntransformnode.h"
#include "gfx2/ngfxserver2.h"
#include "gfx2/ntexture2.h"
#include "gfx2/nshader2.h"
#include "gfx2/nshaderparams.h"
#include "mathlib/transform33.h"

namespace Data
{
	class CBinaryReader;
}

//------------------------------------------------------------------------------
class nAbstractShaderNode : public nTransformNode
{
public:
    /// constructor
    nAbstractShaderNode();

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	/// object persistency
    //virtual bool SaveCmds(nPersistServer* ps);
    /// load resources
    virtual bool LoadResources();
    /// unload resources
    virtual void UnloadResources();

    /// set uv position for texture layer
    void SetUvPos(uint layer, const vector2& p);
    /// get uv position for texture layer
    const vector2& GetUvPos(uint layer) const;
    /// set uv euler rotation for texture layer
    void SetUvEuler(uint layer, const vector3& p);
    /// get uv euler rotation for texture layer
    const vector3& GetUvEuler(uint layer) const;
    /// set uv scale for texture layer
    void SetUvScale(uint layer, const vector2& p);
    /// get uv scale for texture layer
    const vector2& GetUvScale(uint layer) const;

    /// bind a texture resource to a shader variable
    void SetTexture(nShaderState::Param param, const char* texName);
    /// get texture resource bound to variable
    const char* GetTexture(nShaderState::Param param) const;
    /// bind a int value to a a shader variable
    void SetInt(nShaderState::Param param, int val);
    /// get an int value bound to a shader variable
    int GetInt(nShaderState::Param param) const;
    /// bind a bool value to a a shader variable
    void SetBool(nShaderState::Param param, bool val);
    /// get an bool value bound to a shader variable
    bool GetBool(nShaderState::Param param) const;
    /// bind a float value to a shader variable
    void SetFloat(nShaderState::Param param, float val);
    /// get a float value bound to a shader variable
    float GetFloat(nShaderState::Param param) const;
    /// bind a vector value to a shader variable
    void SetVector(nShaderState::Param param, const vector4& val);
    /// get a vector value bound to a shader variable
    const vector4& GetVector(nShaderState::Param param) const;
    /// get shader params
    nShaderParams& GetShaderParams();
    /// returns true, if node possesses the param
    bool HasParam(nShaderState::Param param);

    /// get number of textures
    int GetNumTextures() const;
    /// get texture resource name at index
    const char* GetTextureAt(int index) const;
    /// get texture shader parameter at index
    nShaderState::Param GetTextureParamAt(int index) const;

    int GetNumParams() const;
    const char* GetParamNameByIndex(int index) const;
    const char* GetParamTypeByIndex(int index) const;

protected:
    /// load a texture resource
    bool LoadTexture(int index);
    /// unload a texture resource
    void UnloadTexture(int index);
    /// abstract method: returns always true
    virtual bool IsTextureUsed(nShaderState::Param param);

    class TexNode
    {
    public:
        /// default constructor
        TexNode();
        /// constructor
        TexNode(nShaderState::Param shaderParam, const char* texName);

        nShaderState::Param shaderParameter;
        nString texName;
        nRef<nTexture2> refTexture;
    };

    nArray<TexNode> texNodeArray;
    transform33 textureTransform[nGfxServer2::MaxTextureStages];
    nShaderParams shaderParams;
};

//------------------------------------------------------------------------------
/**
*/
inline
nAbstractShaderNode::TexNode::TexNode() :
    shaderParameter(nShaderState::InvalidParameter)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nAbstractShaderNode::TexNode::TexNode(nShaderState::Param shaderParam, const char* name) :
    shaderParameter(shaderParam),
    texName(name)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAbstractShaderNode::SetUvPos(uint layer, const vector2& p)
{
    n_assert(layer < nGfxServer2::MaxTextureStages);
    this->textureTransform[layer].settranslation(p);
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector2&
nAbstractShaderNode::GetUvPos(uint layer) const
{
    n_assert(layer < nGfxServer2::MaxTextureStages);
    return this->textureTransform[layer].gettranslation();
}

//------------------------------------------------------------------------------
/**
    -01-Nov-06  kims  Changed to have vector3 in-args for uv animation.
*/
inline
void
nAbstractShaderNode::SetUvEuler(uint layer, const vector3& e)
{
    n_assert(layer < nGfxServer2::MaxTextureStages);
    this->textureTransform[layer].seteulerrotation(e);
}

//------------------------------------------------------------------------------
/**
    -01-Nov-06  kims  Changed to return vector3 type for uv animation.
*/
inline
const vector3&
nAbstractShaderNode::GetUvEuler(uint layer) const
{
    n_assert(layer < nGfxServer2::MaxTextureStages);
    return this->textureTransform[layer].geteulerrotation();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAbstractShaderNode::SetUvScale(uint layer, const vector2& s)
{
    n_assert(layer < nGfxServer2::MaxTextureStages);
    this->textureTransform[layer].setscale(s);
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector2&
nAbstractShaderNode::GetUvScale(uint layer) const
{
    n_assert(layer < nGfxServer2::MaxTextureStages);
    return this->textureTransform[layer].getscale();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAbstractShaderNode::SetInt(nShaderState::Param param, int val)
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
nAbstractShaderNode::GetInt(nShaderState::Param param) const
{
    return this->shaderParams.GetArg(param).GetInt();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAbstractShaderNode::SetBool(nShaderState::Param param, bool val)
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
nAbstractShaderNode::GetBool(nShaderState::Param param) const
{
    return this->shaderParams.GetArg(param).GetBool();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAbstractShaderNode::SetFloat(nShaderState::Param param, float val)
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
nAbstractShaderNode::GetFloat(nShaderState::Param param) const
{
    return this->shaderParams.GetArg(param).GetFloat();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAbstractShaderNode::SetVector(nShaderState::Param param, const vector4& val)
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
nAbstractShaderNode::GetVector(nShaderState::Param param) const
{
    return this->shaderParams.GetArg(param).GetVector4();
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAbstractShaderNode::GetNumTextures() const
{
    return this->texNodeArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nAbstractShaderNode::GetTextureAt(int index) const
{
    return this->texNodeArray[index].texName.Get();
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderState::Param
nAbstractShaderNode::GetTextureParamAt(int index) const
{
    return this->texNodeArray[index].shaderParameter;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nAbstractShaderNode::GetNumParams() const
{
    return this->shaderParams.GetNumValidParams();
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nAbstractShaderNode::GetParamNameByIndex(int index) const
{
  return nShaderState::ParamToString(this->shaderParams.GetParamByIndex(index));
}
//------------------------------------------------------------------------------
/**
*/
inline
const char*
nAbstractShaderNode::GetParamTypeByIndex(int index) const
{
  return nShaderState::TypeToString(this->shaderParams.GetArgByIndex(index).GetType());
}


//------------------------------------------------------------------------------
#endif
