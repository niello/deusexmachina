#include "Technique.h"
#include <Render/RenderState.h>

namespace Render
{

CTechnique::CTechnique(CStrID Name, std::vector<PRenderState>&& Passes, IPTR MaxLights, PShaderParamTable Params)
	: _Name(Name)
	, _Passes(std::move(Passes))
	, _MaxLightCount(MaxLights)
	, _Params(Params)
{
}
//---------------------------------------------------------------------

CTechnique::~CTechnique() = default;
//---------------------------------------------------------------------

}
