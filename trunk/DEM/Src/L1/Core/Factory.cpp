#include "Factory.h"

#include <Core/RTTI.h>

namespace Core
{

CFactory* CFactory::Instance()
{
	static CFactory Singleton;
	return &Singleton;
}
//---------------------------------------------------------------------

void CFactory::Add(CFactoryFunction Function, const nString& ClassName)
{
	n_assert2(!Has(ClassName), ClassName.Get());
	ClassTable.Add(ClassName.Get(), Function);
}
//---------------------------------------------------------------------

CRefCounted* CFactory::Create(const nString& ClassName) const
{
	CFactoryFunction* f = ClassTable.Get(ClassName.Get());
	n_assert2(f, ClassName.Get());
	return (*f)();
}
//---------------------------------------------------------------------

CRefCounted* CFactory::Create(const CRTTI& Class) const
{
	//???map RTTI->FactoryFunction?
	return Create(Class.GetName());
}
//---------------------------------------------------------------------

} // namespace Core
