#pragma once
#include <LinearMath/btIDebugDraw.h>

// DEM implementation of the Bullet debug draw interface

namespace Debug
{
	class CDebugDraw;
}

namespace Physics
{

class CPhysicsDebugDraw: public btIDebugDraw
{
protected:

	Debug::CDebugDraw& _DebugDraw;
	int Mode;

public:

	CPhysicsDebugDraw(Debug::CDebugDraw& DebugDraw) : _DebugDraw(DebugDraw) {}

	virtual void	drawLine(const btVector3& from,const btVector3& to,const btVector3& color);
	virtual void	drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color);
	virtual void	reportErrorWarning(const char* warningString);
	virtual void	draw3dText(const btVector3& location,const char* textString);	
	virtual void	setDebugMode(int debugMode);
	virtual int		getDebugMode() const;
};

}
