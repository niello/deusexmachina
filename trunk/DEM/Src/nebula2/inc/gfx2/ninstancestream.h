#ifndef N_INSTANCESTREAM_H
#define N_INSTANCESTREAM_H
//------------------------------------------------------------------------------
/**
    @class nInstanceStream
    @ingroup Gfx2

    Instance streams provide the fastest way to render multiple instances
    of the current mesh.

    The Nebula2 Stream Renderer
    ===========================
    Using instance streams one can render many instances of one mesh
    with one call into nGfxServer2. This makes instance rendering
    more efficient even with current graphics cards, and provides maximum
    efficiency for shader model 3.0 capable cards and DX 9.0c where
    instancing can happen entirely on the GPU.

    A stream element in the instance stream contains the shader parameters
    needed for rendering a new instance of the current mesh with the
    current shader.

    The simplest possible instance stream provides just a modelview
    matrix for each instance to render, but any number of shader
    parameters can be defined in the stream.

    Shader parameters which may be included in instance streams must
    be specifically marked in the shader file by a special annotation.
    This is an optimization which allows to distinguish between shader
    parameters which are identical across an entire instance set, and
    shader parameters which may change for each instance.

    (C) 2004 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "gfx2/nshaderstate.h"
#include "resource/nresource.h"

class nGfxServer2;

//------------------------------------------------------------------------------
class nInstanceStream : public nResource
{
public:
    /// a stream component
    class Component
    {
    public:
        /// default constructor
        Component();
        /// constructor
        Component(nShaderState::Type t, nShaderState::Param p);
        /// get byte offset into stream
        ushort GetOffset() const;
        /// get component data type
        nShaderState::Type GetType() const;
        /// get component parameter name
        nShaderState::Param GetParam() const;

    private:
        /// set byte offset
        void SetOffset(ushort o);

        friend class nInstanceStream;

        nShaderState::Type type;
        nShaderState::Param param;
        ushort offset;
    };

    /// type for stream declaration
    typedef nArray<Component> Declaration;

    /// lock types
    enum LockFlags
    {
        Read = (1<<0),
        Write = (1<<1),
        Append = (1<<2),
    };

    /// constructor
    nInstanceStream();
    /// destructor
    virtual ~nInstanceStream();
    /// set stream declaration
    void SetDeclaration(const Declaration& d);
    /// get stream declaration
    const Declaration& GetDeclaration() const;
    /// get the stream stride (in floats)
    int GetStride() const;
    /// get current number of valid elements in stream
    int GetCurrentSize() const;
    /// lock the instance stream
    float* Lock(int flags);
    /// unlock the instance stream
    void Unlock();
    /// return true if currently locked
    bool IsLocked() const;
    /// write a matrix44 value to the stream
    void WriteMatrix44(const matrix44& val);
    /// write a float4 value to the stream
    void WriteFloat4(const nFloat4& val);
    /// write a vector3 value to the stream (will be written as a float4)
    void WriteVector3(const vector3& val);
    /// write a float value to the stream
    void WriteFloat(float val);
    /// read a matrix44 value from the stream
    const matrix44& ReadMatrix44();
    /// read a float4 value from the stream
    const nFloat4& ReadFloat4();
    /// read a float value from the stream
    float ReadFloat();
    /// get an estimated byte size of the resource data (for memory statistics)
    virtual int GetByteSize();

protected:
    /// initialize stream buffer
    virtual bool LoadResource();
    /// clear stream buffer
    virtual void UnloadResource();

private:
    Declaration streamDecl;
    int lockFlags;                      // current lock type
    int streamStride;                   // float stride of stream
    nArray<float>::iterator readPtr;    // read pointer (valid during Read Lock)
    nArray<float> streamArray;
};

//------------------------------------------------------------------------------
/**
*/
inline
nInstanceStream::Component::Component() :
    type(nShaderState::Void),
    param(nShaderState::InvalidParameter),
    offset(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
nInstanceStream::Component::Component(nShaderState::Type t, nShaderState::Param p) :
    type(t),
    param(p),
    offset(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nInstanceStream::Component::SetOffset(ushort o)
{
    this->offset = o;
}

//------------------------------------------------------------------------------
/**
*/
inline
ushort
nInstanceStream::Component::GetOffset() const
{
    return this->offset;
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderState::Type
nInstanceStream::Component::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline
nShaderState::Param
nInstanceStream::Component::GetParam() const
{
    return this->param;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nInstanceStream::SetDeclaration(const Declaration& decl)
{
    n_assert(this->IsUnloaded());
    this->streamDecl = decl;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nInstanceStream::Declaration&
nInstanceStream::GetDeclaration() const
{
    return this->streamDecl;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nInstanceStream::GetStride() const
{
    return this->streamStride;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nInstanceStream::GetCurrentSize() const
{
    return this->streamArray.Size() / this->streamStride;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nInstanceStream::WriteMatrix44(const matrix44& val)
{
    n_assert(Write & this->lockFlags);
    nArray<float>::iterator ptr = this->streamArray.Reserve(16);
    memcpy(ptr, &val, 16 * sizeof(float));
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nInstanceStream::WriteFloat4(const nFloat4& val)
{
    n_assert(Write & this->lockFlags);
    nArray<float>::iterator ptr = this->streamArray.Reserve(4);
    memcpy(ptr, &val, 4 * sizeof(float));
}

//------------------------------------------------------------------------------
/**
    Writes a vector3 to the stream expanded to a float4 (x, y, z, 0.0).
*/
inline
void
nInstanceStream::WriteVector3(const vector3& val)
{
    n_assert(Write & this->lockFlags);
    nArray<float>::iterator ptr = this->streamArray.Reserve(4);
    *ptr++ = val.x;
    *ptr++ = val.y;
    *ptr++ = val.z;
    *ptr++ = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nInstanceStream::WriteFloat(float val)
{
    n_assert(Write & this->lockFlags);
    this->streamArray.Append(val);
}

//------------------------------------------------------------------------------
/**
*/
inline
const matrix44&
nInstanceStream::ReadMatrix44()
{
    const matrix44& val = *(matrix44*)this->readPtr;
    this->readPtr += 16;
    return val;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nFloat4&
nInstanceStream::ReadFloat4()
{
    const nFloat4& val = *(nFloat4*)this->readPtr;
    this->readPtr += 4;
    return val;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nInstanceStream::ReadFloat()
{
    return *this->readPtr++;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nInstanceStream::IsLocked() const
{
    return (0 != (this->lockFlags & 0xffff));
}

//------------------------------------------------------------------------------
#endif
