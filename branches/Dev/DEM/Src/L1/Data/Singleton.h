#pragma once
//------------------------------------------------------------------------------
/**
    @file core/win32/win32singleton.h

    Provides helper macros to implement singleton objects:
    
    - __DeclareSingleton      put this into class declaration
    - __ImplementSingleton    put this into the implemention file
    - __ConstructSingleton    put this into the constructor
    - __DestructSingleton     put this into the destructor

    Get a pointer to a singleton object using the static Instance() method:

    Core::Server* coreServer = Core::Server::Instance();
    
    (C) 2007 Radon Labs GmbH
*/

//------------------------------------------------------------------------------
    //ThreadLocal
#define __DeclareSingleton(type) \
public: \
	static type * Singleton; \
    static type * Instance() { n_assert(Singleton); return Singleton; }; \
    static bool HasInstance() { return Singleton != NULL; }; \
private:

#define __DeclareInterfaceSingleton(type) \
public: \
    static type * Singleton; \
    static type * Instance() { n_assert(Singleton); return Singleton; }; \
    static bool HasInstance() { return Singleton != NULL; }; \
private:

    //ThreadLocal
#define __ImplementSingleton(type) \
	type * type::Singleton = NULL;

#define __ImplementInterfaceSingleton(type) \
    type * type::Singleton = NULL;

#define __ConstructSingleton \
    n_assert(!Singleton); Singleton = this;

#define __ConstructInterfaceSingleton \
    n_assert(!Singleton); Singleton = this;

#define __DestructSingleton \
    n_assert(Singleton); Singleton = NULL;

#define __DestructInterfaceSingleton \
    n_assert(Singleton); Singleton = NULL;
//------------------------------------------------------------------------------
