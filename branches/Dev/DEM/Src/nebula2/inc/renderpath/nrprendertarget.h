#ifndef N_RPRENDERTARGET_H
#define N_RPRENDERTARGET_H
//------------------------------------------------------------------------------
/**
    @class nRpRenderTarget
    @ingroup RenderPath
    @brief Represents a render target in a render path.

    (C) 2004 RadonLabs GmbH
*/
#include "gfx2/ntexture2.h"

//------------------------------------------------------------------------------
class nRpRenderTarget
{
public:

	nRpRenderTarget();
	nRpRenderTarget(const nRpRenderTarget& rhs) { Copy(rhs); }
    ~nRpRenderTarget();

	void Copy(const nRpRenderTarget& rhs);
		
	void operator=(const nRpRenderTarget& rhs) { Copy(rhs); }
    /// set the render target's name
    void SetName(const nString& n);
    /// get the render targets name
    const nString& GetName() const;
    /// set the render target's pixel format
    void SetFormat(nTexture2::Format f);
    /// get the render target's pixel format
    nTexture2::Format GetFormat() const;
    /// set display-relative size of render target
    void SetRelSize(float s);
    /// get display-relative size
    float GetRelSize() const;
    /// set optional absolute width (overrides relative size)
    void SetWidth(int w);
    /// get optional absolute width
    int GetWidth() const;
    /// set optional absolute height (overrides relative size)
    void SetHeight(int h);
    /// get optional absolute height
    int GetHeight() const;
    /// validate the render target
    void Validate();
    /// get pointer to render target texture
    nTexture2* GetTexture() const;

private:
    nString name;
    nTexture2::Format format;
    float relSize;
    int width;
    int height;
    nRef<nTexture2> refTexture;
};

//------------------------------------------------------------------------------
/**
*/
inline void nRpRenderTarget::Copy(const nRpRenderTarget& rhs)
{
    this->name       = rhs.name;
    this->format     = rhs.format;
    this->relSize    = rhs.relSize;
    this->width      = rhs.width;
    this->height     = rhs.height;
    this->refTexture = rhs.refTexture;
    if (this->refTexture.isvalid())
    {
        this->refTexture->AddRef();
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpRenderTarget::SetWidth(int w)
{
    this->width = w;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nRpRenderTarget::GetWidth() const
{
    return this->width;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpRenderTarget::SetHeight(int h)
{
    this->height = h;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nRpRenderTarget::GetHeight() const
{
    return this->height;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpRenderTarget::SetName(const nString& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nRpRenderTarget::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpRenderTarget::SetFormat(nTexture2::Format f)
{
    this->format = f;
}

//------------------------------------------------------------------------------
/**
*/
inline
nTexture2::Format
nRpRenderTarget::GetFormat() const
{
    return this->format;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nRpRenderTarget::SetRelSize(float s)
{
    this->relSize = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nRpRenderTarget::GetRelSize() const
{
    return this->relSize;
}

//------------------------------------------------------------------------------
/**
*/
inline
nTexture2*
nRpRenderTarget::GetTexture() const
{
    return this->refTexture;
}

//------------------------------------------------------------------------------
#endif
