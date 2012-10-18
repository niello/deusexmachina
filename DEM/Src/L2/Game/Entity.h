#pragma once
#ifndef __DEM_L2_GAME_ENTITY_H__
#define __DEM_L2_GAME_ENTITY_H__

// Entity is an abstract game object containing properties which extend it
// Based on mangalore Entity (C) 2003 RadonLabs GmbH

#include "EntityFwd.h"
#include <DB/ValueTable.h>
#include <Events/EventDispatcher.h>

namespace Loading
{
	class CEntityFactory;
}

namespace Game
{
class CProperty;

using namespace DB;
using namespace Events;

class CEntity: public Events::CEventDispatcher
{
	__DeclareClass(CEntity);

protected:

	friend class Loading::CEntityFactory;

	enum
	{
		ENT_LIVE = 0x01,			// true - Live pool, false - Sleeping pool
		ENT_ACTIVE = 0x02,			// true - Active, false - Inactive
		ENT_CHANGING_STATUS = 0x04, // if true: ENT_ACTIVE true - Deactivate, ENT_ACTIVE false - Activate
	};

	CStrID			UID;
	CStrID			Category;
	char			Flags;		// entity status //???align to DWORD?
	Events::PSub	GlobalSubscription;
	DB::PValueTable	AttrTable;
	int				ATRowIdx;

	//!!!instead of IsLive() check can unsubscribe when not alive in SetLive!
	bool OnEvent(const CEventBase& Event);
	
	//???private?
	void SetCategory(CStrID Cat) { n_assert(Cat.IsValid()); Category = Cat; }
	void SetAttrTableRowIndex(int Idx) { ATRowIdx = Idx; }
	void SetAttrTable(const DB::PValueTable& Table) { AttrTable = Table; } //???const Ptr&? O_o
	void SetUniqueIDFromAttrTable();

public:

	CEntity();
	virtual ~CEntity();

	void			Activate();
	void			Deactivate();

	template<class T> T*	FindProperty() const;
	template<class T> bool	HasProperty() const { return FindProperty<T>() != NULL; }

	void			SetUniqueID(CStrID NewUID);
	CStrID			GetUniqueID() const { n_assert(UID.IsValid()); return UID; } //!!!DBG assert!

	// Status getters
	//!!!check it!
	//???what's faster? (x & (a | b)) == (a | b) OR (x & a) && (x & b)
	bool			IsLive() const { return Flags & ENT_LIVE; }
	void			SetLive(bool Live) { if (Live != IsLive()) Flags ^= ENT_LIVE; }
	bool			IsActive() const { return (Flags & ENT_ACTIVE) && !(Flags & ENT_CHANGING_STATUS); }
	bool			IsInactive() const { return !(Flags & ENT_ACTIVE) && !(Flags & ENT_CHANGING_STATUS); }
	bool			IsActivating() const { return !(Flags & ENT_ACTIVE) && (Flags & ENT_CHANGING_STATUS); }
	bool			IsDeactivating() const { return (Flags & ENT_ACTIVE) && (Flags & ENT_CHANGING_STATUS); }
	EntityPool		GetPool() const { return IsLive() ? LivePool : SleepingPool;}

	CStrID			GetCategory() const { return Category; }
	int				GetAttrTableRowIndex() const { return ATRowIdx; }
	const DB::PValueTable& GetAttrTable() const { return AttrTable; }

	bool			HasAttr(CAttrID Attr) const { return AttrTable.isvalid() ? AttrTable->HasColumn(Attr) : false; }

	template<class T>
	void			Add(CAttrID AttrID);
	template<class T>
	void			Set(CAttrID AttrID, const T& Value) { AttrTable->Set<T>(AttrID, ATRowIdx, Value); }
	template<class T>
	const T&		Get(CAttrID AttrID) const { return AttrTable->Get<T>(AttrID, ATRowIdx); }
	template<class T>
	bool			Get(CAttrID AttrID, T& Out) const; //???ref of ptr? to avoid copying big data

	bool			Get(CAttrID AttrID, CData& Out) const;
	void			SetRaw(CAttrID AttrID, const CData& Value) const { AttrTable->SetValue(AttrID, ATRowIdx, Value); }
};

RegisterFactory(CEntity);

typedef Ptr<CEntity> PEntity;

template<class T> T* CEntity::FindProperty() const
{
	PProperty Prop;
	if (T::Storage.Get(UID, Prop))
		if (!Prop->IsA(T::RTTI)) return NULL;
	return (T*)Prop.get_unsafe();
}
//---------------------------------------------------------------------

template<class T> inline bool CEntity::Get(CAttrID AttrID, T& Out) const
{
	if (!HasAttr(AttrID)) FAIL;
	Out = Get<T>(AttrID);
	OK;
}
//---------------------------------------------------------------------

inline bool CEntity::Get(CAttrID AttrID, CData& Out) const
{
	if (!HasAttr(AttrID)) FAIL;
	AttrTable->GetValue(AttrID, ATRowIdx, Out); //???return bool here?
	OK;
}
//---------------------------------------------------------------------

template<> inline bool CEntity::Get(CAttrID AttrID, vector3& Out) const
{
	if (!HasAttr(AttrID)) FAIL;
	const vector4& Tmp = AttrTable->Get<vector4>(AttrID, ATRowIdx);
	Out = vector3(Tmp.x, Tmp.y, Tmp.z);
	OK;
}
//---------------------------------------------------------------------

template<> inline void CEntity::Set(CAttrID AttrID, const vector3& Val)
{
	AttrTable->Set<vector4>(AttrID, ATRowIdx, vector4(Val));
}
//---------------------------------------------------------------------

}

#endif
