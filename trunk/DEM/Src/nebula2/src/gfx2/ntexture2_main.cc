//------------------------------------------------------------------------------
//  ntexture2_main.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/ntexture2.h"
#include "kernel/nkernelserver.h"

nNebulaClass(nTexture2, "nresource");

//------------------------------------------------------------------------------
/**
*/
nTexture2::nTexture2() :
    type(TEXTURE_NOTYPE),
    format(NOFORMAT),
    width(0),
    height(0),
    depth(0),
    numMipMaps(0),
    usage(0),
    compoundFile(0),
    compoundFilePos(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nTexture2::~nTexture2()
{
    if (this->IsLoaded())
    {
        this->Unload();
    }
    if (this->compoundFile)
    {
        this->compoundFile = 0;
    }
}

//------------------------------------------------------------------------------
/**
    Locks a mipmap level of the texture, returns a pointer to the surface
    data and the surface pitch in the provided lockInfo structure.
*/
bool
nTexture2::Lock(LockType /*lockType*/, int /*level*/, LockInfo& /*lockInfo*/)
{
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
nTexture2::Unlock(int /*level*/)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Locks a single surface of a cube texture.
*/
bool
nTexture2::LockCubeFace(LockType /*lockType*/, CubeFace /*face*/, int /*level*/, LockInfo& /*lockInfo*/)
{
    return false;
}


//------------------------------------------------------------------------------
/**
    Unlocks a single surface of a cube texture.
*/
void
nTexture2::UnlockCubeFace(CubeFace /*face*/, int /*level*/)
{
    // empty
}


//------------------------------------------------------------------------------
/**
    Saves the texture to a file
*/
bool
nTexture2::SaveTextureToFile(const nString& filename, FileFormat fileFormat)
{
    return false;
};

//------------------------------------------------------------------------------
/**
    - Feb-04 Kim, H.W. added to support ngameswf.
*/
void nTexture2::GenerateMipMaps()
{
    // reimplemented in derived class's member function.
}
