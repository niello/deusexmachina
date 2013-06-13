#pragma once
#ifndef __DEM_L1_ANIM_CTLR_PRIORITY_BLEND_H__
#define __DEM_L1_ANIM_CTLR_PRIORITY_BLEND_H__

#include <Scene/NodeController.h>

// Priority-blend controller blends inputs from a set of another controllers from the most
// to the least priority according to input weights until weight sum is 1.0f

namespace Anim
{

class CNodeControllerPriorityBlend: public Scene::CNodeController
{
protected:

	struct CSource
	{
		Scene::PNodeController	Ctlr;
		DWORD					Priority;
		float					Weight;
		Math::CTransformSRT		SRT;	// Store only because Ctlr.ApplyTo may return false for unchanged tfm, which must be cached

		bool operator <(const CSource& Other) const { return Priority < Other.Priority; }
		bool operator >(const CSource& Other) const { return Priority > Other.Priority; }
	};

	nArray<CSource> Sources;

public:

	bool			AddSource(Scene::CNodeController& Ctlr, DWORD Priority, float Weight);
	void			RemoveSource(Scene::CNodeController& Ctlr);
	void			Clear();
	void			SetPriority(Scene::CNodeController& Ctlr, DWORD Priority);
	void			SetWeight(Scene::CNodeController& Ctlr, float Weight);

	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm);
};

typedef Ptr<CNodeControllerPriorityBlend> PNodeControllerPriorityBlend;

}

#endif
