#pragma once
#ifndef __DEM_L1_CEGUI_TEXTURE_H__
#define __DEM_L1_CEGUI_TEXTURE_H__

#include <UI/CEGUI/DEMRenderer.h>
#include <CEGUI/Texture.h>
#include <Data/RefCounted.h>

namespace Render
{
	typedef Ptr<class CTexture> PTexture;
}

namespace CEGUI
{
class CDEMRenderer;

class CDEMTexture: public Texture
{
protected:

	friend Texture& CDEMRenderer::createTexture(const String&);
	friend Texture& CDEMRenderer::createTexture(const String&, const String&, const String&);
	friend Texture& CDEMRenderer::createTexture(const String&, const Sizef&);
	friend void CDEMRenderer::destroyTexture(const String&);
	friend void CDEMRenderer::destroyTexture(Texture&);
	friend void CDEMRenderer::destroyAllTextures();

	CDEMRenderer&		Owner;
	Render::PTexture	DEMTexture;
	Sizef				Size;			// tex size //???get from desc?
	Sizef				DataSize;		// size of original data loaded to tex
	Vector2f			TexelScaling;
	const String		Name;

	CDEMTexture(CDEMRenderer& Renderer, const String& name);
	virtual ~CDEMTexture() {}

	void updateCachedScaleValues();
	void updateTextureSize();

public:

	void					setTexture(Render::CTexture* tex);
	Render::CTexture*		getTexture() const { return DEMTexture.GetUnsafe(); }
	void					setOriginalDataSize(const Sizef& sz) { DataSize = sz; updateCachedScaleValues(); }

	void					createEmptyTexture(const Sizef& sz);

	// implement abstract members from base class.
	virtual const String&	getName() const { return Name; }
	virtual const Sizef&	getSize() const { return Size; }
	virtual const Sizef&	getOriginalDataSize() const { return DataSize;}
	virtual const Vector2f&	getTexelScaling() const { return TexelScaling; }
	virtual void			loadFromFile(const String& filename, const String& resourceGroup);
	virtual void			loadFromMemory(const void* buffer, const Sizef& buffer_size, PixelFormat pixel_format);
	virtual void			blitFromMemory(const void* sourceData, const Rectf& area);
	virtual void			blitToMemory(void* targetData);
	virtual bool			isPixelFormatSupported(const PixelFormat fmt) const;
};

inline CDEMTexture::CDEMTexture(CDEMRenderer& Renderer, const String& name):
	Owner(Renderer),
	Size(0, 0),
	DataSize(0, 0),
	TexelScaling(0, 0),
	Name(name)
{
}
//--------------------------------------------------------------------

}

#endif