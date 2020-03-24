#include "DEMRenderer.h"
#include "DEMGeometryBuffer.h"
#include "DEMTextureTarget.h"
#include "DEMViewportTarget.h"
#include "DEMTexture.h"
#include "DEMShaderWrapper.h"
#include <Render/GPUDriver.h>
#include <Render/VertexLayout.h>
#include <CEGUI/System.h>
#include <CEGUI/Logger.h>

namespace CEGUI
{
String CDEMRenderer::RendererID("CEGUI::CDEMRenderer - official DeusExMachina engine renderer by DEM team");

CDEMRenderer::CDEMRenderer(Render::CGPUDriver& GPUDriver)
	: GPU(&GPUDriver)
{
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

	VertexLayoutTextured = nullptr;
	VertexLayoutColoured = nullptr;
	GPU = nullptr;
}
//--------------------------------------------------------------------

CDEMRenderer& CDEMRenderer::create(Render::CGPUDriver& GPUDriver, const int abi)
{
	System::performVersionTest(CEGUI_VERSION_ABI, abi, CEGUI_FUNCTION_NAME);
	return *n_new(CDEMRenderer)(GPUDriver);
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

RenderTarget* CDEMRenderer::createViewportTarget(float width, float height)
{
	Rectf area(0.f, 0.f, width, height);
	CDEMViewportTarget* t = n_new(CDEMViewportTarget)(*this, area);
	VPTargets.Add(t);
	return t;
}
//---------------------------------------------------------------------

void CDEMRenderer::destroyViewportTarget(RenderTarget* target)
{
	IPTR Idx = VPTargets.FindIndex(target);
	if (Idx != INVALID_INDEX)
	{
		VPTargets.RemoveAt(Idx);
		n_delete(target);
	}
}
//---------------------------------------------------------------------

void CDEMRenderer::setEffects(CDEMShaderWrapper* pTextured, CDEMShaderWrapper* pColoured)
{
	pShaderWrapperTextured = pTextured;
	pShaderWrapperColoured = pColoured;
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

RenderTarget& CDEMRenderer::getDefaultRenderTarget()
{
	static CDEMViewportTarget NullTarget(*this, Rectf(0.f, 0.f, 0.f, 0.f));
	n_assert("CDEMRenderer::getDefaultRenderTarget() > should NOT be used! To be removed from CEGUI!");
	return NullTarget;
}
//--------------------------------------------------------------------

RefCounted<RenderMaterial> CDEMRenderer::createRenderMaterial(const DefaultShaderType shaderType) const
{
	if (shaderType == DefaultShaderType::Textured)
	{
		return new RenderMaterial(pShaderWrapperTextured);
	}
	else if (shaderType == DefaultShaderType::Solid)
	{
		return new RenderMaterial(pShaderWrapperColoured);
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
	pBuffer->addVertexAttribute(VertexAttributeType::Position0);
	pBuffer->addVertexAttribute(VertexAttributeType::Colour0);
	pBuffer->addVertexAttribute(VertexAttributeType::TexCoord0);
	pBuffer->setVertexLayout(VertexLayoutTextured);
	addGeometryBuffer(*pBuffer);
	return *pBuffer;
}
//--------------------------------------------------------------------

GeometryBuffer& CDEMRenderer::createGeometryBufferColoured(RefCounted<RenderMaterial> renderMaterial)
{
	CDEMGeometryBuffer* pBuffer = new CDEMGeometryBuffer(*this, renderMaterial);
	pBuffer->addVertexAttribute(VertexAttributeType::Position0);
	pBuffer->addVertexAttribute(VertexAttributeType::Colour0);
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
	pShaderWrapperTextured = nullptr;
	pShaderWrapperColoured = nullptr;
}
//---------------------------------------------------------------------

void CDEMRenderer::setDisplaySize(const Sizef& sz)
{
	DisplaySize = sz;
}
//---------------------------------------------------------------------

unsigned int CDEMRenderer::getMaxTextureSize() const
{
	return GPU->GetMaxTextureSize(Render::Texture_2D);
}
//---------------------------------------------------------------------

}