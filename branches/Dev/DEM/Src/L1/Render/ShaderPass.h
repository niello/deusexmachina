#pragma once
#ifndef __DEM_L1_RENDER_SHADER_PASS_H__
#define __DEM_L1_RENDER_SHADER_PASS_H__

//#include <Render/ShaderVars.h>

// One fully described rendering pipeline pass. Includes all the shaders used,
// their parameters and a rendering pipeline state, if it differs from current defaults

namespace Render
{

class CShaderPass
{
public:

	CStrID	InputSignatureID;

	//???state?

	// API-specific shader objects
};

}

#endif
