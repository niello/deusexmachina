#pragma once
#ifndef __DEM_L1_CEGUI_RENDERER_H__
#define __DEM_L1_CEGUI_RENDERER_H__

#include <Data/RefCounted.h>
#include <Data/HashTable.h>
#include <UI/CEGUI/DEMFwd.h>

#include <CEGUI/Renderer.h>

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
	typedef Ptr<class CVertexLayout> PVertexLayout;
	typedef Ptr<class CVertexBuffer> PVertexBuffer;
	typedef Ptr<class CRenderState> PRenderState;
}

namespace CEGUI
{
class CDEMGeometryBuffer;
class CDEMTextureTarget;
class CDEMTexture;

class CDEMRenderer: public Renderer
{
protected:

	static String						RendererID;

	Render::PGPUDriver					GPU;
	int									SwapChainID;

	Sizef								DisplaySize;
	Vector2f							DisplayDPI;

	CArray<CDEMGeometryBuffer*>			GeomBuffers;
	CArray<CDEMTextureTarget*>			TexTargets;
	CHashTable<String, CDEMTexture*>	Textures;

	RenderTarget*						pDefaultRT;
	Render::PVertexLayout				VertexLayout;
	Render::PRenderState				NormalUnclipped;
	Render::PRenderState				NormalClipped;
	Render::PRenderState				PremultipliedUnclipped;
	Render::PRenderState				PremultipliedClipped;

	CDEMRenderer(Render::CGPUDriver& GPUDriver, int SwapChain, const char* pVertexShaderURI, const char* pPixelShaderURI);
	virtual ~CDEMRenderer();

	static void logTextureCreation(const String& name);
	static void logTextureDestruction(const String& name);

	/*
	ID3DX11EffectShaderResourceVariable* d_boundTextureVariable;
	ID3DX11EffectMatrixVariable* d_worldMatrixVariable;
	ID3DX11EffectMatrixVariable* d_projectionMatrixVariable;
	*/

public:

	static CDEMRenderer&	create(Render::CGPUDriver& GPUDriver, int SwapChain, const char* pVertexShaderURI, const char* pPixelShaderURI, const int abi = CEGUI_VERSION_ABI);
	static void				destroy(CDEMRenderer& renderer);

	Render::CGPUDriver*		getGPUDriver() { return GPU.GetUnsafe(); }
	Render::PVertexBuffer	createVertexBuffer(D3DVertex* pVertexData, DWORD VertexCount);
	void					setRenderState(BlendMode BlendMode, bool Clipped);

	// Implement interface from Renderer
	virtual RenderTarget&	getDefaultRenderTarget() { return *pDefaultRT; }
	virtual GeometryBuffer&	createGeometryBuffer();
	virtual void			destroyGeometryBuffer(const GeometryBuffer& buffer);
	virtual void			destroyAllGeometryBuffers();
	virtual TextureTarget*	createTextureTarget();
	virtual void			destroyTextureTarget(TextureTarget* target);
	virtual void			destroyAllTextureTargets();
	virtual Texture&		createTexture(const String& name);
	virtual Texture&		createTexture(const String& name, const String& filename, const String& resourceGroup);
	virtual Texture&		createTexture(const String& name, const Sizef& size);
	virtual void			destroyTexture(Texture& texture);
	virtual void			destroyTexture(const String& name);
	virtual void			destroyAllTextures();
	virtual Texture&		getTexture(const String& name) const;
	virtual bool			isTextureDefined(const String& name) const { return Textures.Contains(name); }
	virtual void			beginRendering();
	virtual void			endRendering();
	virtual void			setDisplaySize(const Sizef& sz);
	virtual const Sizef&	getDisplaySize() const { return DisplaySize; }
	virtual const Vector2f&	getDisplayDPI() const { return DisplayDPI; }
	virtual uint			getMaxTextureSize() const;
	virtual const String&	getIdentifierString() const { return RendererID; }
};

}

template<> inline unsigned int Hash<CEGUI::String>(const CEGUI::String& Key)
{
	return Hash(Key.c_str(), Key.size());
}
//---------------------------------------------------------------------

#endif