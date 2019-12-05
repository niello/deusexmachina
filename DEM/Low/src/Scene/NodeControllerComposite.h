#pragma once
#ifndef __DEM_L1_SCENE_NODE_CTLR_COMPOSITE_H__
#define __DEM_L1_SCENE_NODE_CTLR_COMPOSITE_H__

#include <Scene/NodeController.h>
#include <Data/Array.h>

// Allows to select or blend output of multiple source controllers. Control values are now restricted
// to Weight and Priority, which is sufficient for priority blending (W + P) as well as simple weighted
// blending (W) or priority-based selection (P).

namespace Scene
{

class CNodeControllerComposite: public CNodeController
{
	RTTI_CLASS_DECL;

protected:

	struct CSource
	{
		PNodeController		Ctlr;
		UPTR				Priority;
		float				Weight;
		Math::CTransformSRT	SRT;	//???!!!revisit?! Store only because Ctlr.ApplyTo may return false for unchanged tfm, which must be cached

		bool operator <(const CSource& Other) const { return Priority > Other.Priority; }
		bool operator >(const CSource& Other) const { return Priority < Other.Priority; }
		bool operator ==(const CSource& Other) const { return Priority == Other.Priority; }
	};

	// Sorted by priority. Search by controller is linear, but there are 2-3 sources in most cases.
	CArray<CSource> Sources;

public:

	virtual bool	OnAttachToNode(Scene::CSceneNode* pSceneNode);
	virtual void	OnDetachFromScene();

	bool			AddSource(Scene::CNodeController& Ctlr, UPTR Priority, float Weight);
	bool			RemoveSource(const Scene::CNodeController& Ctlr);
	void			Clear();

	UPTR			GetSourceCount() const { return Sources.GetCount(); }
	IPTR			GetSourceIndex(Scene::CNodeController& Ctlr) const;
	void			SetPriority(int CtlrIdx, UPTR Priority) { Sources[CtlrIdx].Priority = Priority; Sources.Sort(); }
	void			SetWeight(int CtlrIdx, float Weight) { n_assert(Weight >= 0.f && Weight <= 1.f); Sources[CtlrIdx].Weight = Weight; }
};

typedef Ptr<CNodeControllerComposite> PNodeControllerComposite;

inline IPTR CNodeControllerComposite::GetSourceIndex(Scene::CNodeController& Ctlr) const
{
	for (UPTR i = 0; i < Sources.GetCount(); ++i)
		if (Sources[i].Ctlr.Get() == &Ctlr) return i;
	return INVALID_INDEX;
}
//---------------------------------------------------------------------

}

#endif
