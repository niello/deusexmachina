#ifndef N_SKYSTATE_H
#define N_SKYSTATE_H
//------------------------------------------------------------------------------
/**
    @class nSkyState
    @ingroup Scene

    Provides data for animated or state switching nodes

    (C) 2005 RadonLabs GmbH
*/
#include "scene/ntransformnode.h"

namespace Data
{
	class CBinaryReader;
}

class nSkyState: public nTransformNode
{
public:

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	virtual bool HasTransform() const { return false; }

	void SetInt(nShaderState::Param param, int val);
    int GetInt(nShaderState::Param param) const;
    void SetBool(nShaderState::Param param, bool val);
    bool GetBool(nShaderState::Param param) const;
    void SetFloat(nShaderState::Param param, float val);
    float GetFloat(nShaderState::Param param) const;
    void SetVector(nShaderState::Param param, const vector4& val);
    const vector4& GetVector(nShaderState::Param param) const;
	nShaderParams& GetShaderParams() { return shaderParams; }
	bool HasParam(nShaderState::Param param) { return shaderParams.IsParameterValid(param); }
    int GetNumParams() const;
    const char* GetParamNameByIndex(int index) const;
    const char* GetParamTypeByIndex(int index) const;

private:
    nShaderParams shaderParams;
    nString shaderName;
};
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
*/
inline
void
nSkyState::SetInt(nShaderState::Param param, int val)
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
nSkyState::GetInt(nShaderState::Param param) const
{
    return this->shaderParams.GetArg(param).GetInt();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSkyState::SetBool(nShaderState::Param param, bool val)
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
nSkyState::GetBool(nShaderState::Param param) const
{
    return this->shaderParams.GetArg(param).GetBool();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSkyState::SetFloat(nShaderState::Param param, float val)
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
nSkyState::GetFloat(nShaderState::Param param) const
{
    return this->shaderParams.GetArg(param).GetFloat();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSkyState::SetVector(nShaderState::Param param, const vector4& val)
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
nSkyState::GetVector(nShaderState::Param param) const
{
    return this->shaderParams.GetArg(param).GetVector4();
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nSkyState::GetNumParams() const
{
    return this->shaderParams.GetNumValidParams();
}

//------------------------------------------------------------------------------
/**
*/
inline
const char*
nSkyState::GetParamNameByIndex(int index) const
{
  return nShaderState::ParamToString(this->shaderParams.GetParamByIndex(index));
}
//------------------------------------------------------------------------------
/**
*/
inline
const char*
nSkyState::GetParamTypeByIndex(int index) const
{
  return nShaderState::TypeToString(this->shaderParams.GetArgByIndex(index).GetType());
}

#endif

