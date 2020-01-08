#pragma once
#include <Data/Ptr.h>
#include <Math/Polar.h>
#include <System/System.h>

// Camera controller provides high-level methods for camera manipulation.
// - third-person control logic (first-person may be added later)
// - collision (penetration compensation) and occlusion handling
// - control: input processing, direct interface for scripts and external systems
// - effects: movement smoothing, path following, target following, shaking

//???separate 1st/3rd person behaviour and other logic (effects, collision, occlusion etc)?

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace Frame
{

class CCameraController final
{
protected:

	Scene::PSceneNode _Node;

	CPolar	Angles;
	vector3	COI;          // Center of interest, eye target in parent coordinates
	float	Distance = 1.f;

	float	MinVertAngle = 0.f;
	float	MaxVertAngle = PI * 0.5f;
	float	MinDistance = 0.01f;
	float	MaxDistance = 10000.f;

	bool	Dirty = true;

public:

	void           SetNode(Scene::PSceneNode Node);
	void           Update(float dt);

	void           SetVerticalAngleLimits(float Min, float Max);
	void           SetDistanceLimits(float Min, float Max);

	void           SetVerticalAngle(float AngleRad);
	void           SetHorizontalAngle(float AngleRad);
	void           SetDirection(const vector3& Dir);
	void           SetDistance(float Value);
	void           SetCOI(const vector3& NewCOI);

	void           OrbitVertical(float AngleRad);
	void           OrbitHorizontal(float AngleRad);
	void           Zoom(float Amount);
	void           Move(const vector3& Translation);

	float          GetVerticalAngleMin() const { return MinVertAngle; }
	float          GetVerticalAngleMax() const { return MaxVertAngle; }
	float          GetDistanceMin() const { return MinDistance; }
	float          GetDistanceMax() const { return MaxDistance; }
	float          GetVerticalAngle() const {return Angles.Theta; }
	float          GetHorizontalAngle() const {return Angles.Phi; }
	float          GetDistance() const {return Distance; }
	const vector3& GetCOI() const { return COI; }
};

}
