#pragma once
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

	friend class CDEMRenderer;

	CDEMRenderer&		_Owner;
	Render::PTexture	_DEMTexture;
	Sizef				_Size;
	glm::vec2			_TexelScaling;
	const String		_Name;

	CDEMTexture(CDEMRenderer& Renderer, const String& Name);
	virtual ~CDEMTexture() override;

	void updateTextureSize();

public:

	void					setTexture(Render::CTexture* pTexture);
	Render::CTexture*		getTexture() const { return _DEMTexture.Get(); }

	void					createEmptyTexture(const Sizef& sz);

	virtual const String&	getName() const override { return _Name; }
	virtual const Sizef&	getSize() const override { return _Size; }
	virtual const Sizef&	getOriginalDataSize() const override { return _Size;}
	virtual const glm::vec2& getTexelScaling() const override { return _TexelScaling; }
	virtual void			loadFromFile(const String& filename, const String& resourceGroup) override;
	virtual void			loadFromMemory(const void* buffer, const Sizef& buffer_size, PixelFormat pixel_format) override;
	virtual void			blitFromMemory(const void* sourceData, const Rectf& area) override;
	virtual void			blitToMemory(void* targetData) override;
	virtual bool			isPixelFormatSupported(const PixelFormat fmt) const override;
};

}
