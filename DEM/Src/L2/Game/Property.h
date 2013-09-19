#pragma once
#ifndef __DEM_L2_GAME_PROPERTY_H__
#define __DEM_L2_GAME_PROPERTY_H__

// Properties are attached to game entities to add specific functionality or behaviors to the entity.

#include <Core/RefCounted.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>
#include <Data/HashTable.h>

namespace Game
{
class CEntity;
typedef Ptr<class CProperty> PProperty;
typedef CHashTable<CStrID, PProperty> CPropertyStorage;

class CProperty: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	friend class CEntityManager;

	CEntity*	pEntity;
	bool		Active;

	virtual bool	InternalActivate() = 0;
	virtual void	InternalDeactivate() = 0;
	void			SetEntity(CEntity* pNewEntity);

	DECLARE_EVENT_HANDLER(OnEntityActivated, OnEntityActivated);
	DECLARE_EVENT_HANDLER(OnEntityDeactivated, OnEntityDeactivated);

public:

	CProperty(): Active(false), pEntity(NULL) {}
	virtual ~CProperty();

	void						Activate();
	void						Deactivate();
	virtual CPropertyStorage*	GetStorage() const { return NULL; }
	CEntity*					GetEntity() const { n_assert(pEntity); return pEntity; }
	bool						IsActive() const { return Active; }
};

}

#define __DeclarePropertyStorage \
	public: \
		static Game::CPropertyStorage* pStorage; \
		virtual Game::CPropertyStorage* GetStorage() const; \
	private:

#define __ImplementPropertyStorage(Class) \
	Game::CPropertyStorage* Class::pStorage = NULL; \
	Game::CPropertyStorage* Class::GetStorage() const { return pStorage; }

#define PROP_SUBSCRIBE_NEVENT(EventName, Class, Handler) \
	GetEntity()->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler, &Sub_##EventName)
#define PROP_SUBSCRIBE_NEVENT_PRIORITY(EventName, Class, Handler, Priority) \
	GetEntity()->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler, &Sub_##EventName, Priority)
#define PROP_SUBSCRIBE_PEVENT(EventName, Class, Handler) \
	GetEntity()->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler, &Sub_##EventName)
#define PROP_SUBSCRIBE_PEVENT_PRIORITY(EventName, Class, Handler, Priority) \
	GetEntity()->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler, &Sub_##EventName, Priority)

#endif
