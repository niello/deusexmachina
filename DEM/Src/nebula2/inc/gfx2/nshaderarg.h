#ifndef N_SHADERARG_H
#define N_SHADERARG_H
//------------------------------------------------------------------------------
/**
    @class nShaderArg
    @ingroup Gfx2

    Encapsulates an argument for a shader parameter. This is similar
    to an Data::CData, but does only handle argument types which are
    relevant for a shader.

    (C) 2003 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "kernel/nref.h"
#include "gfx2/nshaderstate.h"
#include "mathlib/vector.h"
#include "mathlib/matrix.h"

class nTexture2;

//------------------------------------------------------------------------------
class nShaderArg
{
public:
    /// constructor
    nShaderArg();
    /// constructor with fixed type
    nShaderArg(nShaderState::Type t);
    /// bool constructor
    nShaderArg(bool val);
    /// int constructor
    nShaderArg(int val);
    /// float constructor
    nShaderArg(float val);
    /// nFloat4 constructor
    nShaderArg(const nFloat4& val);
    /// vector4 constructor
    nShaderArg(const vector4& val);
    /// matrix44 constructor
    nShaderArg(const matrix44* val);
    /// texture constructor
    nShaderArg(nTexture2* val);
    /// destructor
    ~nShaderArg();
    /// equality operator
    bool operator==(const nShaderArg& rhs) const;
    /// assignment operator
    void operator=(const nShaderArg& rhs);
    /// set the data type
    void SetType(nShaderState::Type t);
    /// get data type
    nShaderState::Type GetType() const;
    /// set bool value
    void SetBool(bool val);
    /// get bool value
    bool GetBool() const;
    /// set int value
    void SetInt(int val);
    /// get int value
    int GetInt() const;
    /// set float value
    void SetFloat(float val);
    /// get float value
    float GetFloat() const;
    /// set float4 value
    void SetFloat4(const nFloat4& val);
    /// get float4 value
    const nFloat4& GetFloat4() const;
    /// set content as vector4 value
    void SetVector4(const vector4& val);
    /// get content as vector4 value
    const vector4& GetVector4() const;
    /// set matrix value
    void SetMatrix44(const matrix44* val);
    /// get matrix value
    const matrix44* GetMatrix44() const;
    /// set texture
    void SetTexture(nTexture2* val);
    /// get texture
    nTexture2* GetTexture() const;
    /// clear the memory and set type to void
    void Clear();

private:
    nShaderState::Type type;
    union
    {
        bool b;
        int i;
        float f;
        nFloat4 f4;
        float m[4][4];
    };
    nRef<nTexture2> tex;
};

//------------------------------------------------------------------------------
/**
*/
inline
nShaderArg::nShaderArg()
{
    this->Clear();
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderArg::nShaderArg(nShaderState::Type t) :
    type(t),
    i(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderArg::~nShaderArg()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShaderArg::Clear()
{
    this->type = nShaderState::Void;
    memset(&(this->m), 0, sizeof(this->m));  // make sure that the largest data type is used
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nShaderArg::operator==(const nShaderArg& rhs) const
{
    if (this->type == rhs.type)
    {
        switch (this->type)
        {
            case nShaderState::Void:
                return true;

            case nShaderState::Bool:
                return (this->b == rhs.b);

            case nShaderState::Int:
                return (this->i == rhs.i);

            case nShaderState::Float:
                return (this->f == rhs.f);

            case nShaderState::Float4:
                return ((this->f4.x == rhs.f4.x) &&
                        (this->f4.y == rhs.f4.y) &&
                        (this->f4.z == rhs.f4.z) &&
                        (this->f4.w == rhs.f4.w));

            case nShaderState::Matrix44:
                {
                    bool equal = true;
                    int i;
                    for (i = 0; i < 4; i++)
                    {
                        int j;
                        for (j = 0; j < 4; j++)
                        {
                            if (this->m[i][j] != rhs.m[i][j])
                            {
                                equal = false;
                            }
                        }
                    }
                    return equal;
                }

            case nShaderState::Texture:
                return (this->tex.get_unsafe() == rhs.tex.get_unsafe());

            default:
                n_error("nShaderArg::operator==(): Invalid nShaderArg type!");
                break;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShaderArg::operator=(const nShaderArg& rhs)
{
    this->type = rhs.type;
    switch (rhs.type)
    {
        case nShaderState::Void:
            break;

        case nShaderState::Bool:
            this->b = rhs.b;
            break;

        case nShaderState::Int:
            this->i = rhs.i;
            break;

        case nShaderState::Float:
            this->f = rhs.f;
            break;

        case nShaderState::Float4:
            this->f4 = rhs.f4;
            break;

        case nShaderState::Matrix44:
            memcpy(&(this->m), &(rhs.m), sizeof(this->m));
            break;

        case nShaderState::Texture:
            this->tex = rhs.tex.get_unsafe();
            break;

        default:
            n_error("nShaderArg::operator=(): Invalid argument type!");
            break;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShaderArg::SetType(nShaderState::Type t)
{
    this->type = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderState::Type
nShaderArg::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShaderArg::SetBool(bool val)
{
    this->type = nShaderState::Bool;
    this->b = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nShaderArg::GetBool() const
{
    return this->b;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShaderArg::SetInt(int val)
{
    this->type = nShaderState::Int;
    this->i = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nShaderArg::GetInt() const
{
    return this->i;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShaderArg::SetFloat(float val)
{
    this->type = nShaderState::Float;
    this->f = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nShaderArg::GetFloat() const
{
    return this->f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShaderArg::SetFloat4(const nFloat4& val)
{
    this->type = nShaderState::Float4;
    this->f4 = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nFloat4&
nShaderArg::GetFloat4() const
{
    return this->f4;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShaderArg::SetVector4(const vector4& val)
{
    this->type = nShaderState::Float4;
    this->f4 = *(nFloat4*)&val;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector4&
nShaderArg::GetVector4() const
{
    return *(vector4*)&this->f4;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShaderArg::SetMatrix44(const matrix44* val)
{
    n_assert(val);
    this->type = nShaderState::Matrix44;
    int i;
    for (i = 0; i < 4; i++)
    {
        int j;
        for (j = 0; j < 4; j++)
        {
            this->m[i][j] = val->m[i][j];
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
const matrix44*
nShaderArg::GetMatrix44() const
{
    return (matrix44*)this->m;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nShaderArg::SetTexture(nTexture2* val)
{
    this->type = nShaderState::Texture;
    this->tex = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
nTexture2*
nShaderArg::GetTexture() const
{
    return this->tex.get_unsafe();
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderArg::nShaderArg(bool val) :
    type(nShaderState::Bool),
    b(val)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderArg::nShaderArg(int val) :
    type(nShaderState::Int),
    i(val)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderArg::nShaderArg(float val) :
    type(nShaderState::Float),
    f(val)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderArg::nShaderArg(const nFloat4& val) :
    type(nShaderState::Float4),
    f4(val)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderArg::nShaderArg(const vector4& val) :
    type(nShaderState::Float4)
{
    this->f4.x = val.x;
    this->f4.y = val.y;
    this->f4.z = val.z;
    this->f4.w = val.w;
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderArg::nShaderArg(const matrix44* val)
{
    this->SetMatrix44(val);
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderArg::nShaderArg(nTexture2* val) :
    type(nShaderState::Texture)
{
    if (val)
    {
        this->tex = val;
    }
    else
    {
        this->tex.invalidate();
    }
}

//------------------------------------------------------------------------------
#endif
