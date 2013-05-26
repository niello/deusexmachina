#pragma once
#ifndef __DEM_L1_RTTI_H__
#define __DEM_L1_RTTI_H__

#include <StdDEM.h>
#include <util/nstring.h>

// Implements the runtime type information system of Mangalore. Every class
// derived from CRefCounted should define a static RTTI object which is initialized
// with a pointer to a static string containing the human readable Name
// of the class, and a pointer to the static RTTI object of the Parent class.

namespace Core
{
class CRefCounted;

class CRTTI
{
private:

	typedef CRefCounted* (*CFactoryFunc)(void* pParam);

	nString			Name;
	nFourCC			FourCC;
	DWORD			InstanceSize;

	const CRTTI*	pParent;
	CFactoryFunc	pFactoryFunc;

public:


	CRTTI(const nString& ClassName, nFourCC ClassFourCC, CFactoryFunc pFactoryCreator, const CRTTI* pParentClass, DWORD InstSize);

	CRefCounted*	CreateInstance(void* pParam = NULL) const { return pFactoryFunc ? pFactoryFunc(pParam) : NULL; }
	//void*			AllocInstanceMemory() const { return n_malloc(InstanceSize); }
	//void			FreeInstanceMemory(void* pPtr) { n_free(pPtr); }

	const nString&	GetName() const { return Name; }
	nFourCC			GetFourCC() const { return FourCC; }
	const CRTTI*	GetParent() const { return pParent; }
	DWORD			GetInstanceSize() const;
	bool			IsDerivedFrom(const CRTTI& Other) const;
	bool			IsDerivedFrom(nFourCC OtherFourCC) const;
	bool			IsDerivedFrom(const nString& OtherName) const;

	bool operator ==(const CRTTI& Other) const { return this == &Other; }
	bool operator !=(const CRTTI& Other) const { return this != &Other; }
};
//---------------------------------------------------------------------

inline CRTTI::CRTTI(const nString& ClassName, nFourCC ClassFourCC, CFactoryFunc pFactoryCreator, const CRTTI* pParentClass, DWORD InstSize):
	Name(ClassName),
	FourCC(ClassFourCC),
	InstanceSize(InstSize),
	pParent(pParentClass),
	pFactoryFunc(pFactoryCreator)
{
	n_assert(ClassName.IsValid());
	n_assert(pParentClass != this);
}
//---------------------------------------------------------------------

inline bool CRTTI::IsDerivedFrom(const CRTTI& Other) const
{
	const CRTTI* pCurr = this;
	while (pCurr)
	{
		if (pCurr == &Other) OK;
		pCurr = pCurr->pParent;
	}
	FAIL;
}
//---------------------------------------------------------------------

inline bool CRTTI::IsDerivedFrom(nFourCC OtherFourCC) const
{
	const CRTTI* pCurr = this;
	while (pCurr)
	{
		if (pCurr->FourCC == OtherFourCC) OK;
		pCurr = pCurr->pParent;
	}
	FAIL;
}
//---------------------------------------------------------------------

inline bool CRTTI::IsDerivedFrom(const nString& OtherName) const
{
	const CRTTI* pCurr = this;
	while (pCurr)
	{
		if (pCurr->Name == OtherName) OK;
		pCurr = pCurr->pParent;
	}
	FAIL;
}
//---------------------------------------------------------------------

}

//	void* operator new(size_t size) return RTTI.AllocInstanceMemory(); };
//	void operator delete(void* p) { RTTI.FreeInstanceMemory(p); };
#define __DeclareClass(Class) \
public: \
	static Core::CRTTI	RTTI; \
	static const bool	Registered; \
	virtual Core::CRTTI*		GetRTTI() const; \
	static Core::CRefCounted*	FactoryCreator(void* pParam); \
	static Class*				CreateInstance(void* pParam = NULL); \
	static bool					RegisterInFactory(); \
	static void					ForceFactoryRegistration(); \
private:

#define __DeclareClassNoFactory \
public: \
	static Core::CRTTI RTTI; \
	virtual Core::CRTTI* GetRTTI() const; \
private:

#define __ImplementClass(Class, FourCC, ParentClass) \
	Core::CRTTI Class::RTTI(#Class, FourCC, Class::FactoryCreator, &ParentClass::RTTI, sizeof(Class)); \
	Core::CRTTI* Class::GetRTTI() const { return &RTTI; } \
	Core::CRefCounted* Class::FactoryCreator(void* pParam) { return Class::CreateInstance(pParam); } \
	Class* Class::CreateInstance(void* pParam) { return n_new(Class); } \
	bool Class::RegisterInFactory() \
	{ \
		if (!Factory->IsRegistered(#Class)) \
			Factory->Register(Class::RTTI, #Class, FourCC); \
		OK; \
	} \
	void Class::ForceFactoryRegistration() { Class::Registered; } \
	const bool Class::Registered = Class::RegisterInFactory();

//#define __ImplementClassNoFactory(Class, FourCC, ParentClass) \
//	Core::CRTTI Class::RTTI(#Class, FourCC, NULL, &ParentClass::RTTI, 0); \
//	Core::CRTTI* Class::GetRTTI() const { return &RTTI; }
#define __ImplementClassNoFactory(Class, ParentClass) \
	Core::CRTTI Class::RTTI(#Class, 0, NULL, &ParentClass::RTTI, 0); \
	Core::CRTTI* Class::GetRTTI() const { return &RTTI; }

#define __ImplementRootClass(Class, FourCC) \
	Core::CRTTI Class::RTTI(#Class, FourCC, Class::FactoryCreator, NULL, sizeof(Class)); \
	Core::CRTTI* Class::GetRTTI() const { return &RTTI; } \
	Core::CRefCounted* Class::FactoryCreator(void* pParam) { return Class::CreateInstance(pParam); } \
	Class* Class::CreateInstance(void* pParam) { return n_new(Class); } \
	bool Class::RegisterInFactory() \
	{ \
		if (!Factory->IsRegistered(#Class)) \
			Factory->Register(Class::RTTI, #Class, FourCC); \
		OK; \
	}

#define __ImplementRootClassNoFactory(Class, FourCC) \
	Core::CRTTI Class::RTTI(#Class, FourCC, NULL, NULL, 0); \
	Core::CRTTI* Class::GetRTTI() const { return &RTTI; }

#endif
