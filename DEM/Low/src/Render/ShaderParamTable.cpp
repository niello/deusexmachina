#include "ShaderParamTable.h"
#include <algorithm>

namespace Render
{
__ImplementClassNoFactory(IConstantBufferParam, Core::CObject);

void IShaderConstantParam::SetMatrices(CConstantBuffer& CB, const matrix44* pValue, UPTR Count) const
{
	// transpose to buffer and set floats?
	// Use SetFloats as a backend
	NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------

CShaderParamTable::CShaderParamTable(std::vector<PShaderConstantParam>&& Constants,
	std::vector<PConstantBufferParam>&& ConstantBuffers,
	std::vector<PResourceParam>&& Resources,
	std::vector<PSamplerParam>&& Samplers)

	: _Constants(std::move(Constants))
	, _ConstantBuffers(std::move(ConstantBuffers))
	, _Resources(std::move(Resources))
	, _Samplers(std::move(Samplers))
{
	std::sort(_Constants.begin(), _Constants.end(), [](const PShaderConstantParam& a, const PShaderConstantParam& b)
	{
		return a->GetID() < b->GetID();
	});
	std::sort(_ConstantBuffers.begin(), _ConstantBuffers.end(), [](const PConstantBufferParam& a, const PConstantBufferParam& b)
	{
		return a->GetID() < b->GetID();
	});
	std::sort(_Resources.begin(), _Resources.end(), [](const PResourceParam& a, const PResourceParam& b)
	{
		return a->GetID() < b->GetID();
	});
	std::sort(_Samplers.begin(), _Samplers.end(), [](const PSamplerParam& a, const PSamplerParam& b)
	{
		return a->GetID() < b->GetID();
	});
}
//---------------------------------------------------------------------

}
