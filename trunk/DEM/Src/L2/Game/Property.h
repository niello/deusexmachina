#pragma once
#ifndef __DEM_L2_GAME_PROPERTY_H__
#define __DEM_L2_GAME_PROPERTY_H__

// Properties are attached to game entities to add specific functionality or behaviors to the entity.
// Based on mangalore Property_(C) 2005 Radon Labs GmbH

#include <Core/RefCounted.h>
#include <util/HashTable.h>
#include <Events/Events.h>
#include <Game/EntityFwd.h>

#define DeclarePropertyPools(ActivePools) \
	public: \
		static const int Pools = (ActivePools); \
	private:
#define DeclarePropertyStorage \
	public: \
		static Game::CPropertyStorage Storage; \
	private:
#define ImplementPropertyStorage(Class, MapCapacity) \
	Game::CPropertyStorage Class::Storage(MapCapacity);
#define RegisterProperty(Class) \
	bool PropertyRegistered_##Class = EntityFct->RegisterPropertyMeta(Class::RTTI, Class::Storage, Class::Pools);

#define PROP_SUBSCRIBE_NEVENT(EventName, Class, Handler) \
	Sub_##EventName = GetEntity()->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler)
#define PROP_SUBSCRIBE_PEVENT(EventName, Class, Handler) \
	Sub_##EventName = GetEntity()->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler)

namespace Loading
{
	class CEntityFactory;
}

namespace DB
{
	class CAttributeID;
	typedef const CAttributeID* CAttrID;
}

namespace Game
{
using namespace Events;

class CProperty: public Core::CRefCounted
{
	DeclareRTTI;
	DeclarePropertyPools(LivePool | SleepingPool);

protected:

	friend class CEntity;
	friend class Loading::CEntityFactory;

	CEntity*	pEntity;
	bool		Active;

	void SetEntity(CEntity* pEnt);
	void ClearEntity();

	DECLARE_EVENT_HANDLER(OnEntityActivated, OnEntityActivated);
	DECLARE_EVENT_HANDLER(OnEntityDeactivated, OnEntityDeactivated);

	//!!!to protected everywhere!
	virtual void Activate();
	virtual void Deactivate();

public:

	CProperty();
	virtual ~CProperty() = 0;

	virtual void GetAttributes(nArray<DB::CAttrID>& Attrs);

	bool		IsActive() const { return Active; }
	bool		HasEntity() const { return pEntity != NULL; }
	CEntity*	GetEntity() const { n_assert(pEntity); return pEntity; }
};

typedef Ptr<CProperty> PProperty;
typedef HashTable<CStrID, PProperty> CPropertyStorage;

}

#endif
