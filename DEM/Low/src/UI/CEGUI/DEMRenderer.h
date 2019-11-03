#pragma once
#include <Data/RefCounted.h>
#include <Data/HashTable.h>
#include <Render/RenderFwd.h>

#include <CEGUI/Renderer.h>

namespace CEGUI
{
class CDEMGeometryBuffer;
class CDEMTextureTarget;
class CDEMViewportTarget;
class CDEMTexture;
typedef std::unique_ptr<class CDEMShaderWrapper> PDEMShaderWrapper;

class CDEMRenderer: public Renderer
{
protected:

	static String						RendererID;

	Render::PGPUDriver					GPU;

	Sizef								DisplaySize;

	CArray<CDEMTextureTarget*>			TexTargets;
	CArray<RenderTarget*>				VPTargets;
	CHashTable<String, CDEMTexture*>	Textures;

	Render::PVertexLayout				VertexLayoutTextured;
	Render::PVertexLayout				VertexLayoutColoured;
	PDEMShaderWrapper					ShaderWrapperTextured;
	PDEMShaderWrapper					ShaderWrapperColoured;

	CDEMRenderer(Render::CGPUDriver& GPUDriver, Render::CEffect& Effect);
	virtual ~CDEMRenderer() override;

	static void logTextureCreation(const String& name);
	static void logTextureDestruction(const String& name);

public:

	static CDEMRenderer&	create(Render::CGPUDriver& GPUDriver, Render::CEffect& Effect, const int abi = CEGUI_VERSION_ABI);
	static void				destroy(CDEMRenderer& renderer);

	Render::CGPUDriver*		getGPUDriver() { return GPU.Get(); }
	RenderTarget*			createViewportTarget(float width, float height);
	void					destroyViewportTarget(RenderTarget* target);

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
	virtual bool			isTextureDefined(const String& name) const override { return Textures.Contains(name); }
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
