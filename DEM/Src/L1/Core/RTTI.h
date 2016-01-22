#pragma once
#ifndef __DEM_L1_RTTI_H__
#define __DEM_L1_RTTI_H__

#include <Data/FourCC.h>
#include <Data/String.h>

// Implements the runtime type information system of Mangalore. Every class derived
// from CRTTIBaseClass should define a static RTTI object which is initialized
// with a pointer to a static string containing the human readable Name
// of the class, and a pointer to the static RTTI object of the Parent class.

namespace Core
{
class CRTTIBaseClass;

class CRTTI
{
private:

	typedef CRTTIBaseClass* (*CFactoryFunc)(void* pParam);

	CString			Name; //???use CString?
	Data::CFourCC	FourCC;
	UPTR			InstanceSize;

	const CRTTI*	pParent;
	CFactoryFunc	pFactoryFunc;

public:


	CRTTI(const char* pClassName, Data::CFourCC ClassFourCC, CFactoryFunc pFactoryCreator, const CRTTI* pParentClass, UPTR InstSize);

	CRTTIBaseClass*	CreateClassInstance(void* pParam = NULL) const { return pFactoryFunc ? pFactoryFunc(pParam) : NULL; }
	//void*			AllocInstanceMemory() const { return n_malloc(InstanceSize); }
	//void			FreeInstanceMemory(void* pPtr) { n_free(pPtr); }

	const CString&	GetName() const { return Name; }
	Data::CFourCC	GetFourCC() const { return FourCC; }
	const CRTTI*	GetParent() const { return pParent; }
	UPTR			GetInstanceSize() const;
	bool			IsDerivedFrom(const CRTTI& Other) const;
	bool			IsDerivedFrom(Data::CFourCC OtherFourCC) const;
	bool			IsDerivedFrom(const char* pOtherName) const;

	bool operator ==(const CRTTI& Other) const { return this == &Other; }
	bool operator !=(const CRTTI& Other) const { return this != &Other; }
};

inline CRTTI::CRTTI(const char* pClassName, Data::CFourCC ClassFourCC, CFactoryFunc pFactoryCreator, const CRTTI* pParentClass, UPTR InstSize):
	Name(pClassName),
	FourCC(ClassFourCC),
	InstanceSize(InstSize),
	pParent(pParentClass),
	pFactoryFunc(pFactoryCreator)
{
	n_assert(pClassName && *pClassName);
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

inline bool CRTTI::IsDerivedFrom(Data::CFourCC OtherFourCC) const
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

inline bool CRTTI::IsDerivedFrom(const char* pOtherName) const
{
	const CRTTI* pCurr = this;
	while (pCurr)
	{
		if (pCurr->Name == pOtherName) OK;
		pCurr = pCurr->pParent;
	}
	FAIL;
}
//---------------------------------------------------------------------

}

// With factory

//	void* operator new(size_t size) { return RTTI.AllocInstanceMemory(); };
//	void operator delete(void* p) { RTTI.FreeInstanceMemory(p); };
#define __DeclareClass(Class) \
public: \
	static Core::CRTTI				RTTI; \
	static const bool				Factory_Registered; \
	virtual Core::CRTTI*			GetRTTI() const { return &RTTI; } \
	static Core::CRTTIBaseClass*	CreateClassInstance(void* pParam = NULL); \
	static bool						RegisterInFactory(); \
	static void						ForceFactoryRegistration(); \
private:

#define __ImplementClass(Class, FourCC, ParentClass) \
	Core::CRTTI Class::RTTI(#Class, FourCC, Class::CreateClassInstance, &ParentClass::RTTI, sizeof(Class)); \
	Core::CRTTIBaseClass* Class::CreateClassInstance(void* pParam) { return n_new(Class); } \
	bool Class::RegisterInFactory() \
	{ \
		if (!Factory->IsNameRegistered(#Class)) \
			Factory->Register(Class::RTTI, #Class, FourCC); \
		OK; \
	} \
	void Class::ForceFactoryRegistration() { Class::Factory_Registered; } \
	const bool Class::Factory_Registered = Class::RegisterInFactory();

#define __ImplementRootClass(Class, FourCC) \
	Core::CRTTI Class::RTTI(#Class, FourCC, Class::CreateClassInstance, NULL, sizeof(Class)); \
	Core::CRTTIBaseClass* Class::CreateClassInstance(void* pParam) { return n_new(Class); } \
	bool Class::RegisterInFactory() \
	{ \
		if (!Factory->IsNameRegistered(#Class)) \
			Factory->Register(Class::RTTI, #Class, FourCC); \
		OK; \
	}

// Without factory

#define __DeclareClassNoFactory \
public: \
	static Core::CRTTI				RTTI; \
	virtual Core::CRTTI*			GetRTTI() const { return &RTTI; } \
private:

#define __ImplementClassNoFactory(Class, ParentClass) \
	Core::CRTTI Class::RTTI(#Class, 0, NULL, &ParentClass::RTTI, 0);

#define __ImplementRootClassNoFactory(Class, FourCC) \
	Core::CRTTI Class::RTTI(#Class, FourCC, NULL, NULL, 0);

#endif
