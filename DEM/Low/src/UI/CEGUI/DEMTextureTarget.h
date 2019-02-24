#pragma once
#include <UI/CEGUI/DEMRenderTarget.h>
#include <CEGUI/TextureTarget.h>
#include <Data/RefCounted.h>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4250)
#endif

namespace Render
{
	typedef Ptr<class CRenderTarget> PRenderTarget;
	typedef Ptr<class CDepthStencilBuffer> PDepthStencilBuffer;
}

namespace CEGUI
{
class CDEMRenderer;
class CDEMTexture;

class CDEMTextureTarget : public CDEMRenderTarget, public TextureTarget
{
protected:

	static const float			DEFAULT_SIZE;
	static UPTR					s_textureNumber;
	static String				generateTextureName();

	Render::PRenderTarget		RT;
	CDEMTexture*				d_CEGUITexture;
	Render::PRenderTarget		PrevRT;
	Render::PDepthStencilBuffer	PrevDS;

	void initialiseRenderTexture();
	void cleanupRenderTexture();
	void resizeRenderTexture();
	void enableRenderTexture();
	void disableRenderTexture();

public:

	CDEMTextureTarget(CDEMRenderer& owner, bool addStencilBuffer, const float w = DEFAULT_SIZE, const float h = DEFAULT_SIZE);
	virtual ~CDEMTextureTarget();

	// overrides from CDEMRenderTarget
	virtual void		activate() override;
	virtual void		deactivate() override;

	// implementation of RenderTarget interface
	virtual bool		isImageryCache() const override { return true; }

	// implementation of TextureTarget interface
	virtual void		clear() override;
	virtual Texture&	getTexture() const override;
	virtual void		declareRenderSize(const Sizef& sz) override;
};

}

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif
