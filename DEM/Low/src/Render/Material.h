#pragma once
#include <Data/RefCounted.h>
#include <Data/FixedArray.h>
#include <Render/RenderFwd.h>

// Material defines parameters of an effect to achieve some visual properties of object rendered.
// Material can be shared among render objects, providing the same shaders and shader parameters to all of them.

namespace IO
{
	class CStream;
}

namespace Render
{
typedef Ptr<class CMaterial> PMaterial;
typedef Ptr<class CEffect> PEffect;
class CGPUDriver;

class CMaterial: public Data::CRefCounted
{
protected:

	struct CConstBufferRecord
	{
		HConstBuffer	Handle;
		PConstantBuffer	Buffer;
		EShaderType		ShaderType;		// Now supports binding to only one stage at a time
	};

	struct CResourceRecord
	{
		EShaderType		ShaderType;
		HResource		Handle;
		PTexture		Resource;
	};

	struct CSamplerRecord
	{
		EShaderType		ShaderType;
		HSampler		Handle;
		PSampler		Sampler;
	};

	PEffect							Effect;
	CFixedArray<CConstBufferRecord>	ConstBuffers;
	CFixedArray<CResourceRecord>	Resources;
	CFixedArray<CSamplerRecord>		Samplers;

public:

	CMaterial();
	virtual ~CMaterial();

	bool		Load(CGPUDriver& GPU, IO::CStream& Stream);
	bool		Apply(CGPUDriver& GPU) const;

	bool		IsValid() const { return Effect.IsValidPtr(); }
	CEffect*	GetEffect() const { return Effect.Get(); }
};

}
