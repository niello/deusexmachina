#include "Composite.h"

#include <Physics/Collision/Shape.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CComposite, Core::CRefCounted);
__ImplementClass(Physics::CComposite);

CComposite::CComposite():
	CurrBodyIdx(0),
	CurrJointIdx(0),
	CurrShapeIdx(0),
	pEntity(NULL),
	ODESpaceID(NULL)
{
}
//---------------------------------------------------------------------

CComposite::~CComposite()
{
	if (IsAttached()) Detach();
	ClearJoints();
	ClearBodies();
	ClearShapes(); //???was forgotten or isn't needed?
}
//---------------------------------------------------------------------

void CComposite::BeginBodies(int Count)
{
	n_assert(!Flags.Is(PHYS_COMP_BEGIN_BODIES));
	ClearBodies();
	Bodies.SetSize(Count);
	CurrBodyIdx = 0;
	Flags.Set(PHYS_COMP_BEGIN_BODIES);
}
//---------------------------------------------------------------------

void CComposite::AddBody(CRigidBody* pBody)
{
	n_assert(Flags.Is(PHYS_COMP_BEGIN_BODIES));
	n_assert(pBody);
	Bodies[CurrBodyIdx++] = pBody;
	pBody->Composite = this;
}
//---------------------------------------------------------------------

void CComposite::EndBodies()
{
	n_assert(Flags.Is(PHYS_COMP_BEGIN_BODIES));
	n_assert(Bodies.GetCount() == CurrBodyIdx);
	Flags.Clear(PHYS_COMP_BEGIN_BODIES);
}
//---------------------------------------------------------------------

void CComposite::BeginJoints(int Count)
{
	n_assert(!Flags.Is(PHYS_COMP_BEGIN_JOINTS));
	ClearJoints();
	Joints.SetSize(Count);
	CurrJointIdx = 0;
	Flags.Set(PHYS_COMP_BEGIN_JOINTS);
}
//---------------------------------------------------------------------

void CComposite::AddJoint(CJoint* pJoint)
{
	n_assert(Flags.Is(PHYS_COMP_BEGIN_JOINTS));
	n_assert(pJoint);
	Joints[CurrJointIdx++] = pJoint;
}
//---------------------------------------------------------------------

void CComposite::EndJoints()
{
	n_assert(Flags.Is(PHYS_COMP_BEGIN_JOINTS));
	n_assert(Joints.GetCount() == CurrJointIdx);
	Flags.Clear(PHYS_COMP_BEGIN_JOINTS);
}
//---------------------------------------------------------------------

void CComposite::BeginShapes(int Count)
{
	n_assert(!Flags.Is(PHYS_COMP_BEGIN_SHAPES));
	ClearShapes();
	Shapes.SetSize(Count);
	CurrShapeIdx = 0;
	Flags.Set(PHYS_COMP_BEGIN_SHAPES);
}
//---------------------------------------------------------------------

void CComposite::AddShape(CShape* pShape)
{
	n_assert(Flags.Is(PHYS_COMP_BEGIN_SHAPES));
	n_assert(pShape);
	Shapes[CurrShapeIdx++] = pShape;
}
//---------------------------------------------------------------------

void CComposite::EndShapes()
{
	n_assert(Flags.Is(PHYS_COMP_BEGIN_SHAPES));
	n_assert(Shapes.GetCount() == CurrShapeIdx);
	Flags.Clear(PHYS_COMP_BEGIN_SHAPES);
}
//---------------------------------------------------------------------

void CComposite::Attach(dWorldID WorldID, dSpaceID DynamicSpaceID, dSpaceID StaticSpaceID)
{
	n_assert(!IsAttached());

	// count the number of shapes in the composite, this dictates whether and what
	// type of local collision space will be created.
	if (Bodies.GetCount() > 0)
	{
		int ShapeCount = 0;
		for (int i = 0; i < Bodies.GetCount(); i++)
			ShapeCount += Bodies[i]->GetNumShapes();

		// if number of shapes is equal to 1 we don't allocate
		// a local collide space, but instead add the shape directly
		// to the global collide space, otherwise create a simple space
		dSpaceID LocalSpaceID = DynamicSpaceID;
		/*
		// FIXME: this doesn't work with the static/dynamic space approach
		if (ShapeCount > 1)
		{
			// create a simple collision space
			ODESpaceID = dHashSpaceCreate(DynamicSpaceID);
			dHashSpaceSetLevels(ODESpaceID, -3, 2);
			LocalSpaceID = ODESpaceID;
		}
		*/

		for (int i = 0; i < Bodies.GetCount(); i++)
		{
			CRigidBody* pBody = Bodies[i];
			pBody->Attach(WorldID, LocalSpaceID, pBody->GetInitialTransform() * Transform);
		}

		for (int i = 0; i < Joints.GetCount(); i++)
		{
			CJoint* pJoint = GetJointAt(i);
			pJoint->Attach(WorldID, 0, Transform);
		}
	}

	for (int i = 0; i < Shapes.GetCount(); i++)
	{
		CShape* pShape = Shapes[i];
		pShape->SetTransform(pShape->GetInitialTransform() * Transform);
		pShape->Attach(StaticSpaceID);
	}

	Flags.Set(PHYS_COMP_ATTACHED);
}
//---------------------------------------------------------------------

void CComposite::Detach()
{
	n_assert(IsAttached());

	for (int i = 0; i < Bodies.GetCount(); i++) Bodies[i]->Detach();
	for (int i = 0; i < Joints.GetCount(); i++) Joints[i]->Detach();
	for (int i = 0; i < Shapes.GetCount(); i++) Shapes[i]->Detach();

	if (ODESpaceID)
	{
		dSpaceDestroy(ODESpaceID);
		ODESpaceID = NULL;
	}

	Flags.Clear(PHYS_COMP_ATTACHED);
}
//---------------------------------------------------------------------

void CComposite::OnStepBefore()
{
	if (IsAttached())
		for (int i = 0; i < Bodies.GetCount(); i++) Bodies[i]->OnStepBefore();
}
//---------------------------------------------------------------------

void CComposite::OnStepAfter()
{
	if (IsAttached())
	{
		for (int i = 0; i < Bodies.GetCount(); i++) Bodies[i]->OnStepAfter();

		// update stored transform
		if (Bodies.GetCount() > 0)
		{
			CRigidBody* pMaster = Bodies[0];
			Transform = pMaster->GetInvInitialTransform() * pMaster->GetTransform();
		}
	}
}
//---------------------------------------------------------------------

void CComposite::OnFrameBefore()
{
	if (IsAttached())
	{
		FrameBeforeTfm = Transform;
		Flags.Clear(PHYS_COMP_TFM_CHANGED);
		for (int i = 0; i < Bodies.GetCount(); i++) Bodies[i]->OnFrameBefore();
	}
}
//---------------------------------------------------------------------

void CComposite::OnFrameAfter()
{
	if (IsAttached())
	{
		for (int i = 0; i < Bodies.GetCount(); i++)
		{
			CRigidBody* pBody = Bodies[i];
			pBody->OnFrameAfter();
			// if any rigid pBody is enabled, we set the transform changed flag
			if (pBody->IsEnabled()) Flags.Set(PHYS_COMP_TFM_CHANGED);
		}

		// check if transform has changed by other means
		if (!Flags.Is(PHYS_COMP_TFM_CHANGED))
		{
			if (Flags.Is(PHYS_COMP_TFM_WAS_SET) ||
				(!FrameBeforeTfm.AxisX().isequal(Transform.AxisX(), 0.001f)) ||
				(!FrameBeforeTfm.AxisY().isequal(Transform.AxisY(), 0.001f)) ||
				(!FrameBeforeTfm.AxisZ().isequal(Transform.AxisZ(), 0.001f)) ||
				(!FrameBeforeTfm.Translation().isequal(Transform.Translation(), 0.001f)))
			{
				Flags.Set(PHYS_COMP_TFM_CHANGED);
			}
		}
	}
	Flags.Clear(PHYS_COMP_TFM_WAS_SET);
}
//---------------------------------------------------------------------

int CComposite::GetNumCollisions() const
{
	int Result = 0;
	for (int i = 0; i < Bodies.GetCount(); i++)
		Result += Bodies[i]->GetNumCollisions();
	return Result;
}
//---------------------------------------------------------------------

bool CComposite::IsHorizontalCollided() const
{
	for (int i = 0; i < Bodies.GetCount(); i++)
		if (Bodies[i]->IsHorizontalCollided()) OK;
	FAIL;
}
//---------------------------------------------------------------------

void CComposite::GetAABB(bbox3& AABB) const
{
	int ShapeIdx;
	if (Bodies.GetCount() > 0)
	{
		ShapeIdx = 0;
		Bodies[0]->GetAABB(AABB);
		//???transform?
		for (int i = 1; i < Bodies.GetCount(); i++)
		{
			bbox3 NextAABB;
			Bodies[i]->GetAABB(NextAABB);
			//???transform?
			AABB.extend(NextAABB);
		}
	}
	else if (Shapes.GetCount() > 0)
	{
		ShapeIdx = 1;
		Shapes[0]->GetAABB(AABB);
		//???transform?
	}
	else
	{
		AABB.vmin.x = 
		AABB.vmin.y = 
		AABB.vmin.z = 
		AABB.vmax.x = 
		AABB.vmax.y = 
		AABB.vmax.z = 0.f;
		return;
	}

	for (; ShapeIdx < Shapes.GetCount(); ShapeIdx++)
	{
		bbox3 NextAABB;
		Shapes[ShapeIdx]->GetAABB(NextAABB);
		//???transform?
		AABB.extend(NextAABB);
	}
}
//---------------------------------------------------------------------

void CComposite::SetTransform(const matrix44& Tfm)
{
	Transform = Tfm;
	Flags.Set(PHYS_COMP_TFM_WAS_SET);

	if (IsAttached())
	{
		for (int i = 0; i < Bodies.GetCount(); i++)
		{
			CRigidBody* pBody = Bodies[i];
			pBody->SetTransform(pBody->GetInitialTransform() * Tfm);
		}

		for (int i = 0; i < Joints.GetCount(); i++) Joints[i]->UpdateTransform(Tfm);

		for (int i = 0; i < Shapes.GetCount(); i++)
		{
			CShape* pShape = Shapes[i];
			pShape->SetTransform(pShape->GetInitialTransform() * Tfm);
		}
	}
}
//---------------------------------------------------------------------

void CComposite::RenderDebug()
{
	for (int i = 0; i < Bodies.GetCount(); i++) Bodies[i]->RenderDebug();
	for (int i = 0; i < Joints.GetCount(); i++) Joints[i]->RenderDebug();
	for (int i = 0; i < Shapes.GetCount(); i++) Shapes[i]->RenderDebug(matrix44::identity);
}
//---------------------------------------------------------------------

CRigidBody* CComposite::FindBodyByUniqueID(int ID) const
{
	n_assert(ID > 0); //!!!to check if it never happens normally!
	//if (ID)
	for (int i = 0; i < Bodies.GetCount(); i++)
	{
		CRigidBody* pBody = Bodies[i];
		if (ID == pBody->GetUID()) return pBody;
	}
	return NULL;
}
//---------------------------------------------------------------------

// Returns true if any of the contained rigid bodies has the
// provided link type. This method currently iterates
// through the rigid bodies (and thus may be slow).
bool CComposite::HasLinkType(CRigidBody::ELinkType Type)
{
	for (int i = 0; i < Bodies.GetCount(); i++)
		if (Bodies[i]->IsLinkValid(Type)) OK;
	FAIL;
}
//---------------------------------------------------------------------

//NOTE: this will not increment the refcount of the pEntity to avoid a cyclic reference.
void CComposite::SetEntity(CEntity* pEnt)
{
	n_assert(pEnt);
	pEntity = pEnt;
	for (int i = 0; i < Bodies.GetCount(); i++) Bodies[i]->SetEntity(pEntity);
	for (int i = 0; i < Shapes.GetCount(); i++) Shapes[i]->SetEntity(pEntity);
}
//---------------------------------------------------------------------

} // namespace Physics
