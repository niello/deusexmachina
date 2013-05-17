#include "PropChaseCamera.h"

#include <Time/TimeServer.h>
#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Camera/Event/CameraOrbit.h>
#include <Camera/Event/CameraDistance.h>
#include <Physics/Prop/PropPhysics.h>
#include <Physics/PhysicsUtil.h>
#include <Scene/PropSceneNode.h>
#include <Scene/SceneServer.h>

//BEGIN_ATTRS_REGISTRATION(PropChaseCamera)
//	RegisterFloatWithDefault(CameraDistance, ReadOnly, 7.0f);
//	RegisterFloatWithDefault(CameraMinDistance, ReadOnly, 1.5f);
//	RegisterFloatWithDefault(CameraMaxDistance, ReadOnly, 15.0f);
//	RegisterFloatWithDefault(CameraAngularVelocity, ReadOnly, 6.0f);
//	RegisterVector3WithDefault(CameraOffset, ReadOnly, vector4(0.0f, 1.5f, 0.0f, 0.f));
//	RegisterFloatWithDefault(CameraLowStop, ReadOnly, 5.0f);
//	RegisterFloatWithDefault(CameraHighStop, ReadOnly, 45.0f);
//	RegisterFloatWithDefault(CameraDistanceStep, ReadOnly, 1.0f);
//	RegisterFloatWithDefault(CameraLinearGain, ReadOnly, -10.0f);
//	RegisterFloatWithDefault(CameraAngularGain, ReadOnly, -15.0f);
//	RegisterFloatWithDefault(CameraDefaultTheta, ReadOnly, 20.0f);
//	//???DefineFloatWithDefault(CameraDefaultRho, 'CRHO', ReadWrite, n_deg2rad(10.0f));?
//END_ATTRS_REGISTRATION

namespace Prop
{
__ImplementClass(Prop::CPropChaseCamera, 'PCHC', Prop::CPropCamera);

using namespace Game;

void CPropChaseCamera::Activate()
{
	CPropCamera::Activate();
	ResetCamera();

	PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropChaseCamera, OnPropsActivated);
	PROP_SUBSCRIBE_PEVENT(CameraReset, CPropChaseCamera, OnCameraReset);
	PROP_SUBSCRIBE_NEVENT(CameraOrbit, CPropChaseCamera, OnCameraOrbit);
	PROP_SUBSCRIBE_NEVENT(CameraDistance, CPropChaseCamera, OnCameraDistanceChange);
}
//---------------------------------------------------------------------

void CPropChaseCamera::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(CameraReset);
	UNSUBSCRIBE_EVENT(CameraOrbit);
	UNSUBSCRIBE_EVENT(CameraDistance);

	if (Ctlr.IsValid())
	{
		Ctlr->Activate(false);
		Ctlr = NULL;
	}
	if (Node.IsValid())
	{
		Node->RemoveFromParent();
		Node = NULL;
	}

	CPropCamera::Deactivate();
}
//---------------------------------------------------------------------

bool CPropChaseCamera::OnPropsActivated(const Events::CEventBase& Event)
{
	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (!pProp || !pProp->GetNode()) OK; // No node to chase

	Node = pProp->GetNode()->CreateChild(CStrID("ChaseCamera"));
	Camera = n_new(Scene::CCamera);
	//!!!setup camera params here! or mb load camera from SCN
	Node->AddAttr(*Camera);
	Ctlr = n_new(Scene::CAnimControllerThirdPerson);
	Node->Controller = Ctlr;
	Ctlr->Activate(true);

	Ctlr->SetVerticalAngleLimits(n_deg2rad(GetEntity()->GetAttr<float>(CStrID("CameraLowStop"))),
		n_deg2rad(GetEntity()->GetAttr<float>(CStrID("CameraHighStop"))));
	Ctlr->SetDistanceLimits(GetEntity()->GetAttr<float>(CStrID("CameraMinDistance")),
		GetEntity()->GetAttr<float>(CStrID("CameraMaxDistance")));
	Ctlr->SetAngles(Angles.theta, Angles.phi);
	Ctlr->SetDistance(Distance);

	OK;
}
//---------------------------------------------------------------------

void CPropChaseCamera::OnObtainCameraFocus()
{
	// initialize the feedback loops with the current pCamera values so
	// that we get a smooth interpolation to the new position
	//Graphics::CCameraEntity* pCamera = RenderSrv->GetDisplay().GetCamera();
	//const matrix44& Tfm = pCamera->GetTransform();
	//Position.Reset(TimeSrv->GetTime(), 0.0001f, GetEntity()->GetAttr<float>(CStrID("CameraLinearGain), Tfm.Translation());
	//Lookat.Reset(TimeSrv->GetTime(), 0.0001f, GetEntity()->GetAttr<float>(CStrID("CameraAngularGain),
	//	Tfm.Translation() - (Tfm.AxisZ() * 10.0f));

	GetEntity()->GetLevel().GetScene()->SetMainCamera(Camera);

	CPropCamera::OnObtainCameraFocus();
}
//---------------------------------------------------------------------

void CPropChaseCamera::OnRender()
{
	if (HasFocus()) UpdateCamera();
	CPropCamera::OnRender();
}
//---------------------------------------------------------------------

bool CPropChaseCamera::OnCameraReset(const CEventBase& Event)
{
	ResetCamera();
	OK;
}
//---------------------------------------------------------------------

bool CPropChaseCamera::OnCameraOrbit(const CEventBase& Event)
{
	const Event::CameraOrbit& e = (const Event::CameraOrbit&)Event;
	float dt = (float)TimeSrv->GetFrameTime();
	float AngVel = n_deg2rad(GetEntity()->GetAttr<float>(CStrID("CameraAngularVelocity")));

	if (e.DirHoriz != 0)
		Angles.phi += AngVel * dt * (float)e.DirHoriz;
	if (e.DirVert != 0)
		Angles.theta += AngVel * dt * (float)e.DirVert;

	Angles.phi += e.AngleHoriz;
	Angles.theta = n_clamp(Angles.theta + e.AngleVert,
						n_deg2rad(GetEntity()->GetAttr<float>(CStrID("CameraLowStop"))),
						n_deg2rad(GetEntity()->GetAttr<float>(CStrID("CameraHighStop"))));

	if (Ctlr.IsValid())
	{
		Ctlr->OrbitHorizontal(e.AngleHoriz);
		Ctlr->OrbitVertical(e.AngleVert);
	}

	OK;
}
//---------------------------------------------------------------------

bool CPropChaseCamera::OnCameraDistanceChange(const CEventBase& Event)
{
	Distance = n_clamp(
		Distance + ((const Event::CameraDistance&)Event).RelChange * GetEntity()->GetAttr<float>(CStrID("CameraDistanceStep")),
		GetEntity()->GetAttr<float>(CStrID("CameraMinDistance")),
		GetEntity()->GetAttr<float>(CStrID("CameraMaxDistance")));

	if (Ctlr.IsValid())
		Ctlr->Zoom(((const Event::CameraDistance&)Event).RelChange * GetEntity()->GetAttr<float>(CStrID("CameraDistanceStep")));

	OK;
}
//---------------------------------------------------------------------

vector3 CPropChaseCamera::DoCollideCheck(const vector3& from, const vector3& to)
{
	/* WITH pre-created filterset from N3:
	
	static const vector up(0.0f, 1.0f, 0.0f);
    matrix44 m = matrix44::lookatrh(from, to, up);
    float outContactDist = 1.0f;
#if __USE_PHYSICS__    
    Physics::CPhysicsUtil::RayBundleCheck(from, to, up, m.get_xaxis(), 0.25f, this->collideExcludeSet, outContactDist);
#endif    
    vector vec = vector::normalize(to - from);    
    point newTo = from + vec * outContactDist;
    return newTo;
*/
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
}
//---------------------------------------------------------------------

void CPropChaseCamera::UpdateCamera()
{
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

	// update the graphics subsystem pCamera
	//pCamera->SetTransform(CameraMatrix);
}
//---------------------------------------------------------------------

void CPropChaseCamera::ResetCamera()
{
	Angles.set(GetEntity()->GetAttr<matrix44>(CStrID("Transform")).AxisZ());
	Angles.theta = n_deg2rad(GetEntity()->GetAttr<float>(CStrID("CameraDefaultTheta")));
	Distance = GetEntity()->GetAttr<float>(CStrID("CameraDistance"));
}
//---------------------------------------------------------------------

} // namespace Prop
