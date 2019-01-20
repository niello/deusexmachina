#include "DEMRenderer.h"

#include <Render/GPUDriver.h>
#include <Render/RenderStateDesc.h>
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
#include "CEGUI/System.h"
#include "CEGUI/Logger.h"

namespace CEGUI
{
String CDEMRenderer::RendererID("CEGUI::CDEMRenderer - official DeusExMachina engine renderer by DEM team");

CDEMRenderer::CDEMRenderer(Render::CGPUDriver& GPUDriver, int SwapChainID,
						   float DefaultContextWidth, float DefaultContextHeight,
						   CStrID VertexShaderID, CStrID PixelShaderRegularID, CStrID PixelShaderOpaqueID):
	GPU(&GPUDriver),
	DisplayDPI(96, 96),
	OpaqueMode(false)
{
	Render::PShader VS = GPU->GetShader(VertexShaderID);
	Render::PShader PSRegular = GPU->GetShader(PixelShaderRegularID);
	Render::PShader PSOpaque = GPU->GetShader(PixelShaderOpaqueID);
	n_assert_dbg(VS && PSRegular && PSOpaque && VS->IsResourceValid() && PSRegular->IsResourceValid() && PSOpaque->IsResourceValid());

	if (DefaultContextWidth <= 0.f || DefaultContextHeight <= 0.f)
	{
		const Render::CRenderTargetDesc& RealBackBufDesc = GPU->GetSwapChainRenderTarget(SwapChainID)->GetDesc();
		DisplaySize.d_width = DefaultContextWidth > 0.f ? DefaultContextWidth : static_cast<float>(RealBackBufDesc.Width);
		DisplaySize.d_height = DefaultContextHeight > 0.f ? DefaultContextHeight : static_cast<float>(RealBackBufDesc.Height);
	}
	else
	{
		DisplaySize.d_width = DefaultContextWidth;
		DisplaySize.d_height = DefaultContextHeight;
	}

	//=================================================================
	// Render states
	//=================================================================

	Render::CRenderStateDesc RSDesc;
	Render::CRenderStateDesc::CRTBlend& RTBlendDesc = RSDesc.RTBlend[0];
	RSDesc.SetDefaults();
	RSDesc.VertexShader = VS;
	RSDesc.PixelShader = PSRegular;
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

	// Clipped
	PremultipliedClipped = GPU->CreateRenderState(RSDesc);
	n_assert(PremultipliedClipped.IsValidPtr());

	// Unclipped
	RSDesc.Flags.Clear(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

	PremultipliedUnclipped = GPU->CreateRenderState(RSDesc);
	n_assert(PremultipliedUnclipped.IsValidPtr());

	// Opaque
	RSDesc.PixelShader = PSOpaque;
	RSDesc.Flags.Clear(Render::CRenderStateDesc::Blend_RTBlendEnable << 0);
	RSDesc.Flags.Set(Render::CRenderStateDesc::DS_DepthEnable |
					 Render::CRenderStateDesc::DS_DepthWriteEnable);
	RSDesc.DepthFunc = Render::Cmp_Always;

	// Unclipped
	OpaqueUnclipped = GPU->CreateRenderState(RSDesc);
	n_assert(OpaqueUnclipped.IsValidPtr());

	// Clipped
	RSDesc.Flags.Set(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

	OpaqueClipped = GPU->CreateRenderState(RSDesc);
	n_assert(OpaqueClipped.IsValidPtr());

	//=================================================================
	// Shader constants
	//=================================================================

	// NB: regular and opaque pixel shaders must be compatible
	const Render::IShaderMetadata* pVSMeta = VS->GetMetadata();
	const Render::IShaderMetadata* pPSMeta = PSRegular->GetMetadata();

	ConstWorldMatrix = pVSMeta->GetConstant(pVSMeta->GetConstHandle(CStrID("WorldMatrix")));
	n_assert(ConstWorldMatrix.IsValidPtr());
	hWMCB = ConstWorldMatrix->GetConstantBufferHandle();
	n_assert(hWMCB != INVALID_HANDLE);

	ConstProjMatrix = pVSMeta->GetConstant(pVSMeta->GetConstHandle(CStrID("ProjectionMatrix")));
	n_assert(ConstProjMatrix.IsValidPtr());
	hPMCB = ConstProjMatrix->GetConstantBufferHandle();
	n_assert(hPMCB != INVALID_HANDLE);

	WMCB = GPU->CreateConstantBuffer(hWMCB, Render::Access_GPU_Read | Render::Access_CPU_Write);
	if (hWMCB == hPMCB) PMCB = WMCB;
	else PMCB = GPU->CreateConstantBuffer(hPMCB, Render::Access_GPU_Read | Render::Access_CPU_Write);

	hTexture = pPSMeta->GetResourceHandle(CStrID("BoundTexture"));
	hLinearSampler = pPSMeta->GetSamplerHandle(CStrID("LinearSampler"));

	Render::CSamplerDesc SampDesc;
	SampDesc.SetDefaults();
	SampDesc.AddressU = Render::TexAddr_Clamp;
	SampDesc.AddressV = Render::TexAddr_Clamp;
	SampDesc.Filter = Render::TexFilter_MinMagMip_Linear;
	LinearSampler = GPU->CreateSampler(SampDesc);
	n_assert(LinearSampler.IsValidPtr());

	//=================================================================
	// Vertex declaration
	//=================================================================

	Render::CVertexComponent Components[] = {
			{ Render::VCSem_Position, NULL, 0, Render::VCFmt_Float32_3, 0, 0, false },
			{ Render::VCSem_Color, NULL, 0, Render::VCFmt_UInt8_4_Norm, 0, 12, false },
			{ Render::VCSem_TexCoord, NULL, 0, Render::VCFmt_Float32_2, 0, 16, false } };

	VertexLayout = GPU->CreateVertexLayout(Components, sizeof_array(Components));
	n_assert(VertexLayout.IsValidPtr());

	//!!!need some method to precreate actual vertex layout by passing a shader!
	//but this may require knowledge about D3D11 nature of otherwise abstract GPU
	//???call GPU->CreateRenderCache() and then reuse it?

	pDefaultRT = n_new(CDEMViewportTarget)(*this, Rectf(0.f, 0.f, DefaultContextWidth, DefaultContextHeight));
}
//--------------------------------------------------------------------

CDEMRenderer::~CDEMRenderer()
{
	destroyAllTextureTargets();
	destroyAllTextures();
	destroyAllGeometryBuffers();
	n_delete(pDefaultRT);

	ConstWorldMatrix = NULL;
	ConstProjMatrix = NULL;
	hWMCB = INVALID_HANDLE;
	hPMCB = INVALID_HANDLE;
	hTexture = INVALID_HANDLE;
	hLinearSampler = INVALID_HANDLE;
	VertexLayout = NULL;
	NormalUnclipped = NULL;
	NormalClipped = NULL;
	PremultipliedUnclipped = NULL;
	PremultipliedClipped = NULL;
	OpaqueUnclipped = NULL;
	OpaqueClipped = NULL;
	WMCB = NULL;
	PMCB = NULL;
	LinearSampler = NULL;
	GPU = NULL;
}
//--------------------------------------------------------------------

CDEMRenderer& CDEMRenderer::create(Render::CGPUDriver& GPUDriver, int SwapChainID,
								   float DefaultContextWidth, float DefaultContextHeight,
								   CStrID VertexShaderID, CStrID PixelShaderRegularID,
								   CStrID PixelShaderOpaqueID, const int abi)
{
	System::performVersionTest(CEGUI_VERSION_ABI, abi, CEGUI_FUNCTION_NAME);
	return *n_new(CDEMRenderer)(GPUDriver, SwapChainID, DefaultContextWidth, DefaultContextHeight, VertexShaderID, PixelShaderRegularID, PixelShaderOpaqueID);
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
	IPTR Idx = GeomBuffers.FindIndex((CDEMGeometryBuffer*)&buffer);
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

Render::PVertexBuffer CDEMRenderer::createVertexBuffer(D3DVertex* pVertexData, UPTR VertexCount)
{
	if (!pVertexData || !VertexCount || VertexLayout.IsNullPtr()) return NULL;
	return GPU->CreateVertexBuffer(*VertexLayout, VertexCount, Render::Access_GPU_Read | Render::Access_CPU_Write, pVertexData);
}
//--------------------------------------------------------------------

void CDEMRenderer::setRenderState(BlendMode BlendMode, bool Clipped)
{
	if (OpaqueMode)
	{
		GPU->SetRenderState(Clipped ? OpaqueClipped : OpaqueUnclipped);
	}
	else
	{
		if (BlendMode == BM_RTT_PREMULTIPLIED)
			GPU->SetRenderState(Clipped ? PremultipliedClipped : PremultipliedUnclipped);
		else
			GPU->SetRenderState(Clipped ? NormalClipped : NormalUnclipped);
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

void CDEMRenderer::setWorldMatrix(const matrix44& Matrix)
{
	if (WMCB.IsValidPtr())
		ConstWorldMatrix->SetRawValue(*WMCB.Get(), reinterpret_cast<const float*>(&Matrix), sizeof(matrix44));
}
//--------------------------------------------------------------------

void CDEMRenderer::setProjMatrix(const matrix44& Matrix)
{
	if (PMCB.IsValidPtr())
		ConstProjMatrix->SetRawValue(*PMCB.Get(), reinterpret_cast<const float*>(&Matrix), sizeof(matrix44));
}
//--------------------------------------------------------------------

void CDEMRenderer::commitChangedConsts()
{
	if (WMCB.IsValidPtr())
		GPU->CommitShaderConstants(*WMCB.Get());
	if (hWMCB != hPMCB && PMCB.IsValidPtr())
		GPU->CommitShaderConstants(*PMCB.Get());
}
//--------------------------------------------------------------------

void CDEMRenderer::beginRendering()
{
	GPU->SetVertexLayout(VertexLayout.Get());

	GPU->BindConstantBuffer(Render::ShaderType_Vertex, hWMCB, WMCB.Get());
	GPU->BeginShaderConstants(*WMCB.Get());
	if (hWMCB != hPMCB)
	{
		GPU->BindConstantBuffer(Render::ShaderType_Vertex, hPMCB, PMCB.Get());
		GPU->BeginShaderConstants(*PMCB.Get());
	}

	GPU->BindSampler(Render::ShaderType_Pixel, hLinearSampler, LinearSampler.Get());
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