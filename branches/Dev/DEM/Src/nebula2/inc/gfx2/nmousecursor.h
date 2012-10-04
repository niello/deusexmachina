#ifndef N_MOUSECURSOR_H
#define N_MOUSECURSOR_H
//------------------------------------------------------------------------------
/**
    @class nMouseCursor
    @ingroup Gfx2

    Holds mouse cursor attributes.

    (C) 2003 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "gfx2/ntexture2.h"

class nGfxServer2;

//------------------------------------------------------------------------------
class nMouseCursor
{
public:
    /// constructor
    nMouseCursor();
    /// copy constructor
    nMouseCursor(const nMouseCursor& rhs);
    /// destructor
    ~nMouseCursor();
    /// copy operator
    void operator=(const nMouseCursor& rhs);
    /// create empty mouse cursor
    void CreateEmpty(int width, int height);
    /// set cursor image filename
    void SetFilename(const char* path);
    /// get cursor image filename
    const char* GetFilename() const;
    /// load cursor image
    bool Load();
    /// unload cursor image
    void Unload();
    /// is cursor image loaded?
    bool IsLoaded() const;
    /// set hotspot x
    void SetHotspotX(int x);
    /// get hotspot x
    int GetHotspotX() const;
    /// set hotspot y
    void SetHotspotY(int y);
    /// get hotspot y
    int GetHotspotY() const;
    /// get cursor image texture (only valid after Load())
    nTexture2* GetTexture() const;

private:
    /// delete contents
    void Delete();
    /// copy contents
    void Copy(const nMouseCursor& rhs);

    nString filename;
    nRef<nTexture2> refTexture;
    int hotSpotX;
    int hotSpotY;
};

//------------------------------------------------------------------------------
#endif


