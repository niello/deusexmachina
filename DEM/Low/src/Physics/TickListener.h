#pragma once

// Listener for fixed step physics updates

namespace Physics
{
class CPhysicsLevel;

struct ITickListener
{
	virtual void BeforePhysicsTick(CPhysicsLevel* pLevel, float dt) {}
	virtual void AfterPhysicsTick(CPhysicsLevel* pLevel, float dt) {}
};

}
