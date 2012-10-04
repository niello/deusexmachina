#ifndef N_SHADERPARAMS_H
#define N_SHADERPARAMS_H
//------------------------------------------------------------------------------
/**
    @class nShaderParams
    @ingroup Gfx2

    A container for shader parameters. A shader parameter block
    can be applied to a shader with one call (instead of issuing dozens
    of method calls to set parameters).

    Note that only simple data types (not arrays) can be kept in shader
    parameter blocks.

    (C) 2003 RadonLabs GmbH
*/
#include "gfx2/nshaderstate.h"
#include "gfx2/nshaderarg.h"
#include "util/narray.h"

//------------------------------------------------------------------------------
class nShaderParams
{
public:
    /// constructor
    nShaderParams();
    /// destructor
    ~nShaderParams();
    /// clear array
    void Clear();
    /// get number of valid parameters in object
    int GetNumValidParams() const;
    /// return true if parameter is valid
    bool IsParameterValid(nShaderState::Param p) const;
    /// set a single parameter
    void SetArg(nShaderState::Param p, const nShaderArg& arg);
    /// get a single parameter
    const nShaderArg& GetArg(nShaderState::Param p) const;
    /// clear a single parameter
    void ClearParam(nShaderState::Param p);
    /// get shader parameter using direct index
    nShaderState::Param GetParamByIndex(int index) const;
    /// get shader argument using direct index
    const nShaderArg& GetArgByIndex(int index) const;

private:
    class ParamAndArg
    {
    public:
        /// default constructor
        ParamAndArg();
        /// constructor
        ParamAndArg(nShaderState::Param p, const nShaderArg& a);

        nShaderState::Param param;
        nShaderArg arg;
    };
    char paramIndex[nShaderState::NumParameters];   // index into paramArray, -1 for invalid params
    nArray<ParamAndArg> paramArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
nShaderParams::ParamAndArg::ParamAndArg() :
    param(nShaderState::InvalidParameter)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderParams::ParamAndArg::ParamAndArg(nShaderState::Param p, const nShaderArg& a) :
    param(p),
    arg(a)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShaderParams::Clear()
{
    int i;
    for (i = 0; i < nShaderState::NumParameters; i++)
    {
        this->paramIndex[i] = -1;
    }
    // kaikai: paramArray should Clear instead of Reset, because
    //  nShaderArg has nRef<Texture2> which need to be destroyed.
    this->paramArray.Clear();
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderParams::nShaderParams() :
    paramArray(0, 8)
{
    this->Clear();
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderParams::~nShaderParams()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nShaderParams::GetNumValidParams() const
{
    return this->paramArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline
const nShaderArg&
nShaderParams::GetArgByIndex(int index) const
{
    n_assert(index >= 0 && index < this->paramArray.Size());
    return this->paramArray[index].arg;
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderState::Param
nShaderParams::GetParamByIndex(int index) const
{
    n_assert(index >= 0 && index < this->paramArray.Size());
    return this->paramArray[index].param;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nShaderParams::IsParameterValid(nShaderState::Param p) const
{
    n_assert(p >= 0 && p < nShaderState::NumParameters);
    return (-1 != this->paramIndex[p]);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShaderParams::SetArg(nShaderState::Param p, const nShaderArg& arg)
{
    n_assert(p >= 0 && p < nShaderState::NumParameters);
    char index = this->paramIndex[p];

    ParamAndArg paramAndArg(p, arg);
    if (index == -1)
    {
        this->paramArray.Append(paramAndArg);
        this->paramIndex[p] = this->paramArray.Size() - 1;
    }
    else
    {
        this->paramArray[index] = paramAndArg;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
const nShaderArg&
nShaderParams::GetArg(nShaderState::Param p) const
{
    static nShaderArg invalidArg;
    n_assert(p >= 0 && p < nShaderState::NumParameters);
    char index = this->paramIndex[p];
    if (index != -1)
    {
        return this->paramArray[index].arg;
    }
    else
    {
        n_assert2(index != -1, "Shader parameter wasn't set!");
        return invalidArg;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShaderParams::ClearParam(nShaderState::Param p)
{
    n_assert(p >= 0 && p < nShaderState::NumParameters);
    char index = this->paramIndex[p];

    if (-1 != index)
    {
        // remove arg
        this->paramArray.Erase(index);
        this->paramIndex[p] = -1;

        // fix remaining indices
        int i;
        for (i = 0; i < nShaderState::NumParameters; i++)
        {
            if (this->paramIndex[i] >= index)
            {
                --this->paramIndex[i];
            }
        }
    }
}

//------------------------------------------------------------------------------
#endif

