#pragma once
#ifndef __DEM_L2_GAME_ENTITY_H__
#define __DEM_L2_GAME_ENTITY_H__

#include <Events/EventDispatcher.h>
#include <Data/Flags.h>
#include <Data/Dictionary.h>

// Entity is an abstract game object containing properties which extend it

namespace Game
{
typedef Ptr<class CGameLevel> PGameLevel;

class CEntity: public Events::CEventDispatcher
{
	__DeclareClassNoFactory;

protected:

	friend class CEntityManager;

	enum
	{
		Active				= 0x01,
		ChangingActivity	= 0x02,	// If set: Active set - in Deactivate(), Active not set - in Activate()
		DeleteMe			= 0x04	// Set when need to ddelete entity from inside its method. EntityManager will delete it later.
	};

	CStrID			UID;
	PGameLevel		Level;
	Data::CFlags	Flags;
	Events::PSub	LevelSub;
	CDataDict		Attrs;		//???use hash map?

	void SetUID(CStrID NewUID);
	bool OnEvent(const Events::CEventBase& Event);

	CEntity(CStrID _UID);

public:

	~CEntity();

	void						SetLevel(CGameLevel* pNewLevel);
	void						Activate();
	void						Deactivate();
	void						Save(Data::CParams& OutDesc, const Data::CParams* pInitialDesc = NULL);

	template<class T> T*		GetProperty() const;
	template<class T> bool		HasProperty() const { return GetProperty<T>() != NULL; }

	CStrID						GetUID() const { n_assert_dbg(UID.IsValid()); return UID; }
	CGameLevel*					GetLevel() const { return Level.GetUnsafe(); }

	//???!!!need GetAttr with default?!
	void						BeginNewAttrs(DWORD Count) { Attrs.BeginAdd(Count); }
	template<class T> void		AddNewAttr(CStrID ID, const T& Value) { Attrs.Add(ID, Value); }
	void						EndNewAttrs() { Attrs.EndAdd(); }
	bool						DeleteAttr(CStrID ID) { return Attrs.Remove(ID); }
	template<class T> void		SetAttr(CStrID ID, const T& Value);
	template<> void				SetAttr(CStrID ID, const Data::CData& Value);
	template<class T> const T&	GetAttr(CStrID ID) const { return Attrs[ID].GetValue<T>(); }
	template<class T> const T&	GetAttr(CStrID ID, const T& Default) const;
	template<class T> bool		GetAttr(T& Out, CStrID ID) const;
	template<> bool				GetAttr(Data::CData& Out, CStrID ID) const;
	bool						HasAttr(CStrID ID) const { return Attrs.FindIndex(ID) != INVALID_INDEX; }

	bool						IsActive() const { return Flags.Is(Active) && Flags.IsNot(ChangingActivity); }
	bool						IsInactive() const { return Flags.IsNot(Active) && Flags.IsNot(ChangingActivity); }
	bool						IsActivating() const { return Flags.IsNot(Active) && Flags.Is(ChangingActivity); }
	bool						IsDeactivating() const { return Flags.Is(Active) && Flags.Is(ChangingActivity); }
	void						RequestDestruction() { Flags.Set(DeleteMe); }
	bool						IsWaitingForDestruction() const { return Flags.Is(DeleteMe); }
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
	if (Idx == INVALID_INDEX) Attrs.Add(ID, Value);
	else Attrs.ValueAt(Idx).SetTypeValue(Value);
}
//---------------------------------------------------------------------

template<> void CEntity::SetAttr(CStrID ID, const Data::CData& Value)
{
	if (Value.IsValid()) Attrs.Set(ID, Value);
	else DeleteAttr(ID);
}
//---------------------------------------------------------------------

template<class T>
inline const T& CEntity::GetAttr(CStrID ID, const T& Default) const
{
	int Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) return Default;
	return Attrs.ValueAt(Idx).GetValue<T>();
}
//---------------------------------------------------------------------

//???ref of ptr? to avoid copying big data
template<class T>
inline bool CEntity::GetAttr(T& Out, CStrID ID) const
{
	int Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;
	return Attrs.ValueAt(Idx).GetValue<T>(Out);
}
//---------------------------------------------------------------------

//???ref of ptr? to avoid copying big data
template<>
inline bool CEntity::GetAttr(Data::CData& Out, CStrID ID) const
{
	int Idx = Attrs.FindIndex(ID);
	if (Idx == INVALID_INDEX) FAIL;
	Out = Attrs.ValueAt(Idx);
	OK;
}
//---------------------------------------------------------------------

}

#endif
