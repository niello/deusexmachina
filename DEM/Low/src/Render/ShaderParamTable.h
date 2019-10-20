#pragma once

// Shader parameter table stores metadata necessary to set shader parameter values

namespace Render
{

class CShaderParamTable
{
protected:

	// struct info for constants
	// constants
	// cbs
	// resources
	// samplers

	//???each of params stores its index inside to avoid search? it is like a handle
	//internal dbg validation is simple, pParam == Table.pParams[pParam->Index]

public:

	// get params by name, by index
	// each class of params is completely separate, no common base class for all
};

}
