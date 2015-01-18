#pragma once
#ifndef __DEM_L1_RENDER_SHADER_VARS_H__
#define __DEM_L1_RENDER_SHADER_VARS_H__

#include <Data/StringID.h>

// Shader variable collection, maps variable names to API-specific locations and implements
// passing of variable values to the shader. Shader variable is either a constant or a
// resource, like a buffer or a texture.
// NB: handle is valid only for that shader technique from where it was obtained, except for global vars

namespace Render
{
typedef DWORD HShaderVar; // Opaque to user, so its internal meaning can be overridden in subclasses

class CShaderVars
{
protected:

	// NameID -> Desc(Handle = Buffer | Offset, Type, Size)
	// Set of buffers

public:

	// CB, SRV, SS

	HShaderVar	GetVarHandle(CStrID Name); //!!!get desc!
	// BeginValues
	// SetValue(Handle, Value)
	// EndValues/ApplyValues
	// Batch update? Or some internal access to update directly, faster?
	// 2 + num vars virtual calls per buffer is really bad
};

}

#endif
