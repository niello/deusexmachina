#pragma once
#ifndef __DEM_L1_SINGLETON_H__
#define __DEM_L1_SINGLETON_H__

// Provides helper macros to implement singleton objects:
//
// - __DeclareSingleton      put this into class declaration
// - __ImplementSingleton    put this into the implementation file
// - __ConstructSingleton    put this into the constructor
// - __DestructSingleton     put this into the destructor
//
// Get a pointer to a singleton object using the static Instance() method:
//
// (C) 2007 Radon Labs GmbH

#define __DeclareSingleton(type) \
public: \
	static type* Singleton; \
    static inline type* Instance() { n_assert(Singleton); return Singleton; }; \
    static inline bool HasInstance() { return Singleton != nullptr; }; \
private:

#define __ImplementSingleton(type) \
	type * type::Singleton = nullptr;

#define __ConstructSingleton \
    n_assert(!Singleton); Singleton = this;

#define __DestructSingleton \
    n_assert(Singleton); Singleton = nullptr;

#endif