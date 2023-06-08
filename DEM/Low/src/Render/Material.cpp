#include "Material.h"
#include <Render/Effect.h>

namespace Render
{

CMaterial::CMaterial(CStrID UID, CEffect& Effect, CShaderParamStorage&& Values)
	: _UID(UID)
	, _Effect(&Effect)
	, _Values(std::move(Values))
{
	n_assert(&_Effect->GetMaterialParamTable() == &_Values.GetParamTable());
}
//---------------------------------------------------------------------

CMaterial::~CMaterial() = default;
//---------------------------------------------------------------------

}
