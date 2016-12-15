#include "CameraManager.h"

#include <Scene/SceneNode.h>

namespace Scene
{
__ImplementClassNoFactory(Scene::CCameraManager, Core::CObject);

bool CCameraManager::InitThirdPersonCamera(CSceneNode& NodeWithCamera)
{
	Ctlr = n_new(CNodeControllerThirdPerson);
	if (Ctlr.IsNullPtr()) FAIL;
	IsThirdPerson = true;

	pCameraNode = &NodeWithCamera;
	pCameraNode->SetController(Ctlr);
	Ctlr->Activate(true);

	OK;
}
//---------------------------------------------------------------------

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
	//Position.Reset(CoreSrv->GetTime(), 0.0001f, GetEntity()->GetAttr<float>(CStrID("CameraLinearGain), Tfm.Translation());
	//Lookat.Reset(CoreSrv->GetTime(), 0.0001f, GetEntity()->GetAttr<float>(CStrID("CameraAngularGain),
	//	Tfm.Translation() - (Tfm.AxisZ() * 10.0f));

	GetEntity()->GetLevel().GetScene()->SetMainCamera(Camera);
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
	vector3 goalPos = lookatPoint + Angles.GetCartesianZ() * Distance;
	vector3 CorrectedGoalPos = DoCollideCheck(lookatPoint, goalPos);

	//!!!define constant somewhere!
	if ((lookatPoint - CorrectedGoalPos).SqLength() > 1.5f * 1.5f) goalPos = CorrectedGoalPos;

	CTime Time = CoreSrv->GetTime();

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
	Entities.Add(Entity);
	UIDToEntity.Add(Entity->GetUID(), Entity.GetUnsafe());
	return Entity;
}
//---------------------------------------------------------------------
*/

}