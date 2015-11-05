#include "DEMRenderer.h"

#include <Render/GPUDriver.h>
#include <Render/RenderStateDesc.h>
#include <Render/Shader.h>
#include <Render/ShaderLoader.h>
#include <Render/SamplerDesc.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <IO/PathUtils.h>
#include <IO/IOServer.h>
#include <Data/Buffer.h>
#include "DEMGeometryBuffer.h"
#include "DEMTextureTarget.h"
#include "DEMViewportTarget.h"
#include "DEMTexture.h"
#include "CEGUI/System.h"
#include "CEGUI/Logger.h"
//#include <algorithm>

//#include "shader.txt"

namespace CEGUI
{
String CDEMRenderer::RendererID("CEGUI::CDEMRenderer - official DeusExMachina engine renderer by DEM team");

CDEMRenderer::CDEMRenderer(Render::CGPUDriver& GPUDriver, int SwapChain, const char* pVertexShaderURI, const char* pPixelShaderURI):
	GPU(&GPUDriver),
	SwapChainID(SwapChain),
	pDefaultRT(NULL),
	DisplayDPI(96, 96)
{
	n_assert(GPU->SwapChainExists(SwapChainID));

	Render::CViewport VP;
	n_assert(GPU->GetViewport(0, VP));
	DisplaySize = Sizef((float)VP.Width, (float)VP.Height);

	n_assert_dbg(pVertexShaderURI && pPixelShaderURI);

	Resources::PResource RVS = ResourceMgr->RegisterResource(pVertexShaderURI);
	if (!RVS->IsLoaded())
	{
		Resources::PResourceLoader Loader = RVS->GetLoader();
		if (Loader.IsNullPtr())
			Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CShader>(PathUtils::GetExtension(pVertexShaderURI));
		Loader->As<Resources::CShaderLoader>()->GPU = GPU;
		ResourceMgr->LoadResourceSync(*RVS, *Loader);
		n_assert(RVS->IsLoaded());
	}

	Resources::PResource RPS = ResourceMgr->RegisterResource(pPixelShaderURI);
	if (!RPS->IsLoaded())
	{
		Resources::PResourceLoader Loader = RPS->GetLoader();
		if (Loader.IsNullPtr())
			Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CShader>(PathUtils::GetExtension(pPixelShaderURI));
		Loader->As<Resources::CShaderLoader>()->GPU = GPU;
		ResourceMgr->LoadResourceSync(*RPS, *Loader);
		n_assert(RPS->IsLoaded());
	}

	Render::CRenderStateDesc RSDesc;
	Render::CRenderStateDesc::CRTBlend& RTBlendDesc = RSDesc.RTBlend[0];
	RSDesc.SetDefaults();
	RSDesc.VertexShader = RVS->GetObject()->As<Render::CShader>();
	RSDesc.PixelShader = RPS->GetObject()->As<Render::CShader>();
	RSDesc.Flags.Set(Render::CRenderStateDesc::Blend_RTBlendEnable << 0);
	RSDesc.Flags.Clear(Render::CRenderStateDesc::DS_DepthEnable |
					   Render::CRenderStateDesc::DS_DepthWriteEnable |
					   Render::CRenderStateDesc::Rasterizer_DepthClipEnable |
					   Render::CRenderStateDesc::Rasterizer_Wireframe |
					   Render::CRenderStateDesc::Rasterizer_CullFront |
					   Render::CRenderStateDesc::Rasterizer_CullBack |
					   Render::CRenderStateDesc::Blend_AlphaToCoverage |
					   Render::CRenderStateDesc::Blend_Independent);

	// Normal blend
	RTBlendDesc.SrcBlendArgAlpha = Render::BlendArg_InvDestAlpha;
	RTBlendDesc.DestBlendArgAlpha = Render::BlendArg_One;
	RTBlendDesc.SrcBlendArg = Render::BlendArg_SrcAlpha;
	RTBlendDesc.DestBlendArg = Render::BlendArg_InvSrcAlpha;

	// Unclipped
	RSDesc.Flags.Clear(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

	NormalUnclipped = GPU->CreateRenderState(RSDesc);
	n_assert(NormalUnclipped.IsValidPtr());

	// Clipped
	RSDesc.Flags.Set(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

	NormalClipped = GPU->CreateRenderState(RSDesc);
	n_assert(NormalClipped.IsValidPtr());

	// Premultiplied alpha blend
	RTBlendDesc.SrcBlendArgAlpha = Render::BlendArg_One;
	RTBlendDesc.DestBlendArgAlpha = Render::BlendArg_InvSrcAlpha;
	RTBlendDesc.SrcBlendArg = Render::BlendArg_One;
	RTBlendDesc.DestBlendArg = Render::BlendArg_InvSrcAlpha;

	PremultipliedClipped = GPU->CreateRenderState(RSDesc);
	n_assert(PremultipliedClipped.IsValidPtr());

	// Unclipped
	RSDesc.Flags.Clear(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

	PremultipliedUnclipped = GPU->CreateRenderState(RSDesc);
	n_assert(PremultipliedUnclipped.IsValidPtr());

	hWorldMatrix = RVS->GetObject()->As<Render::CShader>()->GetConstHandle(CStrID("WorldMatrix"));
	hProjMatrix = RVS->GetObject()->As<Render::CShader>()->GetConstHandle(CStrID("ProjectionMatrix"));
	hCBOnResize = RVS->GetObject()->As<Render::CShader>()->GetConstBufferHandle(CStrID("ChangeOnResize"));
	hCBPerObject = RVS->GetObject()->As<Render::CShader>()->GetConstBufferHandle(CStrID("ChangePerObject"));
	hTexture = RPS->GetObject()->As<Render::CShader>()->GetResourceHandle(CStrID("BoundTexture"));
	hLinearSampler = RPS->GetObject()->As<Render::CShader>()->GetSamplerHandle(CStrID("LinearSampler"));

	//???or two buffers? proj matrix is far less frequently set than world!
	CBOnResize = GPU->CreateConstantBuffer(hCBOnResize, Render::Access_GPU_Read | Render::Access_CPU_Write);
	CBPerObject = GPU->CreateConstantBuffer(hCBPerObject, Render::Access_GPU_Read | Render::Access_CPU_Write);

	Render::CSamplerDesc SampDesc;
	SampDesc.SetDefaults();
	SampDesc.AddressU = Render::TexAddr_Clamp;
	SampDesc.AddressV = Render::TexAddr_Clamp;
	SampDesc.Filter = Render::TexFilter_MinMagMip_Linear;
	LinearSampler = GPU->CreateSampler(SampDesc);
	n_assert(LinearSampler.IsValidPtr());

	Render::CVertexComponent Components[] = {
			{ Render::VCSem_Position, NULL, 0, Render::VCFmt_Float32_3, 0, 0 },
			{ Render::VCSem_Color, NULL, 0, Render::VCFmt_UInt8_4_Norm, 0, 12 },
			{ Render::VCSem_TexCoord, NULL, 0, Render::VCFmt_Float32_2, 0, 16 } };

	VertexLayout = GPU->CreateVertexLayout(Components, sizeof_array(Components));
	n_assert(VertexLayout.IsValidPtr());

	//???when to create actual layout? need some method to precreate actual value by passing a shader!
	//but this may require knowledge about D3D11 nature of otherwise abstract GPU
	//???call GPU->CreateRenderCache() and then reuse it?

	pDefaultRT = n_new(CDEMViewportTarget)(*this);
}
//--------------------------------------------------------------------

CDEMRenderer::~CDEMRenderer()
{
	destroyAllTextureTargets();
	destroyAllTextures();
	destroyAllGeometryBuffers();
	n_delete(pDefaultRT);

	hWorldMatrix = INVALID_HANDLE;
	hProjMatrix = INVALID_HANDLE;
	hCBOnResize = INVALID_HANDLE;
	hCBPerObject = INVALID_HANDLE;
	hTexture = INVALID_HANDLE;
	hLinearSampler = INVALID_HANDLE;
	VertexLayout = NULL;
	NormalUnclipped = NULL;
	NormalClipped = NULL;
	PremultipliedUnclipped = NULL;
	PremultipliedClipped = NULL;
	CBOnResize = NULL;
	CBPerObject = NULL;
	LinearSampler = NULL;
	GPU = NULL;
}
//--------------------------------------------------------------------

CDEMRenderer& CDEMRenderer::create(Render::CGPUDriver& GPUDriver, int SwapChain, const char* pVertexShaderURI, const char* pPixelShaderURI, const int abi)
{
	System::performVersionTest(CEGUI_VERSION_ABI, abi, CEGUI_FUNCTION_NAME);
	return *n_new(CDEMRenderer)(GPUDriver, SwapChain, pVertexShaderURI, pPixelShaderURI);
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

void CDEMRenderer::setRenderState(BlendMode BlendMode, bool Clipped)
{
	if (BlendMode == BM_RTT_PREMULTIPLIED)
		GPU->SetRenderState(Clipped ? PremultipliedClipped : PremultipliedUnclipped);
	else
		GPU->SetRenderState(Clipped ? NormalClipped : NormalUnclipped);
}
//--------------------------------------------------------------------

void CDEMRenderer::setWorldMatrix(const matrix44& Matrix)
{
	if (CBPerObject.IsValidPtr())
	{
		GPU->BeginShaderConstants(*CBPerObject.GetUnsafe());
		GPU->SetShaderConstant(*CBPerObject.GetUnsafe(), hWorldMatrix, 0, reinterpret_cast<const float*>(&Matrix), sizeof(matrix44));
		GPU->CommitShaderConstants(*CBPerObject.GetUnsafe());
	}
}
//--------------------------------------------------------------------

void CDEMRenderer::setProjMatrix(const matrix44& Matrix)
{
	if (CBOnResize.IsValidPtr())
	{
		GPU->BeginShaderConstants(*CBOnResize.GetUnsafe());
		GPU->SetShaderConstant(*CBOnResize.GetUnsafe(), hProjMatrix, 0, reinterpret_cast<const float*>(&Matrix), sizeof(matrix44));
		GPU->CommitShaderConstants(*CBOnResize.GetUnsafe());
	}
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
	GPU->BindConstantBuffer(Render::ShaderType_Vertex, hCBOnResize, CBOnResize.GetUnsafe());
	GPU->BindConstantBuffer(Render::ShaderType_Vertex, hCBPerObject, CBPerObject.GetUnsafe());
	GPU->BindSampler(Render::ShaderType_Pixel, hLinearSampler, LinearSampler.GetUnsafe());
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