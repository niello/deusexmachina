#ifndef N_RPSHADER_H
#define N_RPSHADER_H
//------------------------------------------------------------------------------
/**
    @class nRpShader
    @ingroup RenderPath
    @brief A render path shader definition.

    (C) 2004 Radon Labs GmbH
*/
#include "kernel/ntypes.h"
#include "util/nstring.h"
#include "util/narray.h"
#include "kernel/nref.h"
#include "gfx2/nshader2.h"

//------------------------------------------------------------------------------
class nRpShader
{
public:
    /// constructor
    nRpShader();
    /// destructor
    ~nRpShader();
    /// assignment operator
    void operator=(const nRpShader& rhs);
    /// set shader name
    void SetName(const nString& n);
    /// get shader name
    const nString& GetName() const;
    /// set shader file
    void SetFilename(const nString& n);
    /// get shader file
    const nString& GetFilename() const;
    /// set shader bucket index
    void SetBucketIndex(int i);
    /// get shader bucket index
    int GetBucketIndex() const;
    /// validate the shader
    void Validate();
    /// get shader pointer
    nShader2* GetShader() const;

private:
    nString name;
    nString filename;
    int bucketIndex;
    nRef<nShader2> refShader;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpShader::operator=(const nRpShader& rhs)
{
    this->name        = rhs.name;
    this->filename    = rhs.filename;
    this->bucketIndex = rhs.bucketIndex;
    this->refShader   = rhs.refShader;
    if (this->refShader.isvalid())
    {
        this->refShader->AddRef();
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpShader::SetName(const nString& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRpShader::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpShader::SetFilename(const nString& n)
{
    this->filename = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRpShader::GetFilename() const
{
    return this->filename;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpShader::SetBucketIndex(int i)
{
    n_assert(i >= 0);
    this->bucketIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nRpShader::GetBucketIndex() const
{
    return this->bucketIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
nShader2*
nRpShader::GetShader() const
{
    return this->refShader;
}

//------------------------------------------------------------------------------
#endif
