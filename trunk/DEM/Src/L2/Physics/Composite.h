#pragma once
#ifndef __DEM_L2_PHYSICS_COMPOSITE_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_COMPOSITE_H__

#include <Physics/RigidBody.h>
#include <Physics/Joints/Joint.h>
#include <Data/Flags.h>

//A Physics::CComposite contains one or several rigid bodies, connected
//by joints. There is one master pBody defined in the pool which receives
//and delivers positions and forces from/to the outside world. Composites
//are usually constructed once and not altered (i.e. you cannot remove
//bodies from the composite). The master pBody will always be the pBody at index
//0 (the first pBody added to the group).
//Composites may also contain optional static shapes. These are pure collide
//shapes which will move with the composite but will not act physically.

namespace Physics
{
using namespace Data;

class CJoint;
class CEntity;

class CComposite: public Core::CRefCounted
{
	DeclareRTTI;
	DeclareFactory(CComposite);

protected:

	CFlags					Flags;
	matrix44				FrameBeforeTfm;
	matrix44				Transform;
	nFixedArray<PRigidBody>	Bodies;
	nFixedArray<PJoint>		Joints;
	nFixedArray<PShape>		Shapes;
	int						CurrBodyIdx;	//???need both 3 and all the time?
	int						CurrJointIdx;
	int						CurrShapeIdx;
	CEntity*				pEntity;
	dSpaceID				ODESpaceID;

	void ClearBodies() { Bodies.Clear(NULL); }
	void ClearJoints() { Joints.Clear(NULL); }
	void ClearShapes() { Shapes.Clear(NULL); }

public:

	enum
	{
		PHYS_COMP_BEGIN_BODIES	= 0x01,
		PHYS_COMP_BEGIN_JOINTS	= 0x02,
		PHYS_COMP_BEGIN_SHAPES	= 0x04,
		PHYS_COMP_ATTACHED		= 0x08,
		PHYS_COMP_TFM_CHANGED	= 0x10,
		PHYS_COMP_TFM_WAS_SET	= 0x20
	};

	CComposite();
	virtual ~CComposite();

	virtual void	Attach(dWorldID WorldID, dSpaceID DynamicSpaceID, dSpaceID StaticSpaceID);
	virtual void	Detach();
	void			Reset();
	void			RenderDebug();
	void			OnStepBefore();
	void			OnStepAfter();
	void			OnFrameBefore();
	void			OnFrameAfter();

	virtual void	BeginBodies(int Count);
	virtual void	AddBody(CRigidBody* pBody);
	virtual void	EndBodies();
	int				GetNumBodies() const { return Bodies.Size(); }
	CRigidBody*		GetBodyAt(int Idx) const { return Bodies[Idx]; }
	CRigidBody*		FindBodyByUniqueID(int ID) const;
	bool			HasBodyWithName(const nString& Name) const;
	CRigidBody*		GetBodyByName(const nString& Name) const;
	CRigidBody*		GetMasterBody() const { if (Bodies.Size() > 0) return Bodies[0]; else return NULL; }

	virtual void	BeginJoints(int Count);
	virtual void	AddJoint(CJoint* pJoint);
	virtual void	EndJoints();
	int				GetNumJoints() const { return Joints.Size(); }
	CJoint*			GetJointAt(int Idx) const { return Joints[Idx]; }

	virtual void	BeginShapes(int Count);
	virtual void	AddShape(CShape* pShape);
	virtual void	EndShapes();
	int				GetNumShapes() const { return Shapes.Size(); }
	CShape*			GetShapeAt(int Idx) const { return Shapes[Idx]; }

	void			GetAABB(bbox3& AABB) const;
	void			SetTransform(const matrix44& Tfm);
	const matrix44&	GetTransform() const { return Transform; }
	bool			HasTransformChanged() const { return Flags.Is(PHYS_COMP_TFM_CHANGED); }
	int				GetNumCollisions() const;
	void			SetEntity(CEntity* pEnt);
	CEntity*		GetEntity() const { return pEntity; }
	void			SetEnabled(bool Enabled);
	bool			IsEnabled() const { return Bodies.Size() < 1 || Bodies[0]->IsEnabled(); }
	bool			IsAttached() const { return Flags.Is(PHYS_COMP_ATTACHED); }
	bool			IsHorizontalCollided() const;
	bool			HasLinkType(CRigidBody::ELinkType Type);
};
//---------------------------------------------------------------------

RegisterFactory(CComposite);

typedef Ptr<CComposite> PComposite;

inline bool CComposite::HasBodyWithName(const nString& Name) const
{
	//???or store into CStrID/nString nDict & store Key of master body?
	for (int i = 0; i < Bodies.Size(); i++)
		if (Name == Bodies[i]->Name) OK;
	FAIL;
}
//---------------------------------------------------------------------

inline CRigidBody* CComposite::GetBodyByName(const nString& Name) const
{
	for (int i = 0; i < Bodies.Size(); i++)
		if (Name == Bodies[i]->Name) return Bodies[i];
	//n_assert(false);
	return NULL;
}
//---------------------------------------------------------------------

// Enable/disable the composite. The enabled state is simply distributed
// to all rigid bodies in the composite. Disabled bodies will reenable
// themselves automatically on contact with other enabled bodies.
inline void CComposite::SetEnabled(bool Enabled)
{
	for (int i = 0; i < Bodies.Size(); i++) Bodies[i]->SetEnabled(Enabled);
}
//---------------------------------------------------------------------

inline void CComposite::Reset()
{
	for (int i = 0; i < Bodies.Size(); i++) Bodies[i]->Reset();
}
//---------------------------------------------------------------------

}

#endif
