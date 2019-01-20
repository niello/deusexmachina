#pragma once
#include <Render/ShaderLoader.h>

// Direct3D11 shader loader implementations for different shader types.
// Uses engine-specific data format.

namespace Resources
{

class CD3D11ShaderLoader: public CShaderLoader
{
public:

	virtual const Core::CRTTI&	GetResultType() const override;
	virtual PResourceObject		CreateResource(CStrID UID);
};

}
