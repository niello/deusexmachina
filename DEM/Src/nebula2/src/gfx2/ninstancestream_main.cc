//------------------------------------------------------------------------------
//  ninstancestream_main.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/ninstancestream.h"
#include "gfx2/ngfxserver2.h"

nNebulaClass(nInstanceStream, "nresource");

//------------------------------------------------------------------------------
/**
*/
nInstanceStream::nInstanceStream() :
    streamDecl(0, 0),
    lockFlags(0),
    streamStride(0),
    readPtr(0),
    streamArray(0, 128)
{
    this->streamArray.SetFlags(nArray<float>::DoubleGrowSize);
}

//------------------------------------------------------------------------------
/**
*/
nInstanceStream::~nInstanceStream()
{
    if (!this->IsUnloaded())
    {
        this->Unload();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
nInstanceStream::LoadResource()
{
    n_assert(this->IsUnloaded());
    n_assert(this->streamDecl.Size() > 0);
    n_assert(!this->IsLocked());
    n_assert(0 == this->readPtr);
    n_assert(this->streamArray.Empty());

    // update the stream stride and component offsets from the stream declaration
    int i;
    this->streamStride = 0;
    for (i = 0; i < this->streamDecl.Size(); i++)
    {
        this->streamDecl[i].offset = this->streamStride;
        switch (this->streamDecl[i].GetType())
        {
            case nShaderState::Float:     this->streamStride++; break;
            case nShaderState::Float4:    this->streamStride += 4; break;
            case nShaderState::Matrix44:  this->streamStride += 16; break;
            default:
                n_error("nInstanceStream: Invalid data type in stream declaration!");
                break;
        }
    }

    this->SetState(Valid);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
nInstanceStream::UnloadResource()
{
    n_assert(!this->IsUnloaded());
    n_assert(!this->IsLocked());
    n_assert(0 == this->readPtr);

    // first, check if I'm the current instance stream
    if (this == nGfxServer2::Instance()->GetInstanceStream())
    {
        nGfxServer2::Instance()->SetInstanceStream(0);
    }

    // release stream buffer
    this->streamArray.Clear();
    this->streamStride = 0;

    this->SetState(Unloaded);
}

//------------------------------------------------------------------------------
/**
*/
int
nInstanceStream::GetByteSize()
{
    if (this->IsValid())
    {
        return this->streamStride * this->streamArray.AllocSize() * sizeof(float);
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
    NOTE: direct write access to the stream is not allowed. You must
    use the Write*() methods to write data to the stream.
*/
float*
nInstanceStream::Lock(int flags)
{
    n_assert(!this->IsLocked());
    n_assert(0 != flags);
    n_assert(0 == this->readPtr);
    n_assert(this->IsValid());

    if (flags & Write)
    {
        // write mode, do not append
        this->streamArray.Reset();
    }
    else if (flags & Read)
    {
        n_assert(0 == (flags & Append));
        this->readPtr = this->streamArray.Begin();
    }
    this->lockFlags = flags;
    return this->readPtr;
}

//------------------------------------------------------------------------------
/**
*/
void
nInstanceStream::Unlock()
{
    n_assert(this->IsLocked());
    if (Read & this->lockFlags)
    {
        // check for array overflow (yes, a bit late...)
        n_assert(this->readPtr <= this->streamArray.End());
    }
    this->readPtr = 0;
    this->lockFlags = 0;
}
