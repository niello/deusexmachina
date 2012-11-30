#ifndef N_SKYSTATE_H
#define N_SKYSTATE_H

#include "scene/ntransformnode.h"

// Provides data for animated or state switching nodes
// (C) 2005 RadonLabs GmbH

class nSkyState: public nTransformNode
{
private:

	nShaderParams shaderParams;

public:

	virtual bool LoadDataBlock(nFourCC FourCC, Data::CBinaryReader& DataReader);

	virtual bool HasTransform() const { return false; }

	void SetInt(nShaderState::Param param, int val);
	int GetInt(nShaderState::Param param) const { return shaderParams.GetArg(param).GetInt(); }
	void SetBool(nShaderState::Param param, bool val);
	bool GetBool(nShaderState::Param param) const { return shaderParams.GetArg(param).GetBool(); }
	void SetFloat(nShaderState::Param param, float val);
	float GetFloat(nShaderState::Param param) const { return shaderParams.GetArg(param).GetFloat(); }
	void SetVector(nShaderState::Param param, const vector4& val);
	const vector4& GetVector(nShaderState::Param param) const { return shaderParams.GetArg(param).GetVector4(); }
	int GetNumParams() const { return shaderParams.GetNumValidParams(); }
	nShaderParams& GetShaderParams() { return shaderParams; }
	bool HasParam(nShaderState::Param param) { return shaderParams.IsParameterValid(param); }
};
//-----------------------------------------------------------------------------

inline void nSkyState::SetInt(nShaderState::Param param, int val)
{
	if (nShaderState::InvalidParameter != param)
		shaderParams.SetArg(param, nShaderArg(val));
}
//---------------------------------------------------------------------

inline void nSkyState::SetBool(nShaderState::Param param, bool val)
{
	if (nShaderState::InvalidParameter != param)
		shaderParams.SetArg(param, nShaderArg(val));
}
//---------------------------------------------------------------------

inline void nSkyState::SetFloat(nShaderState::Param param, float val)
{
	if (nShaderState::InvalidParameter != param)
		shaderParams.SetArg(param, nShaderArg(val));
}
//---------------------------------------------------------------------

inline void nSkyState::SetVector(nShaderState::Param param, const vector4& val)
{
	if (nShaderState::InvalidParameter != param)
		shaderParams.SetArg(param, nShaderArg(val));
}
//---------------------------------------------------------------------

#endif

