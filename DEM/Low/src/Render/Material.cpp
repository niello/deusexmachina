#include "Material.h"

#include <Render/RenderFwd.h>
#include <Render/GPUDriver.h>
#include <Render/Effect.h>
#include <Render/ShaderConstant.h>
#include <Render/ConstantBuffer.h>
#include <Render/Sampler.h>
#include <Render/Texture.h>
#include <IO/BinaryReader.h>

namespace Render
{
CMaterial::CMaterial() {}
CMaterial::~CMaterial() {}

// NB: GPU must be the same as used for loading
bool CMaterial::Apply(CGPUDriver& GPU) const
{
	for (UPTR i = 0 ; i < ConstBuffers.GetCount(); ++i)
	{
		const CConstBufferRecord& Rec = ConstBuffers[i];
		if (!GPU.BindConstantBuffer(Rec.ShaderType, Rec.Handle, Rec.Buffer.Get())) FAIL;
	}

	for (UPTR i = 0 ; i < Resources.GetCount(); ++i)
	{
		const CResourceRecord& Rec = Resources[i];
		if (!GPU.BindResource(Rec.ShaderType, Rec.Handle, Rec.Resource.Get())) FAIL;
	}

	for (UPTR i = 0 ; i < Samplers.GetCount(); ++i)
	{
		const CSamplerRecord& Rec = Samplers[i];
		if (!GPU.BindSampler(Rec.ShaderType, Rec.Handle, Rec.Sampler.Get())) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

}