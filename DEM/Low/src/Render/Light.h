#pragma once
#include <Core/RTTIBaseClass.h>
#include <Math/Vector4.h>

// A base class for light source instances used for rendering. This is a GPU-friendly implementation
// created from CLightAttribute found in a 3D scene. Subclass for different source types.

class matrix44;

namespace Render
{
typedef std::unique_ptr<class CLight> PLight;

// NB: must match light type codes in HLSL
enum class ELightType : U32
{
	Directional = 0,
	Point,
	Spot,
	IBL,
	COUNT,
	Invalid = INVALID_INDEX_T<std::underlying_type_t<ELightType>>
};

struct alignas(16) CGPULightInfo
{
	vector3    Color; //???TODO: normalize color and extract intensity instead of _PAD1, for correct attenuation?
	float      _PAD1;
	vector3    Position;
	float      SqInvRange;		// For attenuation // FIXME: could instead use attenuation formulas based on intensity
	vector4    Params;			// Spot: x - cos inner, y - cos outer
	vector3    InvDirection;
	ELightType Type = ELightType::Invalid;
};

class CLight : public DEM::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(Render::CLight, DEM::Core::CRTTIBaseClass);

public:

	CGPULightInfo GPUData;
	U32           GPUIndex = INVALID_INDEX_T<U32>;
	U32           BoundsVersion = 0;
	bool          GPUDirty = true;
	bool          IsVisible = false;
	bool          TrackObjectLightIntersections = false;

	//???shadow casting flag etc here?
};

}
