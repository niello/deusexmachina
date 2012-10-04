#ifndef N_SHADER2_H
#define N_SHADER2_H
//------------------------------------------------------------------------------
/**
    @class nShader2
    @ingroup Gfx2

    A shader object loads itself from a shader resource file, and contains
    everything to render a mesh and texture. It may be completely
    render state based, use vertex and pixel shader programs, or both.
    Shaders usually use a 3rd party subsystem, like D3DX effects, or CgFX.
    This is done by subclasses of the nShader2 class.

    How the shader is rendered is totally up to the gfx server.

    For the sake of efficiency, shader parameters are now enums with
    associated string names. The use of enums allows to do parameter lookup
    as simple indexed array lookup. The disadvantage is of course, that
    new shader states require this file to be extended or replaced.

    (C) 2002 RadonLabs GmbH
*/
#include "resource/nresource.h"
#include "gfx2/nshaderparams.h"
#include "gfx2/ninstancestream.h"

class nTexture2;

class nShader2: public nResource
{
public:

	virtual ~nShader2() { if (!IsUnloaded()) Unload(); }

	virtual bool HasTechnique(const char* t) const { return false; }
	virtual bool SetTechnique(const char* t) { return false; }
	virtual const char* GetTechnique() const { return NULL; }
	virtual int UpdateInstanceStreamDecl(nInstanceStream::Declaration& decl) { return 0; }

	virtual bool IsParameterUsed(nShaderState::Param p) { return false; }

	virtual void SetBool(nShaderState::Param p, bool val) {}
	virtual void SetInt(nShaderState::Param p, int val) {}
	virtual void SetFloat(nShaderState::Param p, float val) {}
	virtual void SetVector4(nShaderState::Param p, const vector4& val) {}
	virtual void SetVector3(nShaderState::Param p, const vector3& val) {}
	virtual void SetFloat4(nShaderState::Param p, const nFloat4& val) {}
	virtual void SetMatrix(nShaderState::Param p, const matrix44& val) {}
	virtual void SetTexture(nShaderState::Param p, nTexture2* tex) {}

	virtual void SetBoolArray(nShaderState::Param p, const bool* array, int count) {}
	virtual void SetIntArray(nShaderState::Param p, const int* array, int count) {}
	virtual void SetFloatArray(nShaderState::Param p, const float* array, int count) {}
	virtual void SetFloat4Array(nShaderState::Param p, const nFloat4* array, int count) {}
	virtual void SetVector4Array(nShaderState::Param p, const vector4* array, int count) {}
	virtual void SetMatrixArray(nShaderState::Param p, const matrix44* array, int count) {}
	virtual void SetMatrixPointerArray(nShaderState::Param p, const matrix44** array, int count) {}
	virtual void SetParams(const nShaderParams& params) {}

	virtual int	Begin(bool saveState) { return 0; }
	virtual void BeginPass(int pass) {}
	virtual void CommitChanges() {}
	virtual void EndPass() {}
	virtual void End() {}
};

#endif
