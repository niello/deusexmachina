#include "ShaderParamTable.h"
#include <algorithm>

namespace Render
{

CShaderParamTable::CShaderParamTable(std::vector<PShaderConstantParam>&& Constants,
	std::vector<PShaderConstantBufferParam>&& ConstantBuffers,
	std::vector<PShaderResourceParam>&& Resources,
	std::vector<PShaderSamplerParam>&& Samplers)

	: _Constants(std::move(Constants))
	, _ConstantBuffers(std::move(ConstantBuffers))
	, _Resources(std::move(Resources))
	, _Samplers(std::move(Samplers))
{
	std::sort(_Constants.begin(), _Constants.end(), [](const PShaderConstantParam& a, const PShaderConstantParam& b)
	{
		return a->GetID() < b->GetID();
	});
	std::sort(_ConstantBuffers.begin(), _ConstantBuffers.end(), [](const PShaderConstantBufferParam& a, const PShaderConstantBufferParam& b)
	{
		return a->GetID() < b->GetID();
	});
	std::sort(_Resources.begin(), _Resources.end(), [](const PShaderResourceParam& a, const PShaderResourceParam& b)
	{
		return a->GetID() < b->GetID();
	});
	std::sort(_Samplers.begin(), _Samplers.end(), [](const PShaderSamplerParam& a, const PShaderSamplerParam& b)
	{
		return a->GetID() < b->GetID();
	});
}
//---------------------------------------------------------------------

}
