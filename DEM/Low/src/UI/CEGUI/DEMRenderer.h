#pragma once
#include <Data/RefCounted.h>
#include <Data/Array.h>
#include <Data/Hash.h>
#include <Render/RenderFwd.h>

#include <CEGUI/Renderer.h>
#include <CEGUI/Sizef.h>

namespace CEGUI
{
class CDEMGeometryBuffer;
class CDEMTextureTarget;
class CDEMViewportTarget;
class CDEMTexture;
class CDEMShaderWrapper;

class CDEMRenderer: public Renderer
{
protected:

	static String						RendererID;

	Render::PGPUDriver					GPU;

	Sizef								DisplaySize;

	CArray<CDEMTextureTarget*>			TexTargets;
	CArray<RenderTarget*>				VPTargets;
	std::unordered_map<String, CDEMTexture*> Textures;

	Render::PVertexLayout				VertexLayoutTextured;
	Render::PVertexLayout				VertexLayoutColoured;
	CDEMShaderWrapper*					pShaderWrapperTextured = nullptr;
	CDEMShaderWrapper*					pShaderWrapperColoured = nullptr;

	CDEMRenderer(Render::CGPUDriver& GPUDriver);
	virtual ~CDEMRenderer() override;

	static void logTextureCreation(const String& name);
	static void logTextureDestruction(const String& name);

public:

	static CDEMRenderer&	create(Render::CGPUDriver& GPUDriver, const int abi = CEGUI_VERSION_ABI);
	static void				destroy(CDEMRenderer& renderer);

	Render::CGPUDriver*		getGPUDriver() { return GPU.Get(); }
	RenderTarget*			createViewportTarget(float width, float height); // TODO: to base class!
	void					destroyViewportTarget(RenderTarget* target);

	void                    setEffects(CDEMShaderWrapper* pTextured, CDEMShaderWrapper* pColoured);

	// Implement interface from Renderer
	virtual RenderTarget&	getDefaultRenderTarget() override;
	virtual RefCounted<RenderMaterial> createRenderMaterial(const DefaultShaderType shaderType) const override;
	virtual GeometryBuffer& createGeometryBufferTextured(RefCounted<RenderMaterial> renderMaterial) override;
	virtual GeometryBuffer& createGeometryBufferColoured(RefCounted<RenderMaterial> renderMaterial) override;
	virtual TextureTarget*	createTextureTarget(bool addStencilBuffer) override;
	virtual void			destroyTextureTarget(TextureTarget* target) override;
	virtual void			destroyAllTextureTargets() override;
	virtual Texture&		createTexture(const String& name) override;
	virtual Texture&		createTexture(const String& name, const String& filename, const String& resourceGroup) override;
	virtual Texture&		createTexture(const String& name, const Sizef& size) override;
	virtual void			destroyTexture(Texture& texture) override;
	virtual void			destroyTexture(const String& name) override;
	virtual void			destroyAllTextures() override;
	virtual Texture&		getTexture(const String& name) const override;
	virtual bool			isTextureDefined(const String& name) const override { return Textures.find(name) != Textures.cend(); }
	virtual void			beginRendering() override;
	virtual void			endRendering() override;
	virtual void			setDisplaySize(const Sizef& sz) override;
	virtual const Sizef&	getDisplaySize() const override { return DisplaySize; }
	virtual unsigned int	getMaxTextureSize() const override;
	virtual const String&	getIdentifierString() const override { return RendererID; }
	virtual bool			isTexCoordSystemFlipped() const override { return false; }
};

}

template<> inline unsigned int Hash<CEGUI::String>(const CEGUI::String& Key)
{
	return Hash(Key.c_str(), Key.size());
}
//---------------------------------------------------------------------
