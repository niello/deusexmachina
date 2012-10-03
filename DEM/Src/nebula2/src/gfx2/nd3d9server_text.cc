//------------------------------------------------------------------------------
//  nd3d9server_text.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "gfx2/nd3d9server.h"
#include "gfx2/nd3d9texture.h"

//------------------------------------------------------------------------------
/**
    Public text rendering routine. Either draws the text immediately,
    or stores the text internally in a text element array and draws it
    towards the end of frame.
*/
void nD3D9Server::DrawText(const nString& text, const vector4& color, const rectangle& rect, uint flags, bool immediate)
{
    if (immediate) DrawTextImmediate(text, color, rect, flags);
    else textElements.Append(TextElement(text, color, rect, flags));
}

//------------------------------------------------------------------------------
/**
    Draws the accumulated text elements and flushes the text buffer.
*/
void
nD3D9Server::DrawTextBuffer()
{
    for (int i = 0; i < textElements.Size(); i++)
    {
        const TextElement& textElement = this->textElements[i];
        this->DrawTextImmediate(textElement.text, textElement.color, textElement.rect, textElement.flags);
    }
    this->textElements.Clear();
}

//------------------------------------------------------------------------------
/**
    Internal text rendering routine.

    @param  text    the text to draw
    @param  color   the text color
    @param  rect    screen space rectangle in which to draw the text
    @param  flags   combination of CFont::RenderFlags
*/
void
nD3D9Server::DrawTextImmediate(const nString& text, const vector4& color, const rectangle& rect, uint flags)
{
    if (!text.IsValid())
    {
        // nothing to do.
        return;
    }

    float dispWidth  = (float)Display.GetDisplayMode().Width;
    float dispHeight = (float)Display.GetDisplayMode().Height;
    RECT r;
    r.left   = (LONG)(rect.v0.x * dispWidth);
    r.top    = (LONG)(rect.v0.y * dispHeight);
    r.right  = (LONG)(rect.v1.x * dispWidth);
    r.bottom = (LONG)(rect.v1.y * dispHeight);

    DWORD d3dFlags = 0;
    nString wordBreakString;
    if (flags & Bottom)     d3dFlags |= DT_BOTTOM;
    if (flags & Top)        d3dFlags |= DT_TOP;
    if (flags & Center)     d3dFlags |= DT_CENTER;
    if (flags & Left)       d3dFlags |= DT_LEFT;
    if (flags & Right)      d3dFlags |= DT_RIGHT;
    if (flags & VCenter)    d3dFlags |= DT_VCENTER;
    if (flags & NoClip)     d3dFlags |= DT_NOCLIP;
    if (flags & ExpandTabs) d3dFlags |= DT_EXPANDTABS;

    DWORD d3dColor = D3DCOLOR_COLORVALUE(color.x, color.y, color.z, color.w);
    n_assert(pD3DFont);
    this->pD3DXSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE);
    if (flags & WordBreak) d3dFlags |= DT_WORDBREAK;
    pD3DFont->DrawText(this->pD3DXSprite, text.Get(), -1, &r, d3dFlags, d3dColor);
    this->pD3DXSprite->End();
}

//------------------------------------------------------------------------------
/**
    Get text extents.

    - 16-Feb-04     floh    hmm, ID3DXFont extent computation is confused by spaces,
                            now uses GDI functions to compute text extents
*/
vector2
nD3D9Server::GetTextExtent(const nString& text)
{
    int width = 0;
    int height = 0;
    float dispWidth  = (float)Display.GetDisplayMode().Width;
    float dispHeight = (float)Display.GetDisplayMode().Height;

	RECT rect = { 0 };
    if (text.IsValid()) pD3DFont->DrawTextA(NULL, text.Get(), -1, &rect, DT_LEFT | DT_NOCLIP | DT_CALCRECT, 0);
    else
    {
        // hmm, an empty text should give us at least the correct height
        pD3DFont->DrawTextA(NULL, " ", -1, &rect, DT_LEFT | DT_NOCLIP | DT_CALCRECT, 0);
        rect.right = 0;
        rect.left = 0;
    }
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

	return vector2((float(width) / dispWidth), (float(height) / dispHeight));
}
