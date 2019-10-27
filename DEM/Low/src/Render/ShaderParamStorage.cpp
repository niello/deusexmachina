#include "ShaderParamStorage.h"
#include <Render/ShaderParamTable.h>
#include <Render/GPUDriver.h>

namespace Render
{

CShaderParamStorage::CShaderParamStorage(CShaderParamTable& Table, CGPUDriver& GPU)
	: _Table(&Table)
	, _GPU(&GPU)
{
}
//---------------------------------------------------------------------

CShaderParamStorage::~CShaderParamStorage() {}
//---------------------------------------------------------------------

}