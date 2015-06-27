#pragma once
#ifndef __DEM_L1_CEGUI_TEXTURE_H__
#define __DEM_L1_CEGUI_TEXTURE_H__

#include <CEGUI/Texture.h>
#include <Data/RefCounted.h>

namespace Render
{
	typedef Ptr<class CTexture> PTexture;
}

namespace CEGUI
{

class CDEMTexture: public Texture
{
protected:

	//friend Texture& CDEMRenderer::createTexture(const String&);
	//friend Texture& CDEMRenderer::createTexture(const String&, const String&, const String&);
	//friend Texture& CDEMRenderer::createTexture(const String&, const Sizef&);
	//friend void CDEMRenderer::destroyTexture(Texture&);
	//friend void CDEMRenderer::destroyTexture(const String&);

	Render::PTexture	DEMTexture;
	Sizef				Size;			// tex size //???get from desc?
	Sizef				DataSize;		// size of original data loaded to tex
	Vector2f			TexelScaling;
	const String		Name;

	//CDEMTexture(IDevice11& device, const String& name);
	//CDEMTexture(IDevice11& device, const String& name, const String& filename, const String& resourceGroup);
	//CDEMTexture(IDevice11& device, const String& name, const Sizef& sz);
	//CDEMTexture(IDevice11& device, const String& name, ID3D11Texture2D* tex);
	//virtual ~CDEMTexture() { }

	void updateCachedScaleValues();
	void updateTextureSize();

public:

	void					setTexture(Render::CTexture* tex);
	Render::CTexture*		getTexture() const { return DEMTexture.GetUnsafe(); }
	void					setOriginalDataSize(const Sizef& sz) { DataSize = sz; updateCachedScaleValues(); }

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

}

#endif