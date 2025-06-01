#pragma once

// Properties are attached to game entities to add specific functionality or behaviors to the entity.

#include <Core/Object.h>
#include <Data/StringID.h>
#include <Data/HashTable.h>
#include <Events/EventsFwd.h>

namespace Game
{
class CEntity;
typedef Ptr<class CProperty> PProperty;
typedef CHashTable<CStrID, PProperty> CPropertyStorage;

class CProperty: public DEM::Core::CObject
{
	RTTI_CLASS_DECL(Game::CProperty, DEM::Core::CObject);

protected:

	CEntity*	pEntity;
	bool		Active;

	virtual bool	InternalActivate() = 0;
	virtual void	InternalDeactivate() = 0;
	void			SetEntity(CEntity* pNewEntity);

	DECLARE_EVENT_HANDLER(OnActivated, OnEntityActivated);
	DECLARE_EVENT_HANDLER(OnDeactivated, OnEntityDeactivated);

public:

	CProperty();
	virtual ~CProperty();

	virtual bool				Initialize() { OK; }
	void						Activate();
	void						Deactivate();
	virtual CPropertyStorage*	GetStorage() const { return nullptr; }
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
	Game::CPropertyStorage* Class::pStorage = nullptr; \
	Game::CPropertyStorage* Class::GetStorage() const { return pStorage; }

#define PROP_SUBSCRIBE_NEVENT(EventName, Class, Handler) \
	Sub_##EventName = GetEntity()->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler)
#define PROP_SUBSCRIBE_NEVENT_PRIORITY(EventName, Class, Handler, Priority) \
	Sub_##EventName = GetEntity()->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler, Priority)
#define PROP_SUBSCRIBE_PEVENT(EventName, Class, Handler) \
	Sub_##EventName = GetEntity()->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler)
#define PROP_SUBSCRIBE_PEVENT_PRIORITY(EventName, Class, Handler, Priority) \
	Sub_##EventName = GetEntity()->Subscribe<Class>(CStrID(#EventName), this, &Class::Handler, Priority)
