/***********************************************************************
    created:    Fri Jan 15 2010
    author:     Eugene Marcotte
*************************************************************************/
/***************************************************************************
 *   Copyright (C) 2004 - 2010 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 ***************************************************************************/
#ifndef _CEGUINullGeometryBuffer_h_
#define _CEGUINullGeometryBuffer_h_

#include "CEGUI/GeometryBuffer.h"
#include "CEGUI/RendererModules/Null/Renderer.h"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4251)
#endif

namespace CEGUI
{

//! Implementation of CEGUI::GeometryBuffer for the Null engine
class NULL_GUIRENDERER_API NullGeometryBuffer : public GeometryBuffer
{
public:

    NullGeometryBuffer(CEGUI::RefCounted<RenderMaterial> renderMaterial);

    // Implementation/overrides of member functions inherited from GeometryBuffer
    void draw(uint32_t drawModeMask = DrawModeMaskAll) const override;
};


}

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif
