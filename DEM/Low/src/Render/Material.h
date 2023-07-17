#pragma once
#include <Data/RefCounted.h>
#include <Render/ShaderParamStorage.h>

// Material defines parameters of an effect to achieve some visual properties of object rendered.
// Material can be shared among render objects, providing the same shaders and shader parameters to all of them.

namespace IO
{
	class IStream;
}

namespace Render
{
typedef Ptr<class CMaterial> PMaterial;

class CMaterial: public Data::CRefCounted
{
protected:

	CStrID              _UID;
	PEffect             _Effect;
	CShaderParamStorage _Values;
	U16                 _SortingKey = 0;

public:

	CMaterial(CStrID UID, U16 SortingKey, CEffect& Effect, CShaderParamStorage&& Values);
	virtual ~CMaterial() override;

	bool     Apply() { return _Values.Apply(); }

	CStrID   GetUID() const { return _UID; }
	bool     IsValid() const { return _Effect.IsValidPtr(); }
	CEffect* GetEffect() const { return _Effect.Get(); }
	U16      GetSortingKey() const { return _SortingKey; }
};

}
