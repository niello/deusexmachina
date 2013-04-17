#include "PhysicsServer.h"

#include <Physics/Level.h>
#include <Physics/Composite.h>
#include <Physics/Collision/BoxShape.h>
#include <Physics/Collision/SphereShape.h>
#include <Physics/Collision/CapsuleShape.h>
#include <Physics/Collision/MeshShape.h>
#include <Physics/Collision/HeightfieldShape.h>
#include <Physics/Ray.h>
#include <Scene/SceneServer.h> // For 3D ray picking. To EnvQuaryMgr?
#include <Data/DataServer.h>
#include <Data/DataArray.h>

namespace Physics
{
__ImplementClass(Physics::CPhysicsServer, 'PHSR', Core::CRefCounted);
__ImplementSingleton(Physics::CPhysicsServer);

uint CPhysicsServer::UniqueStamp = 0;

static const nString PhysClassPrefix("Physics::C");

CPhysicsServer::CPhysicsServer():
	isOpen(false),
	Contacts(256, 256),
	Entities(1024, 1024)
{
	__ConstructSingleton;
	Contacts.SetFlags(CContacts::DoubleGrowSize);
}
//---------------------------------------------------------------------

CPhysicsServer::~CPhysicsServer()
{
	n_assert(!isOpen);
	n_assert(!CurrLevel.isvalid()); //???need? mb release it right here if not NULL?
	__DestructSingleton;
}
//---------------------------------------------------------------------

// A NULL pointer is valid and will just release the previous level.
void CPhysicsServer::SetLevel(CLevel* pLevel)
{
	n_assert(isOpen);
	if (CurrLevel.isvalid())
	{
		CurrLevel->Deactivate();
		CurrLevel = NULL;
	}
	if (pLevel)
	{
		CurrLevel = pLevel;
		CurrLevel->Activate();
	}
}
//---------------------------------------------------------------------

bool CPhysicsServer::Open()
{
	n_assert(!isOpen);
	CMaterialTable::Setup();
	isOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CPhysicsServer::Close()
{
	n_assert(isOpen);
	SetLevel(NULL);
	isOpen = false;
}
//---------------------------------------------------------------------

// Perform one or more simulation steps. The number of simulation steps
// performed depends on the time of the last call to Trigger().
void CPhysicsServer::Trigger()
{
	n_assert(isOpen);
	if (CurrLevel.isvalid()) CurrLevel->Trigger();
}
//---------------------------------------------------------------------

// Do a ray check starting from position `pos' along direction `dir'.
// Make resulting intersection points available in `GetIntersectionPoints()'.
bool CPhysicsServer::RayCheck(const vector3& Pos, const vector3& Dir, const CFilterSet* ExcludeSet)
{
	Contacts.Clear();
	CRay R;
	R.SetOrigin(Pos);
	R.SetDirection(Dir);
	if (ExcludeSet) R.SetExcludeFilterSet(*ExcludeSet);
	R.DoRayCheckAllContacts(matrix44::identity, Contacts);
	return Contacts.Size() > 0;
}
//---------------------------------------------------------------------

CBoxShape* CPhysicsServer::CreateBoxShape(const matrix44& TF, CMaterialType MatType, const vector3& Size) const
{
	CBoxShape* Shape = CBoxShape::Create();
	Shape->SetTransform(TF);
	Shape->SetMaterialType(MatType);
	Shape->SetSize(Size);
	return Shape;
}
//---------------------------------------------------------------------

CSphereShape* CPhysicsServer::CreateSphereShape(const matrix44& TF, CMaterialType MatType, float Radius) const
{
	CSphereShape* Shape = CSphereShape::Create();
	Shape->SetTransform(TF);
	Shape->SetMaterialType(MatType);
	Shape->SetRadius(Radius);
	return Shape;
}
//---------------------------------------------------------------------

CCapsuleShape* CPhysicsServer::CreateCapsuleShape(const matrix44& TF, CMaterialType MatType,
												  float Radius, float Length) const
{
	CCapsuleShape* Shape = CCapsuleShape::Create();
	Shape->SetTransform(TF);
	Shape->SetMaterialType(MatType);
	Shape->SetRadius(Radius);
	Shape->SetLength(Length);
	return Shape;
}
//---------------------------------------------------------------------

CMeshShape* CPhysicsServer::CreateMeshShape(const matrix44& TF, CMaterialType MatType, const nString& FileName) const
{
	CMeshShape* Shape = CMeshShape::Create();
	Shape->SetTransform(TF);
	Shape->SetMaterialType(MatType);
	Shape->SetFileName(FileName);
	return Shape;
}
//---------------------------------------------------------------------

CHeightfieldShape* CPhysicsServer::CreateHeightfieldShape(const matrix44& TF, CMaterialType MatType, const nString& FileName) const
{
	CHeightfieldShape* Shape = CHeightfieldShape::Create();
	Shape->SetTransform(TF);
	Shape->SetMaterialType(MatType);
	Shape->SetFileName(FileName);
	return Shape;
}
//---------------------------------------------------------------------

Physics::CRay* CPhysicsServer::CreateRay(const vector3& Origin, const vector3& Dir) const
{
	CRay* R = CRay::Create();
	R->SetOrigin(Origin);
	R->SetDirection(Dir);
	return R;
}
//---------------------------------------------------------------------

//!!!???to loaders?!
CComposite* CPhysicsServer::LoadCompositeFromPRM(const nString& Name) const
{
	n_assert(Name.IsValid());
	PParams Desc = DataSrv->LoadPRM(nString("physics:") + Name + ".prm");
	if (!Desc.isvalid()) return NULL;

	CComposite* pComposite = (CComposite*)CoreFct->Create(PhysClassPrefix + Desc->Get<nString>(CStrID("Type")));
	if (!pComposite) return NULL;

	int Idx = Desc->IndexOf(CStrID("Bodies"));
	if (Idx != INVALID_INDEX)
	{
		CDataArray& Bodies = *Desc->Get<PDataArray>(Idx);
		pComposite->BeginBodies(Bodies.Size());
		for (int i = 0; i < Bodies.Size(); i++)
		{
			PParams BodyDesc = Bodies[i];
			PRigidBody pBody = CRigidBody::Create();
			pBody->Name = BodyDesc->Get<nString>(CStrID("Name"));
			pBody->CollideConnected = BodyDesc->Get<bool>(CStrID("CollideConnected"), false);
			matrix44 InitialTfm(quaternion(
				BodyDesc->Get<float>(CStrID("RotX")),
				BodyDesc->Get<float>(CStrID("RotY")),
				BodyDesc->Get<float>(CStrID("RotZ")),
				BodyDesc->Get<float>(CStrID("RotW"))));
			InitialTfm.translate(vector3(
				BodyDesc->Get<float>(CStrID("PosX")),
				BodyDesc->Get<float>(CStrID("PosY")),
				BodyDesc->Get<float>(CStrID("PosZ"))));
			pBody->SetInitialTransform(InitialTfm);
			pBody->SetLinkName(CRigidBody::ModelNode, BodyDesc->Get<nString>(CStrID("Model"), NULL));
			pBody->SetLinkName(CRigidBody::ShadowNode, BodyDesc->Get<nString>(CStrID("Shadow"), NULL));
			pBody->SetLinkName(CRigidBody::JointNode, BodyDesc->Get<nString>(CStrID("Joint"), NULL));

			CDataArray& Shapes = *BodyDesc->Get<PDataArray>(CStrID("Shapes"));
			pBody->BeginShapes(Shapes.Size());
			for (int j = 0; j < Shapes.Size(); j++)
			{
				PParams ShapeDesc = Shapes[j];
				PShape pShape = (CShape*)CoreFct->Create(PhysClassPrefix + ShapeDesc->Get<nString>(CStrID("Type")));
				pShape->Init(ShapeDesc);
				pBody->AddShape(pShape);
			}
			pBody->EndShapes();

			pComposite->AddBody(pBody);
		}
		pComposite->EndBodies();
	}
	
	Idx = Desc->IndexOf(CStrID("Joints"));
	if (Idx != INVALID_INDEX)
	{
		CDataArray& Joints = *Desc->Get<PDataArray>(Idx);
		pComposite->BeginJoints(Joints.Size());
		for (int i = 0; i < Joints.Size(); i++)
		{
			PParams JointDesc = Joints[i];
			PJoint pJoint = (CJoint*)CoreFct->Create(PhysClassPrefix + JointDesc->Get<nString>(CStrID("Type")));
			Idx = Desc->IndexOf(CStrID("Body1"));
			if (Idx != INVALID_INDEX)
				pJoint->SetBody1(pComposite->GetBodyByName(Desc->Get(Idx).GetRawValue()));
			Idx = Desc->IndexOf(CStrID("Body2"));
			if (Idx != INVALID_INDEX)
				pJoint->SetBody2(pComposite->GetBodyByName(Desc->Get(Idx).GetRawValue()));
			pJoint->LinkName = Desc->Get<nString>(CStrID("Joint"), NULL);
			pJoint->Init(JointDesc);
			pComposite->AddJoint(pJoint);
		}
		pComposite->EndJoints();
	}

	Idx = Desc->IndexOf(CStrID("Shapes"));
	if (Idx != INVALID_INDEX)
	{
		CDataArray& Shapes = *Desc->Get<PDataArray>(Idx);
		pComposite->BeginShapes(Shapes.Size());
		for (int i = 0; i < Shapes.Size(); i++)
		{
			PParams ShapeDesc = Shapes[i];
			PShape pShape = (CShape*)CoreFct->Create("Physics::C" + ShapeDesc->Get<nString>(CStrID("Type")));
			pShape->Init(ShapeDesc);
			pComposite->AddShape(pShape);
		}
		pComposite->EndShapes();
	}

	return pComposite;
}
//---------------------------------------------------------------------

const CContactPoint* CPhysicsServer::GetClosestContactAlongRay(const vector3& Pos,
															   const vector3& Dir,
															   const CFilterSet* ExcludeSet)
{
	RayCheck(Pos, Dir, ExcludeSet);

	// Find closest contact
	int Idx = INVALID_INDEX;
	float ClosestDistanceSq = Dir.lensquared();
	for (int i = 0; i < Contacts.Size(); i++)
	{
		const CContactPoint& CurrContact = Contacts[i];
		float DistanceSq = (CurrContact.Position - Pos).lensquared();
		if (DistanceSq < ClosestDistanceSq)
		{
			Idx = i;
			ClosestDistanceSq = DistanceSq;
		}
	}
	return (Idx != INVALID_INDEX) ? &Contacts[Idx] : NULL;
}
//---------------------------------------------------------------------

// NB: Camera must be updated before this check
const CContactPoint* CPhysicsServer::GetClosestContactUnderMouse(const vector2& MousePosRel,
																 float Length,
																 const CFilterSet* ExcludeSet)
{
	line3 Ray;
	SceneSrv->GetCurrentScene()->GetMainCamera()->GetRay3D(MousePosRel.x, MousePosRel.y, Length, Ray);
	return GetClosestContactAlongRay(Ray.start(), Ray.vec(), ExcludeSet);
}
//---------------------------------------------------------------------

// Apply an impulse on the first rigid body which lies along the defined ray.
bool CPhysicsServer::ApplyImpulseAlongRay(const vector3& Pos, const vector3& Dir, float Impulse,
										  const CFilterSet* ExcludeSet)
{
	const CContactPoint* pContact = GetClosestContactAlongRay(Pos, Dir, ExcludeSet);
	if (pContact)
	{
		CRigidBody* Body = pContact->GetRigidBody();
		if (Body)
		{
			vector3 NormDir = Dir;
			NormDir.norm();
			Body->ApplyImpulseAtPos(pContact->Position, NormDir * Impulse);
			OK;
		}
	}
	FAIL;
}
//---------------------------------------------------------------------

void CPhysicsServer::RenderDebug()
{
	n_assert(CurrLevel);
	//GFX
	/*
	nGfxServer2::Instance()->BeginShapes();
	CurrLevel->RenderDebug();
	nGfxServer2::Instance()->EndShapes();
	*/
}
//---------------------------------------------------------------------

int CPhysicsServer::GetEntitiesInShape(PShape Shape, const CFilterSet& ExcludeSet,
									   nArray<PEntity>& Result)
{
	n_assert(CurrLevel);

	Shape->Attach(CurrLevel->GetODEDynamicSpaceID());
	Contacts.Reset();
	Shape->Collide(ExcludeSet, Contacts);
	Shape->Detach();

	int OldResultSize = Result.Size();
	
	//???stamp?
	uint Stamp = GetUniqueStamp();
	for (int i = 0; i < Contacts.Size(); i++)
	{
		CEntity* pEnt = Contacts[i].GetEntity();
		if (pEnt && pEnt->GetStamp() != Stamp)
		{
			pEnt->SetStamp(Stamp);
			Result.Append(pEnt);
		}
	}
	return Result.Size() - OldResultSize;
}
//---------------------------------------------------------------------

// This method returns all physics entities touching the given spherical
// area. The method creates a sphere shape and calls its collide
// method, so it's quite fast. Note that entities will be appended to the
// array, so usually you should make sure to pass an empty array. This method
// will also overwrite the internal Contacts array which can be
// queried after the method has returned, but note that there will only
// be one contact per physics shape.
int CPhysicsServer::GetEntitiesInSphere(const vector3& Pos, float Radius, const CFilterSet& ExcludeSet,
										nArray<PEntity>& Result)
{
	n_assert(Radius >= 0.0f); //???in shape constructor?
	
	matrix44 Tfm;
	Tfm.translate(Pos);
	
	return GetEntitiesInShape(CreateSphereShape(Tfm, InvalidMaterial, Radius), ExcludeSet, Result);
}
//---------------------------------------------------------------------

int CPhysicsServer::GetEntitiesInBox(const vector3& Scale, const matrix44& TF, const CFilterSet& ExcludeSet,
									 nArray<PEntity>& Result)
{
	n_assert(Scale.x >= 0 && Scale.y >= 0 && Scale.z >= 0); //???in shape constructor?

	//???can optimize?
	matrix44 Tfm;
	Tfm.x_component() = TF.x_component() / Scale.x;
	Tfm.y_component() = TF.y_component() / Scale.y;
	Tfm.z_component() = TF.z_component() / Scale.z;
	Tfm.set_translation(TF.pos_component());

	return GetEntitiesInShape(CreateBoxShape(Tfm, InvalidMaterial, Scale), ExcludeSet, Result);
}
//---------------------------------------------------------------------

} // namespace Physics
