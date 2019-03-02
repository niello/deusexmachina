#include "DEMRenderer.h"

#include <Render/GPUDriver.h>
#include <Render/RenderState.h>
#include <Render/RenderTarget.h>
#include <Render/Shader.h>
#include <Render/ShaderMetadata.h>
#include <Render/ShaderConstant.h>
#include <Render/SamplerDesc.h>
#include <Render/Sampler.h>
#include <Render/ConstantBuffer.h>
#include <Render/VertexBuffer.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <IO/PathUtils.h>
#include <IO/IOServer.h>
#include <Data/Buffer.h>
#include <Data/StringUtils.h>
#include "DEMGeometryBuffer.h"
#include "DEMTextureTarget.h"
#include "DEMViewportTarget.h"
#include "DEMTexture.h"
#include "DEMShaderWrapper.h"
#include "CEGUI/System.h"
#include "CEGUI/Logger.h"

namespace CEGUI
{
String CDEMRenderer::RendererID("CEGUI::CDEMRenderer - official DeusExMachina engine renderer by DEM team");

CDEMRenderer::CDEMRenderer(Render::CGPUDriver& GPUDriver,
						   float DefaultContextWidth, float DefaultContextHeight,
						   CStrID VertexShaderID, CStrID PixelShaderRegularID, CStrID PixelShaderOpaqueID):
	GPU(&GPUDriver),
	DisplaySize(DefaultContextWidth, DefaultContextHeight)
{
	n_assert(DefaultContextWidth > 0.f && DefaultContextHeight > 0.f);

	Render::PShader VS = GPU->GetShader(VertexShaderID);
	Render::PShader PSRegular = GPU->GetShader(PixelShaderRegularID);
	Render::PShader PSOpaque = GPU->GetShader(PixelShaderOpaqueID);
	n_assert(VS && PSRegular && PSOpaque && VS->IsValid() && PSRegular->IsValid() && PSOpaque->IsValid());

	// NB: regular and opaque pixel shaders must be compatible

	//???pass 3 shaders inside, create render states there etc?
	ShaderWrapperTextured.reset(new CDEMShaderWrapper(*this, VS, PSRegular, PSOpaque));

	//!!!TODO: non-textured shaders!
	//ShaderWrapperColoured = new CDEMShaderWrapper(*ShaderColoured, this);

	//???use Render::VCFmt_UInt8_4_Norm for color? convert on append vertices?
	Render::CVertexComponent Components[] = {
			{ Render::VCSem_Position, nullptr, 0, Render::VCFmt_Float32_3, 0, 0, false },
			{ Render::VCSem_Color, nullptr, 0, Render::VCFmt_Float32_4, 0, 12, false },
			{ Render::VCSem_TexCoord, nullptr, 0, Render::VCFmt_Float32_2, 0, 28, false } };

	VertexLayoutTextured = GPU->CreateVertexLayout(Components, 3);
	n_assert(VertexLayoutTextured.IsValidPtr());

	VertexLayoutColoured = GPU->CreateVertexLayout(Components, 2);
	n_assert(VertexLayoutColoured.IsValidPtr());

	//!!!need some method to precreate actual D3D11 input layout by passing a shader!
	//but this may require knowledge about D3D11 nature of otherwise abstract GPU
	//???call GPU->CreateRenderCache() and then reuse it?
}
//--------------------------------------------------------------------

CDEMRenderer::~CDEMRenderer()
{
	destroyAllTextureTargets();
	destroyAllTextures();
	destroyAllGeometryBuffers();
	SAFE_DELETE(pDefaultRT);

	ShaderWrapperTextured = nullptr;
	ShaderWrapperColoured = nullptr;
	VertexLayoutTextured = nullptr;
	VertexLayoutColoured = nullptr;
	GPU = nullptr;
}
//--------------------------------------------------------------------

CDEMRenderer& CDEMRenderer::create(Render::CGPUDriver& GPUDriver,
								   float DefaultContextWidth, float DefaultContextHeight,
								   CStrID VertexShaderID, CStrID PixelShaderRegularID,
								   CStrID PixelShaderOpaqueID, const int abi)
{
	System::performVersionTest(CEGUI_VERSION_ABI, abi, CEGUI_FUNCTION_NAME);
	return *n_new(CDEMRenderer)(GPUDriver, DefaultContextWidth, DefaultContextHeight, VertexShaderID, PixelShaderRegularID, PixelShaderOpaqueID);
}
//--------------------------------------------------------------------

void CDEMRenderer::destroy(CDEMRenderer& renderer)
{
	n_delete(&renderer);
}
//--------------------------------------------------------------------

void CDEMRenderer::logTextureCreation(const String& name)
{
	Logger* logger = Logger::getSingletonPtr();
	if (logger) logger->logEvent("[CEGUI::CDEMRenderer] Created texture: " + name);
}
//---------------------------------------------------------------------

void CDEMRenderer::logTextureDestruction(const String& name)
{
	Logger* logger = Logger::getSingletonPtr();
	if (logger) logger->logEvent("[CEGUI::CDEMRenderer] Destroyed texture: " + name);
}
//---------------------------------------------------------------------

TextureTarget* CDEMRenderer::createTextureTarget(bool addStencilBuffer)
{
	CDEMTextureTarget* t = n_new(CDEMTextureTarget)(*this, addStencilBuffer);
	TexTargets.Add(t);
	return t;
}
//--------------------------------------------------------------------

void CDEMRenderer::destroyTextureTarget(TextureTarget* target)
{
	IPTR Idx = TexTargets.FindIndex((CDEMTextureTarget*)target);
	if (Idx != INVALID_INDEX)
	{
		TexTargets.RemoveAt(Idx);
		n_delete(target);
	}
}
//--------------------------------------------------------------------

void CDEMRenderer::destroyAllTextureTargets()
{
	for (CArray<CDEMTextureTarget*>::CIterator It = TexTargets.Begin(); It < TexTargets.End(); ++It)
		n_delete(*It);
	TexTargets.Clear(true);
}
//--------------------------------------------------------------------

Texture& CDEMRenderer::createTexture(const String& name)
{
	n_assert(!Textures.Contains(name));
	CDEMTexture* tex = n_new(CDEMTexture)(*this, name);
	Textures.Add(name, tex);
	logTextureCreation(name);
	return *tex;
}
//--------------------------------------------------------------------

Texture& CDEMRenderer::createTexture(const String& name, const String& filename, const String& resourceGroup)
{
	n_assert(!Textures.Contains(name));
	CDEMTexture* tex = n_new(CDEMTexture)(*this, name);
	tex->loadFromFile(filename, resourceGroup);
	Textures.Add(name, tex);
	logTextureCreation(name);
	return *tex;
}
//--------------------------------------------------------------------

Texture& CDEMRenderer::createTexture(const String& name, const Sizef& size)
{
	n_assert(!Textures.Contains(name));
	CDEMTexture* tex = n_new(CDEMTexture)(*this, name);
	tex->createEmptyTexture(size);
	Textures.Add(name, tex);
	logTextureCreation(name);
	return *tex;
}
//--------------------------------------------------------------------

void CDEMRenderer::destroyTexture(Texture& texture)
{
	if (Textures.Remove(texture.getName()))
	{
		logTextureDestruction(texture.getName());
		n_delete(&texture);
	}
}
//--------------------------------------------------------------------

void CDEMRenderer::destroyTexture(const String& name)
{
	CDEMTexture* pTexture;
	if (Textures.Get(name, pTexture))
	{
		logTextureDestruction(name);
		n_delete(pTexture);
		Textures.Remove(name); //!!!double search! need iterator!
	}
}
//--------------------------------------------------------------------

void CDEMRenderer::destroyAllTextures()
{
	for (CHashTable<String, CDEMTexture*>::CIterator It = Textures.Begin(); It; ++It)
		n_delete(*It);
	Textures.Clear();
}
//--------------------------------------------------------------------

Texture& CDEMRenderer::getTexture(const String& name) const
{
	CDEMTexture** ppTex = Textures.Get(name);
	n_assert(ppTex && *ppTex);
	return **ppTex;
}
//--------------------------------------------------------------------

RefCounted<RenderMaterial> CDEMRenderer::createRenderMaterial(const DefaultShaderType shaderType) const
{
	if (shaderType == DefaultShaderType::Textured)
	{
		return new RenderMaterial(ShaderWrapperTextured.get());
	}
	else if (shaderType == DefaultShaderType::Solid)
	{
		return new RenderMaterial(ShaderWrapperColoured.get());
	}
	else
	{
		throw RendererException("A default shader of this type does not exist.");
		return RefCounted<RenderMaterial>();
	}
}
//--------------------------------------------------------------------

GeometryBuffer& CDEMRenderer::createGeometryBufferTextured(RefCounted<RenderMaterial> renderMaterial)
{
	CDEMGeometryBuffer* pBuffer = new CDEMGeometryBuffer(*this, renderMaterial);
	pBuffer->setVertexLayout(VertexLayoutTextured);
	addGeometryBuffer(*pBuffer);
	return *pBuffer;
}
//--------------------------------------------------------------------

GeometryBuffer& CDEMRenderer::createGeometryBufferColoured(RefCounted<RenderMaterial> renderMaterial)
{
	CDEMGeometryBuffer* pBuffer = new CDEMGeometryBuffer(*this, renderMaterial);
	pBuffer->setVertexLayout(VertexLayoutColoured);
	addGeometryBuffer(*pBuffer);
	return *pBuffer;
}
//--------------------------------------------------------------------

void CDEMRenderer::beginRendering()
{
}
//---------------------------------------------------------------------

void CDEMRenderer::endRendering()
{
}
//---------------------------------------------------------------------

void CDEMRenderer::setDisplaySize(const Sizef& sz)
{
	if (sz != DisplaySize)
	{
		DisplaySize = sz;

		// FIXME: This is probably not the right thing to do in all cases.
		Rectf area(pDefaultRT->getArea());
		area.setSize(sz);
		pDefaultRT->setArea(area);
	}
}
//---------------------------------------------------------------------

unsigned int CDEMRenderer::getMaxTextureSize() const
{
	return GPU->GetMaxTextureSize(Render::Texture_2D);
}
//---------------------------------------------------------------------

}