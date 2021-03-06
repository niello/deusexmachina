#pragma once

// Physical properties of the object material
// For values one may look at:
// https://www.thoughtspike.com/friction-coefficients-for-bullet-physics/
// https://en.wikipedia.org/wiki/Rolling_resistance
// https://en.wikipedia.org/wiki/Friction
// http://atc.sjf.stuba.sk/files/mechanika_vms_ADAMS/Contact_Table.pdf
// NB: for 2 object contact parameters there are bullet callbacks:
// gCalculateCombinedRestitutionCallback
// gCalculateCombinedFrictionCallback
// gCalculateCombinedRollingFrictionCallback etc.

// TODO: get predefined by string name?

namespace Physics
{

struct CPhysicsMaterial
{
	static CPhysicsMaterial Default() { return {}; }
	static CPhysicsMaterial DryWood() { return { 0.6f, 0.04f, 0.75f }; }

	float Friction = 0.5f;
	float RollingFriction = 0.f;
	float Bounciness = 1.f;
};

}
