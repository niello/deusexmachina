#pragma once
#ifndef __DEM_L3_ITEM_H__
#define __DEM_L3_ITEM_H__

#include "ItemTpl.h"

// Item class is an abstract game item instance. If it differs from template instance, it stores difference
// inside itself, else item just refers to the template instance.

namespace Items
{

class CItem: public Core::CRefCounted
{
protected:

	CItemTpl* Template;

	// Should be called from Clone of top-level class and from CopyFields of parent
	// classes down to CItem itself.
	// No need of this function here cause CItem has no per-instance fields to clone now
	//void CopyFields(CItem& Dest);

public:

	CItem(CItemTpl* Tpl): Template(Tpl) {}
	CItem(const CItem& TplInstance): Template(TplInstance.Template) {}

	// Here are stored item params that can change
	// Type-specific params are defined in derived classes
	// Assert !IsTemplateInstance at least in debug when get/set per-instance values

	virtual Ptr<CItem>	Clone() const;

	virtual bool		IsEqual(const CItem* pOther) const { return GetID() == pOther->GetID(); }
	bool				IsTemplateInstance() const { return this == Template->GetTemplateItem(); }
	CStrID				GetID() const { return Template->GetID(); }
	PItemTpl			GetTpl() const { return Template; }
};

typedef Ptr<CItem> PItem;

}

#endif