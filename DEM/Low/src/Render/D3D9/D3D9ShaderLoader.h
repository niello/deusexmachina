#pragma once
#include <Render/ShaderLoader.h>

// Direct3D9 shader loader implementations for different shader types.
// Uses engine-specific data format.

namespace Resources
{

class CD3D9ShaderLoader: public CShaderLoader
{
public:

	virtual const Core::CRTTI&	GetResultType() const override;
	virtual PResourceObject		CreateResource(CStrID UID);
};

typedef Ptr<CD3D9ShaderLoader> PD3D9ShaderLoader;

}
