#pragma once
#ifndef __DEM_L1_CEGUI_TEXTURE_TARGET_H__
#define __DEM_L1_CEGUI_TEXTURE_TARGET_H__

#include <UI/CEGUI/DEMRenderTarget.h>
#include <CEGUI/TextureTarget.h>
#include <Data/RefCounted.h>

namespace Render
{
	typedef Ptr<class CRenderTarget> PRenderTarget;
	typedef Ptr<class CDepthStencilBuffer> PDepthStencilBuffer;
}

namespace CEGUI
{
class CDEMRenderer;
class CDEMTexture;

class CDEMTextureTarget: public CDEMRenderTarget<TextureTarget>
{
protected:

	static const float			DEFAULT_SIZE;
	static unsigned int			s_textureNumber;
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

	CDEMTextureTarget(CDEMRenderer& owner, const float size = DEFAULT_SIZE);
	virtual ~CDEMTextureTarget();

	// overrides from CDEMRenderTarget
	virtual void		activate();
	virtual void		deactivate();

	// implementation of RenderTarget interface
	virtual bool		isImageryCache() const { return true; }

	// implementation of TextureTarget interface
	virtual void		clear();
	virtual Texture&	getTexture() const;
	virtual void		declareRenderSize(const Sizef& sz);
	virtual bool		isRenderingInverted() const { return false; }
};

}

#endif