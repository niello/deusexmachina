#pragma once
#include <Render/RenderFwd.h>

// Shader parameter storage stores values in a form ready to be committed to the GPU.
// In a combination with shader parameter table this class simplifies the process of
// setting shader parameters from the code.

namespace Render
{

class CShaderParamStorage final
{
protected:

	PShaderParamTable _Table;
	PGPUDriver        _GPU;
	// GPU ref
	// CBs
	// Resources
	// Samplers

	// All of them are in vectors, indexed the same as params in a table
	// So for Apply() we can just get param by index of the value

public:

	CShaderParamStorage(CShaderParamTable& Table, CGPUDriver& GPU);
	~CShaderParamStorage();

	// Apply
};

}
