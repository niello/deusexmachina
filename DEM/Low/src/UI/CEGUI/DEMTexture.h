#pragma once
#ifndef __DEM_L1_CEGUI_TEXTURE_H__
#define __DEM_L1_CEGUI_TEXTURE_H__

#include <UI/CEGUI/DEMRenderer.h>
#include <CEGUI/Texture.h>
#include <Data/RefCounted.h>
#include <CEGUI/Sizef.h>

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
	glm::vec2			TexelScaling;
	const String		Name;

	CDEMTexture(CDEMRenderer& Renderer, const String& name);
	virtual ~CDEMTexture();

	void updateCachedScaleValues();
	void updateTextureSize();

public:

	void					setTexture(Render::CTexture* tex);
	Render::CTexture*		getTexture() const { return DEMTexture.Get(); }
	void					setOriginalDataSize(const Sizef& sz) { DataSize = sz; updateCachedScaleValues(); }

	void					createEmptyTexture(const Sizef& sz);

	// implement abstract members from base class.
	virtual const String&	getName() const override { return Name; }
	virtual const Sizef&	getSize() const override { return Size; }
	virtual const Sizef&	getOriginalDataSize() const override { return DataSize;}
	virtual const glm::vec2& getTexelScaling() const override { return TexelScaling; }
	virtual void			loadFromFile(const String& filename, const String& resourceGroup) override;
	virtual void			loadFromMemory(const void* buffer, const Sizef& buffer_size, PixelFormat pixel_format) override;
	virtual void			blitFromMemory(const void* sourceData, const Rectf& area) override;
	virtual void			blitToMemory(void* targetData) override;
	virtual bool			isPixelFormatSupported(const PixelFormat fmt) const override;
};

}

#endif
