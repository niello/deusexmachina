#pragma once
#include <Data/RefCounted.h>
#include <Render/ShaderParamStorage.h>

// Material defines parameters of an effect to achieve some visual properties of object rendered.
// Material can be shared among render objects, providing the same shaders and shader parameters to all of them.

namespace IO
{
	class CStream;
}

namespace Render
{
typedef Ptr<class CMaterial> PMaterial;

class CMaterial: public Data::CRefCounted
{
protected:

	PEffect             Effect;
	CShaderParamStorage Values;

public:

	CMaterial();
	virtual ~CMaterial() override;

	bool     Apply() const { return Values.Apply(); }

	bool     IsValid() const { return Effect.IsValidPtr(); }
	CEffect* GetEffect() const { return Effect.Get(); }
};

}
