#pragma once
#include <Core/RTTIBaseClass.h>

// A base class for light source instances used for rendering. This is a GPU-friendly implementation
// created from CLightAttribute found in a 3D scene. Subclass for different source types.

class matrix44;

namespace Render
{
typedef std::unique_ptr<class CLight> PLight;

class CLight : public Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(Render::CLight, Core::CRTTIBaseClass);

protected:

	// ...

public:

	U32  GPUIndex = INVALID_INDEX_T<U32>;
	U32  BoundsVersion = 0;
	bool IsVisible = false;
	bool TrackObjectLightIntersections = false;

	// color and intensity? even for IBL?
	//???shadow casting flag etc here?
};

}
