#include "PhysicsDebugDraw.h"

#include <Physics/BulletConv.h>
#include <Render/DebugDraw.h>

namespace Physics
{

void CPhysicsDebugDraw::drawLine(const btVector3& from,const btVector3& to,const btVector3& color)
{
	DebugDraw->DrawLine(BtVectorToVector(from), BtVectorToVector(to), BtVectorToVector(color));
}
//---------------------------------------------------------------------

void CPhysicsDebugDraw::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
{
}
//---------------------------------------------------------------------

void CPhysicsDebugDraw::reportErrorWarning(const char* warningString)
{
}
//---------------------------------------------------------------------

void CPhysicsDebugDraw::draw3dText(const btVector3& location,const char* textString)
{
}
//---------------------------------------------------------------------

void CPhysicsDebugDraw::setDebugMode(int debugMode)
{
	Mode = debugMode;
}
//---------------------------------------------------------------------

int CPhysicsDebugDraw::getDebugMode() const
{
	return Mode;
}
//---------------------------------------------------------------------

}