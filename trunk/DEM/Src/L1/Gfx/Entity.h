#pragma once
#ifndef __DEM_L2_GFX_ENTITY_H__
#define __DEM_L2_GFX_ENTITY_H__

#include <Core/RefCounted.h>
#include <Data/Flags.h>
#include <Data/StringID.h>
#include <Data/QuadTree.h>
#include <Data/LinkedListSet.h>

// Graphics entity is a part of scene, either camera, light or some mesh

namespace Time
{
	typedef Ptr<class CTimeSource> PTimeSource;
}

namespace Graphics
{
typedef Ptr<class CEntity> PEntity;

enum EEntityType
{
	GFXShape = 0,
	GFXLight,
	GFXCamera,
	GFXEntityNumTypes
};

enum ELinkType
{
	CameraLink = 0,
	LightLink,
	PickupLink,
	NumLinkTypes
};

typedef Data::CLinkedListSet<EEntityType, PEntity> CGfxEntityListSet;

struct CGfxQTNode
{
	typedef CGfxEntityListSet::CElement CElement;

	CGfxEntityListSet	Entities; //???or inheritance?
	vector4				DebugColor;
	//int				EntityCountByType[CEntity::NumTypes]; //!!!in hierarchy!

	//for (int i = 0; i < CEntity::NumTypes; i++)
		//EntityCountByType[i] = 0;
	CGfxQTNode() { DebugColor.set(n_rand(), n_rand(), n_rand(), 0.3f); }
	~CGfxQTNode();

	CElement*	Add(const PEntity& Object) { return Entities.Add(Object); } //UpdateNumEntitiesInHierarchy(pEntity->GetType(), 1);
	bool		Remove(const PEntity& Object);
	void		Remove(CElement* pElement);
};

typedef Data::CQuadTree<class CEntity*, CGfxQTNode> CGfxQT;
typedef CGfxQT::CElement CGfxEntityNode;

class CEntity: public Core::CRefCounted
{
	DeclareRTTI;

protected:

	typedef nFixedArray<nArray<PEntity>> CLinkArray;

	enum
	{
		ACTIVE				= 0x01,
		VISIBLE				= 0x02,
		GLOBAL_BOX_DIRTY	= 0x04
	};

	friend class CLevel;

	Data::CFlags		Flags;
	CLevel*				pLevel;
	CGfxQT::CNode*		pQTNode;
	bbox3				LocalBBox;
	bbox3				GlobalBBox;			// BB in world space

	matrix44			Transform;
	CLinkArray			Links;

	nTime				ActivationTime;

	CStrID				UserData;

	virtual void		UpdateGlobalBox();

public:

	float				TimeFactor;
	Time::PTimeSource	ExtTimeSrc; //???set-only?

	//???does camera need this?
	float				MaxVisibleDistance;
	float				MinVisibleSize;

	CEntity();
	virtual ~CEntity();

	virtual void		Activate();
	virtual void		Deactivate();
	virtual void		RenderDebug();
	bool				TestLODVisibility(); //???does camera need this?

	virtual EEntityType	GetType() const = 0;
	CLevel*				GetLevel() const { return pLevel; }
	virtual void		SetTransform(const matrix44& Tfm);
	const matrix44&		GetTransform() const { return Transform; }
	virtual EClipStatus	GetBoxClipStatus(const bbox3& Box);
	void				ResetActivationTime();
	nTime				GetActivationTime() const { return ActivationTime; }
	nTime				GetEntityTime() const;

	void				SetLocalBox(const bbox3& Box);
	const bbox3&		GetLocalBox() const { return LocalBBox; }
	const bbox3&		GetBox();
	
	// For CLinkedListSet
	EEntityType			GetKey() const { return GetType(); }

	// For CQuadTree
	void				GetCenter(vector2& Out) const { Out.x = Transform.pos_component().x; Out.y = Transform.pos_component().z; }
	void				GetHalfSize(vector2& Out);
	CGfxQT::CNode*		GetQuadTreeNode() const { return pQTNode; }
	void				SetQuadTreeNode(CGfxQT::CNode* pNode) { pQTNode = pNode; }

	void				ClearLinks(ELinkType LinkType);
	void				AddLink(ELinkType LinkType, CEntity* pEntity);
	int					GetNumLinks(ELinkType LinkType) const { return Links[LinkType].Size(); }
	CEntity*			GetLinkAt(ELinkType LinkType, int Idx) const { return Links[LinkType][Idx]; }
	void				SetUserData(CStrID Data) { UserData = Data; }
	CStrID				GetUserData() const { return UserData; }
};
//---------------------------------------------------------------------

typedef Ptr<CEntity> PEntity;

inline const bbox3& CEntity::GetBox()
{
	if (Flags.Is(GLOBAL_BOX_DIRTY)) UpdateGlobalBox();
	return GlobalBBox;
}
//---------------------------------------------------------------------

inline void CEntity::SetLocalBox(const bbox3& Box)
{
	LocalBBox = Box;
	Flags.Set(GLOBAL_BOX_DIRTY);
}
//---------------------------------------------------------------------

inline void CEntity::AddLink(ELinkType LinkType, CEntity* pEntity)
{
	n_assert(pEntity);
	Links[LinkType].Append(pEntity);
}
//---------------------------------------------------------------------

// Clear the links array. This will decrement the refcounts of all linked entities.
// FIXME: hmm, this may be performance problem, since all contained smart
// pointer will calls their destructors. But it's definitely safer that way.
inline void CEntity::ClearLinks(ELinkType LinkType)
{
	Links[LinkType].Clear();
}
//---------------------------------------------------------------------

inline void CEntity::GetHalfSize(vector2& Out)
{
	const bbox3& Box = GetBox();
	Out.x = (Box.vmax.x - Box.vmin.x) * 0.5f;
	Out.y = (Box.vmax.z - Box.vmin.z) * 0.5f;
}
//---------------------------------------------------------------------

}

#endif
