#include "Technique.h"
#include <Render/RenderState.h>

namespace Render
{

CTechnique::CTechnique(CStrID Name, U8 SortingKey, std::vector<PRenderState>&& Passes, IPTR MaxLights, PShaderParamTable Params)
	: _Name(Name)
	, _Passes(std::move(Passes))
	, _MaxLightCount(MaxLights)
	, _Params(Params)
	, _SortingKey(SortingKey)
{
}
//---------------------------------------------------------------------

CTechnique::~CTechnique() = default;
//---------------------------------------------------------------------

}
