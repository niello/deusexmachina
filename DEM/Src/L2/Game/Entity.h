#pragma once
#ifndef __DEM_L2_GAME_ENTITY_H__
#define __DEM_L2_GAME_ENTITY_H__

// Entity is an abstract game object containing properties which extend it

#include <Events/EventDispatcher.h>
#include <Data/Flags.h>
#include <util/ndictionary.h>

namespace Game
{

class CEntity: public Events::CEventDispatcher
{
	__DeclareClassNoFactory;

protected:

	friend class CEntityManager;

	enum
	{
		Active				= 0x01,
		ChangingActivity	= 0x02	// If set: Active set - in Deactivate(), Active not set - in Activate()
	};

	CStrID								UID;
	CStrID								LevelID;
	Data::CFlags						Flags;
	Events::PSub						GlobalSub;
	nDictionary<CStrID, Data::CData>	Attrs;		//???use hash map?

	void SetUID(CStrID NewUID);
	//void SetLevelID(CStrID NewLevelID);
	bool OnEvent(const Events::CEventBase& Event);

	CEntity(CStrID _UID, CStrID _LevelID): CEventDispatcher(16), UID(_UID), LevelID(_LevelID) {}

public:

	void						Activate();
	void						Deactivate();

	template<class T> T*		GetProperty() const;
	template<class T> bool		HasProperty() const { return GetProperty<T>() != NULL; }

	CStrID						GetUID() const { n_assert_dbg(UID.IsValid()); return UID; }
	CStrID						GetLevelID() const { return LevelID; }

	template<class T> void		SetAttr(CStrID ID, const T& Value);
	template<> void				SetAttr(CStrID ID, const Data::CData& Value);
	template<class T> const T&	GetAttr(CStrID ID) const { return Attrs[ID].GetValue<T>(); }
	template<class T> bool		GetAttr(CStrID ID, T& Out) const;
	template<> bool				GetAttr(CStrID ID, Data::CData& Out) const;
	bool						HasAttr(CStrID ID) const { return Attrs.FindIndex(ID) != INVALID_INDEX; }

	bool						IsActive() const { return Flags.Is(Active) && Flags.IsNot(ChangingActivity); }
	bool						IsInactive() const { return Flags.IsNot(Active) && Flags.IsNot(ChangingActivity); }
	bool						IsActivating() const { return Flags.IsNot(Active) && Flags.Is(ChangingActivity); }
	bool						IsDeactivating() const { return Flags.Is(Active) && Flags.Is(ChangingActivity); }
};

typedef Ptr<CEntity> PEntity;

template<class T>
T* CEntity::GetProperty() const
{
	PProperty Prop;
	if (T::pStorage && T::pStorage->Get(UID, Prop))
		if (!Prop->IsA(T::RTTI)) return NULL;
	return (T*)Prop.GetUnsafe();
}
//---------------------------------------------------------------------

template<class T>
inline void CEntity::SetAttr(CStrID ID, const T& Value)
{
	int Idx = Attrs.FindIndex(ID);
	if (ID == INVALID_INDEX) Attrs.Add(ID, Value);
	else Attrs.ValueAtIndex(Idx).SetTypeValue(Value);
}
//---------------------------------------------------------------------

template<> void CEntity::SetAttr(CStrID ID, const Data::CData& Value)
{
	if (Value.IsValid()) Attrs.Set(ID, Value);
	else
	{
		int Idx = Attrs.FindIndex(ID);
		if (ID != INVALID_INDEX) Attrs.Erase(ID);
	}
}
//---------------------------------------------------------------------

//???ref of ptr? to avoid copying big data
template<class T>
inline bool CEntity::GetAttr(CStrID ID, T& Out) const
{
	int Idx = Attrs.FindIndex(ID);
	if (ID == INVALID_INDEX) FAIL;
	return Attrs.ValueAtIndex(Idx).GetValue<T>(Out);
}
//---------------------------------------------------------------------

//???ref of ptr? to avoid copying big data
template<>
inline bool CEntity::GetAttr(CStrID ID, Data::CData& Out) const
{
	int Idx = Attrs.FindIndex(ID);
	if (ID == INVALID_INDEX) FAIL;
	Out = Attrs.ValueAtIndex(Idx);
	OK;
}
//---------------------------------------------------------------------

}

#endif
