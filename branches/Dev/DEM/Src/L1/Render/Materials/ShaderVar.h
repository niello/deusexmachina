#pragma once
#ifndef __DEM_L1_RENDER_SHADER_VAR_H__
#define __DEM_L1_RENDER_SHADER_VAR_H__

#include <Render/Materials/Shader.h>
#include <Data/Data.h>

// This is used to apply value to a named shader variable through var handle obtained on binding to shader.
// Values of these types are good:
// bool, int, float, vector4, /*vector3*/, matrix, texture
// May be later arrays of these types, including matrix pointer array, will be added.

namespace Render
{

class CShaderVar
{
protected:

	CStrID			Name; //???or Semantic?
	CShader::HVar	hVar;

public:

	Data::CData		Value;

	CShaderVar(): hVar(0) {}

	void			SetName(CStrID NewName) { if (Name != NewName) { Name = NewName; hVar = 0; } }
	CStrID			GetName() const { return Name; }
	CShader::HVar	GetVarHandle() const { return hVar; }
	bool			IsBound() const { return hVar != 0; }
	bool			Bind(const CShader& Shader) { hVar = Shader.GetVarHandleByName(Name); return hVar != 0; }
	bool			Apply(CShader& Shader) const { return IsBound() && Value.IsValid() && Shader.Set(hVar, Value); }
};

typedef nDictionary<CStrID, CShaderVar> CShaderVarMap;

}

#endif