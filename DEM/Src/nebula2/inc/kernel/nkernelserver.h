#ifndef N_KERNELSERVER_H
#define N_KERNELSERVER_H

#include "util/nstack.h"
#include "util/nhashlist.h"
#include <Data/Singleton.h>

// Every Nebula2 application needs exactly one kernel server object which
// persists throughout the lifetime of the application.
// (C) 2002 RadonLabs GmbH

class nRoot;
class nClass;

class nKernelServer
{
	__DeclareSingleton(nKernelServer);

private:

	nHashList		ClassList;	// list of nClass objects
	nRoot*			pRoot;		// the pRoot object of the Nebula object hierarchy
	nRoot*			pCwd;		// the current working object
	nStack<nRoot*>	CwdStack;	// stack of previous pCwd's

	class nMutex*	pMutex;                   // the kernel lock pMutex

	nRoot* CheckCreatePath(const char* pClassName, const char* path, bool dieOnError);
	nRoot* NewUnnamedObject(const char* pClassName);

public:

	nKernelServer();
	~nKernelServer();

	void	AddClass(const char* superClassName, nClass* cl);
	nClass*	FindClass(const char* pClassName);

	nRoot*	New(const char* pClassName, const char* objectName, bool DieOnError = true);
	nRoot*	Lookup(const char* path);

	void	SetCwd(nRoot* newCwd);
	nRoot*	GetCwd() { return pCwd; }
	void	PushCwd(nRoot* newCwd);
	nRoot*	PopCwd();

	void	AddPackage(void (*_func)()) { _func(); }
	void	AddModule(const char *, bool (*_init_func)(nClass*, nKernelServer*), void* (*_new_func)());
};

#endif
