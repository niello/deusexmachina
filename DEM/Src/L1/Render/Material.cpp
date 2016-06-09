#include "Material.h"

#include <Render/GPUDriver.h>

namespace Render
{
__ImplementClassNoFactory(Render::CMaterial, /*'MTRL', */Resources::CResourceObject);

bool CMaterial::Apply(CGPUDriver& GPU) const
{
	for (UPTR i = 0 ; i < ConstBuffers.GetCount(); ++i)
	{
		const CConstBufferRec& Rec = ConstBuffers[i];
		if (!GPU.BindConstantBuffer(Rec.ShaderType, Rec.Handle, Rec.Buffer.GetUnsafe())) FAIL;
	}

	for (UPTR i = 0 ; i < Resources.GetCount(); ++i)
	{
		const CResourceRec& Rec = Resources[i];
		if (!GPU.BindResource(Rec.ShaderType, Rec.Handle, Rec.Resource.GetUnsafe())) FAIL;
	}

	for (UPTR i = 0 ; i < Samplers.GetCount(); ++i)
	{
		const CSamplerRec& Rec = Samplers[i];
		if (!GPU.BindSampler(Rec.ShaderType, Rec.Handle, Rec.Sampler.GetUnsafe())) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

}