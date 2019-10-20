#pragma once

// Shader parameter storage stores values in a form ready to be committed to the GPU.
// In a combination with shader parameter table this class simplifies the process of
// setting shader parameters from the code.

namespace Render
{

class CShaderParamStorage
{
protected:

	// GPU ref
	// Param table ref(shared intrusive ptr?)
	// CBs
	// Resources
	// Samplers

	// All of them are in vectors, indexed the same as params in a table
	// So for Apply() we can just get param by index of the value

public:

	// Apply
};

}
