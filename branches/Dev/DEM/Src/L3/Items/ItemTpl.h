#pragma once
#ifndef __IPG_ITEM_TPL_H__
#define __IPG_ITEM_TPL_H__

#include <Core/RefCounted.h>
#include <Data/StringID.h>

// Item template is a factory for creation of item instances. It is a "class" of concrete items like
// "Wooden arrow" or "Plate Mail +1". This class creates template item instance which is a blueprint for
// item instances creation. If item instance is intact (R/O), template instance pointer can be used
// for it. This can reduce memory footprint.

namespace Data
{
	class CParams;
}

namespace Items
{

using namespace Data;

class CItem;

class CItemTpl: public Core::CRefCounted
{
	__DeclareClass(CItemTpl);

protected:

	CStrID		ID;
	CStrID		Type;
	Ptr<CItem>	TemplateItem;	// If in derived Tpl item has no per-instance params, Tpl can
								// use CItem class itself to instantiate its TemplateItem.

public:

	// Here are stored item params that can't change
	// Type-specific params are defined in derived classes
	float			Weight; //???can it change per instance?
	float			Volume; //???can it change per instance?
	nString			UIName;

	// When derive and need TemplateItem of specific type, call parent ::Init only after TemplateItem creation
	virtual void	Init(CStrID SID, const CParams& Params);

	Ptr<CItem>		CreateNewItem() const; //!!!can call GetTemplateItem()->Clone() directly too!
	//const CItem*	GetTemplateItem() const { return TemplateItem; }
	CItem*			GetTemplateItem() const { return TemplateItem; } // Cause we want to assign it to nonconst item ptrs
	CStrID			GetID() const { return ID; }
	CStrID			GetType() const { return Type; }
};

typedef Ptr<CItemTpl> PItemTpl;

}

#endif