//------------------------------------------------------------------------------
//  gfx2/nmousecursor.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/nmousecursor.h"
#include "gfx2/ngfxserver2.h"

//------------------------------------------------------------------------------
/**
*/
nMouseCursor::nMouseCursor() :
    hotSpotX(0),
    hotSpotY(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
nMouseCursor::Delete()
{
    this->filename = 0;
    if (this->refTexture.isvalid())
    {
        this->refTexture->Release();
        this->refTexture.invalidate();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
nMouseCursor::Copy(const nMouseCursor& rhs)
{
    n_assert(!this->refTexture.isvalid());
    this->filename   = rhs.filename;
    this->hotSpotX   = rhs.hotSpotX;
    this->hotSpotY   = rhs.hotSpotY;
    this->refTexture = rhs.refTexture;
    if (this->refTexture.isvalid())
    {
        this->refTexture->AddRef();
    }
}

//------------------------------------------------------------------------------
/**
*/
nMouseCursor::~nMouseCursor()
{
    this->Delete();
}

//------------------------------------------------------------------------------
/**
*/
void
nMouseCursor::operator=(const nMouseCursor& rhs)
{
    this->Delete();
    this->Copy(rhs);
}

//------------------------------------------------------------------------------
/**
*/
bool
nMouseCursor::IsLoaded() const
{
    return this->refTexture.isvalid();
}

//------------------------------------------------------------------------------
/**
*/
bool
nMouseCursor::Load()
{
    n_assert(!this->IsLoaded());
    n_assert(!this->filename.IsEmpty());
    this->refTexture = nGfxServer2::Instance()->NewTexture(this->filename);
    n_assert(this->refTexture.isvalid());
    if (this->refTexture->IsUnloaded())
    {
        this->refTexture->SetFilename(this->filename);
        bool success = this->refTexture->Load();
        return success;
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
nMouseCursor::Unload()
{
    n_assert(this->IsLoaded());
    this->refTexture->Release();
    this->refTexture.invalidate();
}


//------------------------------------------------------------------------------
/**
    create empty mouse cursor
*/
void
nMouseCursor::CreateEmpty(int width, int height) {
    this->refTexture = nGfxServer2::Instance()->NewTexture(0);
    this->refTexture->SetUsage(nTexture2::CreateEmpty);
    this->refTexture->SetType(nTexture2::TEXTURE_2D);
    this->refTexture->SetFormat(nTexture2::A8R8G8B8);
    this->refTexture->SetWidth(width);
    this->refTexture->SetHeight(height);
    this->refTexture->Load();
}

//------------------------------------------------------------------------------
/**
*/
void
nMouseCursor::SetFilename(const char* path)
{
    n_assert(path);
    this->filename = path;
    if (this->IsLoaded())
    {
        this->Unload();
    }
}

//------------------------------------------------------------------------------
/**
*/
const char*
nMouseCursor::GetFilename() const
{
    return this->filename.IsEmpty() ? 0 : this->filename.Get();
}

//------------------------------------------------------------------------------
/**
*/
nTexture2*
nMouseCursor::GetTexture() const
{
    return this->refTexture.isvalid() ? this->refTexture.get() : 0;
}

//------------------------------------------------------------------------------
/**
*/
void
nMouseCursor::SetHotspotX(int x)
{
    this->hotSpotX = x;
}

//------------------------------------------------------------------------------
/**
*/
int
nMouseCursor::GetHotspotX() const
{
    return this->hotSpotX;
}

//------------------------------------------------------------------------------
/**
*/
void
nMouseCursor::SetHotspotY(int y)
{
    this->hotSpotY = y;
}

//------------------------------------------------------------------------------
/**
*/
int
nMouseCursor::GetHotspotY() const
{
    return this->hotSpotY;
}

