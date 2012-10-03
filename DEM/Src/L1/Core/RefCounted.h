#pragma once
#ifndef __DEM_L1_REFCOUNTED_H__
#define __DEM_L1_REFCOUNTED_H__

#include <Core/RTTI.h>
#include <Core/CoreServer.h>
#include <Core/Factory.h>
#include <kernel/ntypes.h>
#include <util/nnode.h>

// Class with simple refcounting mechanism, and security check at application shutdown.

namespace Core
{

class CRefCounted: private nNode
{
	DeclareRTTI;

private:

	//friend class CCoreServer;

	int RefCount;

protected:

	virtual ~CRefCounted() = 0;

public:

	CRefCounted();

	void			AddRef() { ++RefCount; }
	void			Release() { n_assert(RefCount > 0); if (--RefCount == 0) n_delete(this); }
	int				GetRefCount() const { return RefCount; }
	bool			IsInstanceOf(const CRTTI& Other) const { return GetRTTI() == &Other; }
	bool			IsInstanceOf(const nString& Other) const { return GetRTTI()->GetName() == Other; }
	bool			IsA(const CRTTI& Other) const;
	bool			IsA(const nString& Other) const;
	const nString&	GetClassName() const { return GetRTTI()->GetName(); }
};
//---------------------------------------------------------------------

inline bool CRefCounted::IsA(const CRTTI& Other) const
{
	for (const CRTTI* i = GetRTTI(); i != NULL; i = i->GetParent())
		if (i == &Other)
			return true;
	return false;
}
//---------------------------------------------------------------------

inline bool CRefCounted::IsA(const nString& Other) const
{
	for (const CRTTI* i = GetRTTI(); i != NULL; i = i->GetParent())
		if (i->GetName() == Other)
			return true;
	return false;
}
//---------------------------------------------------------------------

// Declare factory macro.
#define DeclareFactory(classname) \
public: \
    static classname* Create(); \
    static Core::CRefCounted* InternalCreate(); \
	static bool RegisterFactoryFunction(); \
private:

// Register factory macro.
#define RegisterFactory(classname) \
static bool factoryRegistered_##classname = classname::RegisterFactoryFunction();

// Implement factory macro.
#define ImplementFactory(classname) \
classname* classname::Create() \
{ \
    return (classname*)InternalCreate(); \
} \
Core::CRefCounted* classname::InternalCreate() \
{ \
	classname* result = n_new(classname); \
	n_assert(result != 0); \
	return result; \
} \
bool classname::RegisterFactoryFunction() \
{ \
	if (!Core::CFactory::Instance()->Has(#classname)) \
		Core::CFactory::Instance()->Add(classname::InternalCreate, #classname); \
	return true; \
}

#define __DeclareClass(classname) \
	DeclareRTTI; \
	DeclareFactory(classname);

#define __ImplementClass(type, fourcc, baseType) \
	ImplementRTTI(type, baseType); \
	ImplementFactory(type);

}

#endif
