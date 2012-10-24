#include "PropChaseCamera.h"

#include <Time/TimeServer.h>
#include <Game/Entity.h>
#include <Gfx/GfxServer.h>
#include <Gfx/CameraEntity.h>
#include <Camera/Event/CameraOrbit.h>
#include <Camera/Event/CameraDistance.h>
#include <Physics/Prop/PropPhysics.h>
#include <Physics/PhysicsUtil.h>
#include <DB/DBServer.h>
#include <Loading/EntityFactory.h>

namespace Attr
{
	DefineFloat(CameraDistance);
	DefineFloat(CameraMinDistance);
	DefineFloat(CameraMaxDistance);
	DefineFloat(CameraDistanceStep);
	DefineFloat(CameraAngularVelocity);
	DefineVector3(CameraOffset);
	DefineFloat(CameraLowStop);
	DefineFloat(CameraHighStop);
	DefineFloat(CameraLinearGain);
	DefineFloat(CameraAngularGain);
	DefineFloat(CameraDefaultTheta);
};

BEGIN_ATTRS_REGISTRATION(PropChaseCamera)
	RegisterFloatWithDefault(CameraDistance, ReadOnly, 7.0f);
	RegisterFloatWithDefault(CameraMinDistance, ReadOnly, 1.5f);
	RegisterFloatWithDefault(CameraMaxDistance, ReadOnly, 15.0f);
	RegisterFloatWithDefault(CameraAngularVelocity, ReadOnly, 6.0f);
	RegisterVector3WithDefault(CameraOffset, ReadOnly, vector4(0.0f, 1.5f, 0.0f, 0.f));
	RegisterFloatWithDefault(CameraLowStop, ReadOnly, -30.0f);
	RegisterFloatWithDefault(CameraHighStop, ReadOnly, 30.0f);
	RegisterFloatWithDefault(CameraDistanceStep, ReadOnly, 1.0f);
	RegisterFloatWithDefault(CameraLinearGain, ReadOnly, -10.0f);
	RegisterFloatWithDefault(CameraAngularGain, ReadOnly, -15.0f);
	RegisterFloatWithDefault(CameraDefaultTheta, ReadOnly, n_deg2rad(-20.0f));
	//???DefineFloatWithDefault(CameraDefaultRho, 'CRHO', ReadWrite, n_deg2rad(10.0f));?
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropChaseCamera, Properties::CPropCamera);
ImplementFactory(Properties::CPropChaseCamera);
RegisterProperty(CPropChaseCamera);

using namespace Game;

void CPropChaseCamera::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CPropCamera::GetAttributes(Attrs);
	Attrs.Append(Attr::CameraDistance);
	Attrs.Append(Attr::CameraMinDistance);
	Attrs.Append(Attr::CameraMaxDistance);
	Attrs.Append(Attr::CameraAngularVelocity);
	Attrs.Append(Attr::CameraOffset);
	Attrs.Append(Attr::CameraLowStop);
	Attrs.Append(Attr::CameraHighStop);
	Attrs.Append(Attr::CameraDistanceStep);
	Attrs.Append(Attr::CameraLinearGain);
	Attrs.Append(Attr::CameraAngularGain);
	Attrs.Append(Attr::CameraDefaultTheta);
}
//---------------------------------------------------------------------

void CPropChaseCamera::Activate()
{
	CPropCamera::Activate();
	ResetCamera();

	PROP_SUBSCRIBE_PEVENT(CameraReset, CPropChaseCamera, OnCameraReset);
	PROP_SUBSCRIBE_NEVENT(CameraOrbit, CPropChaseCamera, OnCameraOrbit);
	PROP_SUBSCRIBE_NEVENT(CameraDistance, CPropChaseCamera, OnCameraDistanceChange);
}
//---------------------------------------------------------------------

void CPropChaseCamera::Deactivate()
{
	UNSUBSCRIBE_EVENT(CameraReset);
	UNSUBSCRIBE_EVENT(CameraOrbit);
	UNSUBSCRIBE_EVENT(CameraDistance);

	CPropCamera::Deactivate();
}
//---------------------------------------------------------------------

void CPropChaseCamera::OnObtainCameraFocus()
{
	// initialize the feedback loops with the current pCamera values so
	// that we get a smooth interpolation to the new position
	Graphics::CCameraEntity* pCamera = GfxSrv->GetCamera();
	const matrix44& Tfm = pCamera->GetTransform();
	Position.Reset(TimeSrv->GetTime(), 0.0001f, GetEntity()->Get<float>(Attr::CameraLinearGain), Tfm.pos_component());
	Lookat.Reset(TimeSrv->GetTime(), 0.0001f, GetEntity()->Get<float>(Attr::CameraAngularGain),
		Tfm.pos_component() - (Tfm.z_component() * 10.0f));

	CPropCamera::OnObtainCameraFocus();
}
//---------------------------------------------------------------------

void CPropChaseCamera::OnRender()
{
	if (CPropChaseCamera::HasFocus()) UpdateCamera();
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
	/*
	    float angularVelocity = this->entity->GetFloat(Attr::CameraAngularVelocity);
    float lowStop = n_deg2rad(this->entity->GetFloat(Attr::CameraLowStop));
    float hiStop  = n_deg2rad(this->entity->GetFloat(Attr::CameraHighStop));
    
    float frameTime = (float) InputTimeSource::Instance()->GetFrameTime();;
    this->cameraAngles.rho += dRho * angularVelocity * frameTime;
    this->cameraAngles.theta += -dTheta * angularVelocity * frameTime;
    this->cameraAngles.theta = n_clamp(this->cameraAngles.theta, lowStop, hiStop);
*/
	const Event::CameraOrbit& e = (const Event::CameraOrbit&)Event;
	float dt = (float)TimeSrv->GetFrameTime();
	float AngVel = n_deg2rad(GetEntity()->Get<float>(Attr::CameraAngularVelocity));

	if (e.DirHoriz != 0)
		Angles.rho += AngVel * dt * (float)e.DirHoriz;
	if (e.DirVert != 0)
		Angles.theta += AngVel * dt * (float)e.DirVert;

	Angles.rho += e.AngleHoriz;
	Angles.theta = n_clamp(Angles.theta + e.AngleVert,
						n_deg2rad(GetEntity()->Get<float>(Attr::CameraLowStop)),
						n_deg2rad(GetEntity()->Get<float>(Attr::CameraHighStop)));
	OK;
}
//---------------------------------------------------------------------

bool CPropChaseCamera::OnCameraDistanceChange(const CEventBase& Event)
{
	Distance = n_clamp(
		Distance + ((const Event::CameraDistance&)Event).RelChange * GetEntity()->Get<float>(Attr::CameraDistanceStep),
		GetEntity()->Get<float>(Attr::CameraMinDistance),
		GetEntity()->Get<float>(Attr::CameraMaxDistance));
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
	CPropAbstractPhysics* physProp = GetEntity()->FindProperty<CPropAbstractPhysics>();
	if (physProp && physProp->IsEnabled())
	{
		Physics::CEntity* physEntity = physProp->GetPhysicsEntity();
		if (physEntity) ExcludeSet.AddEntityID(physEntity->GetUniqueID());
	}
	Physics::CPhysicsUtil::RayBundleCheck(from, to, up, m.x_component(), 0.25f, outContactDist, &ExcludeSet);

	vector3 vec = to - from;
	vec.norm();
	vector3 newTo = from + vec * outContactDist;

	return newTo;
}
//---------------------------------------------------------------------

void CPropChaseCamera::UpdateCamera()
{
	Graphics::CCameraEntity* pCamera = GfxSrv->GetCamera();
	n_assert(pCamera);
	static const vector3 upVec(0.0f, 1.0f, 0.0f);

	// compute the lookat point in global space
	const matrix44& m44 = GetEntity()->Get<matrix44>(Attr::Transform);
	matrix33 m33 = matrix33(m44.x_component(), m44.y_component(), m44.z_component());
	vector3 Offset;
	GetEntity()->Get<vector3>(Attr::CameraOffset, Offset);
	vector3 lookatPoint = m44.pos_component() + m33 * Offset;

	// compute the collided goal position
	matrix44 orbitMatrix;
	orbitMatrix.rotate_x(Angles.theta);
	orbitMatrix.rotate_y(Angles.rho);
	orbitMatrix.translate(lookatPoint);
	vector3 goalPos = orbitMatrix.pos_component() + orbitMatrix.z_component() * Distance;
	vector3 CorrectedGoalPos = DoCollideCheck(lookatPoint, goalPos);

	//!!!define constant somewhere!
	if ((lookatPoint - CorrectedGoalPos).lensquared() > 1.5f * 1.5f) goalPos = CorrectedGoalPos;

	nTime Time = TimeSrv->GetTime();

	// check if the pCamera is currently at the origin, if yes it is in its initial
	// position and should not interpolate towards its target position
	const vector3& camPos = pCamera->GetTransform().pos_component();
	if (camPos.isequal(vector3::Zero, 0.0f))
	{
		Position.Reset(Time, 0.0001f, GetEntity()->Get<float>(Attr::CameraLinearGain), goalPos);
		Lookat.Reset(Time, 0.0001f, GetEntity()->Get<float>(Attr::CameraAngularGain), lookatPoint);
	}

	// feed and update the feedback loops
	Position.Goal = goalPos;
	Lookat.Goal = lookatPoint;
	Position.Update(Time);
	Lookat.Update(Time);

	// construct the new pCamera matrix
	matrix44 CameraMatrix;
	CameraMatrix.translate(Position.State);
	CameraMatrix.lookatRh(Lookat.State, upVec);

	// update the graphics subsystem pCamera
	pCamera->SetTransform(CameraMatrix);
}
//---------------------------------------------------------------------

void CPropChaseCamera::ResetCamera()
{
	float curTheta = GetEntity()->Get<float>(Attr::CameraDefaultTheta);
	Angles.set(GetEntity()->Get<matrix44>(Attr::Transform).z_component());
	Angles.theta = curTheta;
	Distance = GetEntity()->Get<float>(Attr::CameraDistance);
}
//---------------------------------------------------------------------

} // namespace Properties
