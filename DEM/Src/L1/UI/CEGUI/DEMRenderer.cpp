#include "DEMRenderer.h"

#include <Render/GPUDriver.h>
#include "DEMGeometryBuffer.h"
#include "DEMTextureTarget.h"
#include "DEMViewportTarget.h"
#include "DEMTexture.h"
//#include "CEGUI/Exceptions.h"
#include "CEGUI/System.h"
//#include "CEGUI/DefaultResourceProvider.h"
#include "CEGUI/Logger.h"
//#include <algorithm>

//#include "shader.txt"

namespace CEGUI
{
String CDEMRenderer::RendererID("CEGUI::CDEMRenderer - official DeusExMachina engine renderer by DEM team");

CDEMRenderer::CDEMRenderer(Render::CGPUDriver& GPUDriver, int SwapChain):
	GPU(&GPUDriver),
	SwapChainID(SwapChain),
	pDefaultRT(NULL),
	DisplayDPI(96, 96)
{
	n_assert(GPU->SwapChainExists(SwapChainID));

	Render::CViewport VP;
	n_assert(GPU->GetViewport(0, VP));
	DisplaySize = Sizef((float)VP.Width, (float)VP.Height);

	n_assert(false);
/*
    // create the main effect from the shader source.
    ID3D10Blob* errors = 0;

	DWORD DefaultOptions=NULL;//D3D10_SHADER_PACK_MATRIX_ROW_MAJOR|D3D10_SHADER_PARTIAL_PRECISION|D3D10_SHADER_SKIP_VALIDATION;

	ID3D10Blob* ShaderBlob=NULL;//first we compile shader, then create effect from it

	if (FAILED(D3DX11CompileFromMemory(shaderSource,sizeof(shaderSource),
		"shaderSource",NULL,NULL,NULL,"fx_5_0",
		DefaultOptions,NULL,NULL,&ShaderBlob,&errors,NULL)))
	{
		std::string msg(static_cast<const char*>(errors->GetBufferPointer()),
			errors->GetBufferSize());
		errors->Release();
		Sys::Error(msg.c_str());
	}

	n_assert(SUCCEEDED(D3DX11CreateEffectFromMemory(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize(),0, 
		d_device.d_device, &d_effect) ));

	if (ShaderBlob) ShaderBlob->Release();

    // extract the rendering techniques
    d_normalClippedTechnique = d_effect->GetTechniqueByName("BM_NORMAL_Clipped_Rendering");
    d_normalUnclippedTechnique = d_effect->GetTechniqueByName("BM_NORMAL_Unclipped_Rendering");
    d_premultipliedClippedTechnique = d_effect->GetTechniqueByName("BM_RTT_PREMULTIPLIED_Clipped_Rendering");
    d_premultipliedClippedTechnique = d_effect->GetTechniqueByName("BM_RTT_PREMULTIPLIED_Unclipped_Rendering");

    // Get the variables from the shader we need to be able to access
    d_boundTextureVariable =
            d_effect->GetVariableByName("BoundTexture")->AsShaderResource();
    d_worldMatrixVariable =
            d_effect->GetVariableByName("WorldMatrix")->AsMatrix();
    d_projectionMatrixVariable =
            d_effect->GetVariableByName("ProjectionMatrix")->AsMatrix();

	*/

	Render::CVertexComponent Components[] = {
			{ Render::VCSem_Position, NULL, 0, Render::VCFmt_Float32_3, 0, 0 },
			{ Render::VCSem_Color, NULL, 0, Render::VCFmt_UInt8_4_Norm, 0, 12 },
			{ Render::VCSem_TexCoord, NULL, 0, Render::VCFmt_Float32_2, 0, 16 } };

	VertexLayout = GPU->CreateVertexLayout(Components, sizeof_array(Components));
	n_assert(VertexLayout.IsValidPtr());

	//???when to create actual layout? need some method to precreate actual value by passing a shader!
	//but this may require knowledge about D3D11 nature of otherwise abstract GPU

	pDefaultRT = n_new(CDEMViewportTarget)(*this);
}
//--------------------------------------------------------------------

CDEMRenderer::~CDEMRenderer()
{
	destroyAllTextureTargets();
	destroyAllTextures();
	destroyAllGeometryBuffers();
	n_delete(pDefaultRT);

	n_assert(false);
	//if (d_effect) d_effect->Release();
}
//--------------------------------------------------------------------

CDEMRenderer& CDEMRenderer::create(Render::CGPUDriver& GPUDriver, int SwapChain, const int abi)
{
	System::performVersionTest(CEGUI_VERSION_ABI, abi, CEGUI_FUNCTION_NAME);
	return *n_new(CDEMRenderer)(GPUDriver, SwapChain);
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

GeometryBuffer& CDEMRenderer::createGeometryBuffer()
{
	CDEMGeometryBuffer* b = n_new(CDEMGeometryBuffer)(*this);
	GeomBuffers.Add(b);
	return *b;
}
//--------------------------------------------------------------------

void CDEMRenderer::destroyGeometryBuffer(const GeometryBuffer& buffer)
{
	int Idx = GeomBuffers.FindIndex((CDEMGeometryBuffer*)&buffer);
	if (Idx != INVALID_INDEX)
	{
		GeomBuffers.RemoveAt(Idx);
		n_delete(&buffer);
	}
}
//--------------------------------------------------------------------

void CDEMRenderer::destroyAllGeometryBuffers()
{
	for (CArray<CDEMGeometryBuffer*>::CIterator It = GeomBuffers.Begin(); It < GeomBuffers.End(); ++It)
		n_delete(*It);
	GeomBuffers.Clear(true);
}
//--------------------------------------------------------------------

Render::PVertexBuffer CDEMRenderer::createVertexBuffer(D3DVertex* pVertexData, DWORD VertexCount)
{
	if (!pVertexData || !VertexCount || VertexLayout.IsNullPtr()) return NULL;
	return GPU->CreateVertexBuffer(*VertexLayout, VertexCount, Render::Access_GPU_Read | Render::Access_CPU_Write, pVertexData);
}
//--------------------------------------------------------------------

TextureTarget* CDEMRenderer::createTextureTarget()
{
	CDEMTextureTarget* t = n_new(CDEMTextureTarget)(*this);
	TexTargets.Add(t);
	return t;
}
//--------------------------------------------------------------------

void CDEMRenderer::destroyTextureTarget(TextureTarget* target)
{
	int Idx = TexTargets.FindIndex((CDEMTextureTarget*)target);
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

void CDEMRenderer::beginRendering()
{
	GPU->SetVertexLayout(VertexLayout.GetUnsafe());
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

uint CDEMRenderer::getMaxTextureSize() const
{
	return GPU->GetMaxTextureSize(Render::Texture_2D);
}
//---------------------------------------------------------------------

}