#pragma once
#ifndef __DEM_L1_RESOURCE_H__
#define __DEM_L1_RESOURCE_H__

#include <Core/Object.h>
#include <Data/StringID.h>

// Resource wraps a piece of abstract data, used by other systems. It can be loaded and managed by
// resource system, so clients needn't to care about loading, unloading and LOD of resources.

namespace Resources
{
class IResourceManager;

enum EResourceState
{
	Rsrc_NotLoaded,
	Rsrc_Loaded,
	Rsrc_Failed
	//Rsrc_Pending - for async
};

class CResource: public Core::CObject
{
	__DeclareClassNoFactory;

protected:

	CStrID				UID;
	DWORD				ByteSize;
	EResourceState		State;

	CResource(CStrID ID): UID(ID), ByteSize(0), State(Rsrc_NotLoaded) {} // Create only by manager

public:

	virtual ~CResource() {}

	virtual void	Unload() { State = Rsrc_NotLoaded; }

	CStrID			GetUID() const { return UID; }
	DWORD			GetSizeInBytes() const { return ByteSize; }
	EResourceState	GetState() const { return State; }
	bool			IsLoaded() const { return State == Rsrc_Loaded; }
};

typedef Ptr<CResource> PResource;

}

#define __ImplementResourceClass(Class, FourCC, ParentClass) \
	Core::CRTTI Class::RTTI(#Class, FourCC, Class::FactoryCreator, &ParentClass::RTTI, sizeof(Class)); \
	Core::CRTTI* Class::GetRTTI() const { return &RTTI; } \
	Core::CObject* Class::FactoryCreator(void* pParam) { return Class::CreateInstance(pParam); } \
	Class* Class::CreateInstance(void* pParam) { return n_new(Class)(*(CStrID*)pParam); } \
	bool Class::RegisterInFactory() \
	{ \
		if (!Factory->IsNameRegistered(#Class)) \
			Factory->Register(Class::RTTI, #Class, FourCC); \
		OK; \
	} \
	void Class::ForceFactoryRegistration() { Class::Registered; } \
	const bool Class::Registered = Class::RegisterInFactory();

#endif
