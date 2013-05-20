#include "CameraManager.h"

#include <Game/GameServer.h>
#include <Scene/Scene.h>

namespace Game
{
__ImplementClassNoFactory(Game::CCameraManager, Core::CRefCounted);
__ImplementSingleton(Game::CCameraManager);

bool CCameraManager::InitThirdPersonCamera()
{
	Scene::PAnimController Ctlr = n_new(Scene::CAnimControllerThirdPerson);
	if (!Ctlr.IsValid()) FAIL;

	CGameLevel* pLevel = GameSrv->GetActiveLevel();
	if (pLevel && pLevel->GetScene())
	{
		pCameraNode = pLevel->GetScene()->GetMainCamera().GetNode();
		pCameraNode->Controller = Ctlr;
	}

	OK;
}
//---------------------------------------------------------------------

/* activation
	Ctlr = n_new(Scene::CAnimControllerThirdPerson);
	Node->Controller = Ctlr;
	Ctlr->Activate(true);

	Ctlr->SetVerticalAngleLimits(n_deg2rad(GetEntity()->GetAttr<float>(CStrID("CameraLowStop"))),
		n_deg2rad(GetEntity()->GetAttr<float>(CStrID("CameraHighStop"))));
	Ctlr->SetDistanceLimits(GetEntity()->GetAttr<float>(CStrID("CameraMinDistance")),
		GetEntity()->GetAttr<float>(CStrID("CameraMaxDistance")));
	Ctlr->SetAngles(Angles.theta, Angles.phi);
	Ctlr->SetDistance(Distance);
*/

/* deactivation
	if (Ctlr.IsValid())
	{
		Ctlr->Activate(false);
		Ctlr = NULL;
	}
*/

/* changing targer / camera
	// initialize the feedback loops with the current pCamera values so
	// that we get a smooth interpolation to the new position
	//Graphics::CCameraEntity* pCamera = RenderSrv->GetDisplay().GetCamera();
	//const matrix44& Tfm = pCamera->GetTransform();
	//Position.Reset(TimeSrv->GetTime(), 0.0001f, GetEntity()->GetAttr<float>(CStrID("CameraLinearGain), Tfm.Translation());
	//Lookat.Reset(TimeSrv->GetTime(), 0.0001f, GetEntity()->GetAttr<float>(CStrID("CameraAngularGain),
	//	Tfm.Translation() - (Tfm.AxisZ() * 10.0f));

	GetEntity()->GetLevel().GetScene()->SetMainCamera(Camera);
*/

/*orbiting
	const Event::CameraOrbit& e = (const Event::CameraOrbit&)Event;
	float dt = (float)TimeSrv->GetFrameTime();
	float AngVel = n_deg2rad(GetEntity()->GetAttr<float>(CStrID("CameraAngularVelocity")));

	if (e.DirHoriz != 0)
		Angles.phi += AngVel * dt * (float)e.DirHoriz;
	if (e.DirVert != 0)
		Angles.theta += AngVel * dt * (float)e.DirVert;

	Angles.phi += e.AngleHoriz;
	Angles.theta = Angles.theta + e.AngleVert;

	if (Ctlr.IsValid())
	{
		Ctlr->OrbitHorizontal(e.AngleHoriz);
		Ctlr->OrbitVertical(e.AngleVert);
	}
*/

/*zooming
	Distance = Distance + ((const Event::CameraDistance&)Event).RelChange * GetEntity()->GetAttr<float>(CStrID("CameraDistanceStep"));

	if (Ctlr.IsValid())
		Ctlr->Zoom(((const Event::CameraDistance&)Event).RelChange * GetEntity()->GetAttr<float>(CStrID("CameraDistanceStep")));
*/

/*collision test
	WITH pre-created filterset from N3:
	
	static const vector up(0.0f, 1.0f, 0.0f);
    matrix44 m = matrix44::lookatrh(from, to, up);
    float outContactDist = 1.0f;
#if __USE_PHYSICS__    
    Physics::CPhysicsUtil::RayBundleCheck(from, to, up, m.get_xaxis(), 0.25f, this->collideExcludeSet, outContactDist);
#endif    
    vector vec = vector::normalize(to - from);    
    point newTo = from + vec * outContactDist;
    return newTo;

	static const vector3 up(0.0f, 1.0f, 0.0f);
	matrix44 m;
	m.set_translation(from);
	m.lookatRh(to, up);

	// Setup the exclude set for the ray check
	float outContactDist;
	Physics::CFilterSet ExcludeSet;
	CPropAbstractPhysics* physProp = GetEntity()->GetProperty<CPropAbstractPhysics>();
	if (physProp && physProp->IsEnabled())
	{
		Physics::CEntity* physEntity = physProp->GetPhysicsEntity();
		if (physEntity) ExcludeSet.AddEntityID(physEntity->GetUID());
	}
	Physics::CPhysicsUtil::RayBundleCheck(from, to, up, m.AxisX(), 0.25f, outContactDist, &ExcludeSet);

	vector3 vec = to - from;
	vec.norm();
	vector3 newTo = from + vec * outContactDist;

	return newTo;
*/

/*updating
	// compute the lookat point in global space
	const matrix44& m44 = GetEntity()->GetAttr<matrix44>(CStrID("Transform"));
	matrix33 m33 = matrix33(m44.AxisX(), m44.AxisY(), m44.AxisZ());
	vector3 Offset;
	GetEntity()->GetAttr<vector3>(CStrID("CameraOffset"), Offset);
	vector3 lookatPoint = m44.Translation() + m33 * Offset;

	// compute the collided goal position
	vector3 goalPos = lookatPoint + Angles.get_cartesian_z() * Distance;
	vector3 CorrectedGoalPos = DoCollideCheck(lookatPoint, goalPos);

	//!!!define constant somewhere!
	if ((lookatPoint - CorrectedGoalPos).lensquared() > 1.5f * 1.5f) goalPos = CorrectedGoalPos;

	nTime Time = TimeSrv->GetTime();

	// check if the pCamera is currently at the origin, if yes it is in its initial
	// position and should not interpolate towards its target position
	//const vector3& camPos = pCamera->GetTransform().Translation();
	//if (camPos.isequal(vector3::Zero, 0.0f))
	//{
	//	Position.Reset(Time, 0.0001f, GetEntity()->GetAttr<float>(CStrID("CameraLinearGain), goalPos);
	//	Lookat.Reset(Time, 0.0001f, GetEntity()->GetAttr<float>(CStrID("CameraAngularGain), lookatPoint);
	//}

	// feed and update the feedback loops
	Position.Goal = goalPos;
	Lookat.Goal = lookatPoint;
	Position.Update(Time);
	Lookat.Update(Time);

	// construct the new camera matrix
	matrix44 CameraMatrix;
	CameraMatrix.translate(Position.State);
	CameraMatrix.lookatRh(Lookat.State, vector3::Up);
*/

/*reset
	Angles.set(GetEntity()->GetAttr<matrix44>(CStrID("Transform")).AxisZ());
	Angles.theta = n_deg2rad(GetEntity()->GetAttr<float>(CStrID("CameraDefaultTheta")));
	Distance = GetEntity()->GetAttr<float>(CStrID("CameraDistance"));
*/

/*
PEntity CCameraManager::CreateEntity(CStrID UID, CGameLevel& Level)
{
	n_assert(UID.IsValid());
	n_assert(!EntityExists(UID)); //???return NULL or existing entity?
	PEntity Entity = n_new(CEntity(UID, Level));
	Entities.Append(Entity);
	UIDToEntity.Add(Entity->GetUID(), Entity.GetUnsafe());
	return Entity;
}
//---------------------------------------------------------------------
*/

}