//--------------------------------------------------------------------
//  nclass.cc
//  (C) 1998..2000 Andre Weissflog
//--------------------------------------------------------------------
#include <stdlib.h>

#include "kernel/nclass.h"
#include "kernel/nroot.h"
#include "kernel/nkernelserver.h"

//--------------------------------------------------------------------
/**
    @param name               name of the class
    @param kserv              pointer to kernel server
    @param initFunc           pointer to n_init function in class package
    @param newFunc            pointer to n_create function in class package
*/
nClass::nClass(const char* name,
               bool (*initFunc)(nClass*, nKernelServer*),
               void* (*newFunc)()) :
    //nSignalRegistry(),
    nHashNode(name),
    superClass(0),
    refCount(0),
    instanceSize(0),
    n_init_ptr(initFunc),
    n_new_ptr(newFunc)
{
    // call the class module's init function
	n_init_ptr(this, nKernelServer::Instance());
}

//--------------------------------------------------------------------
/**
     - 04-Aug-99   floh    boeser Bug: nCmdProto-Liste wurde als nCmd
                           Objekte freigegeben...
     - 18-Feb-00   floh    + cmd_table is now a nKeyArray
*/
nClass::~nClass()
{
    // if I'm still connected to a superclass, unlink from superclass
    if (superClass) superClass->RemSubClass(this);

    n_assert(!superClass && !refCount);
}

//--------------------------------------------------------------------

nRoot* nClass::NewObject()
{
	nRoot* obj = (nRoot*)n_new_ptr();
	n_assert(obj);
	obj->AddRef();
	obj->pClass = this;
	return obj;
}

//--------------------------------------------------------------------
/**
    Checks if the given class is an ancestor of this class.

    @param className Name (all lowercase) of a class.
    @return true if className is an ancestor of this class, or if
            ancestorName is the name of this class.
            false otherwise.
*/
bool
nClass::IsA(const char* className) const
{
    n_assert(className);

    if (strcmp(GetName(), className) == 0)
        return true;

    nClass* ancestor = superClass;
    while (ancestor)
    {
        if (strcmp(ancestor->GetName(), className) == 0) return true;
        ancestor = ancestor->GetSuperClass();
    }

    return false;
}

//--------------------------------------------------------------------
//  EOF
//--------------------------------------------------------------------
