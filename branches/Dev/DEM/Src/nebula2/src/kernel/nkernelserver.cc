//------------------------------------------------------------------------------
//  nkernelserver.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>

#include "kernel/nkernelserver.h"
#include "kernel/nmutex.h"
#include "kernel/nroot.h"

__ImplementSingleton(nKernelServer);

extern bool n_init_nroot (nClass *, nKernelServer *);
extern void *n_new_nroot (void);

nKernelServer::nKernelServer(): pRoot(NULL), pCwd(NULL), ClassList(64)
{
	__ConstructSingleton;
	pMutex = n_new(nMutex);
	nKernelServer::Instance()->AddModule("nroot", n_init_nroot, n_new_nroot);
	pRoot = NewUnnamedObject("nroot");
	n_assert(pRoot);
	pRoot->SetName("/");
	pCwd = pRoot;
}
//---------------------------------------------------------------------

nKernelServer::~nKernelServer()
{
    pMutex->Lock();

    pRoot->Release(); // Kills _all_ hierarchy

    // kill class list
    // ===============
    // Class objects must be released in inheritance order,
    // subclasses first, then parent class. Do multiple
    // runs over the class list and release classes
    // with a ref count of 0, until list is empty or
    // no classes with a ref count of 0 zero exists
    // (that's a fatal error, there are still objects
    // around of that class).
    bool isEmpty;
    long numZeroRefs = 1;
    while ((!(isEmpty = ClassList.IsEmpty())) && (numZeroRefs > 0))
    {
        numZeroRefs = 0;
        nClass* actClass = (nClass*)ClassList.GetHead();
        nClass* nextClass;
        do
        {
            nextClass = (nClass*)actClass->GetSucc();
            if (actClass->GetRefCount() == 0)
            {
                numZeroRefs++;
                actClass->Remove();
                n_delete(actClass);
            }
            actClass = nextClass;
        }
		while (actClass);
    }

	if (!isEmpty)
    {
        n_printf("~nKernelServer(): ref_count error cleaning up class list!\n");
        n_printf("Offending classes:\n");
        for (nClass* actClass = (nClass*)ClassList.GetHead(); actClass; actClass = (nClass*)actClass->GetSucc())
            n_printf("%s: refcount %d\n", actClass->GetName(), actClass->GetRefCount());
        n_error("nKernelServer: Refcount errors occurred during cleanup, check log for details!\n");
    }

    pMutex->Unlock();
	n_delete(pMutex);

    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
    Loads a class and return a pointer to it.

    @param    pClassName   name of the class to be opened
    @return               pointer to class object

     - 08-Oct-98 floh    created
     - 04-Oct-98 floh    char * -> const char *
     - 10-Aug-99 floh    + if class not loaded, now first looks into
                         package tocs before trying to load directly
                         from dll
     - 24-Oct-99 floh    returns zero if class could not be loaded
     - 29-Jul-02 floh    loading from dll's no longer supported, all classes
                         must now be part of a class package!
*/

// Create a new unnamed Nebula object.
nRoot* nKernelServer::NewUnnamedObject(const char* pClassName)
{
    n_assert(pClassName);
    pMutex->Lock();
    nClass* cl = (nClass*)ClassList.Find(pClassName);
	nRoot* obj = 0;
    if (cl) obj = (nRoot*)cl->NewObject();
    pMutex->Unlock();
    return obj;
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Lookup an object by the given pPath and return a pointer to it.
    Check whether object described by pPath exists, returns
    pointer or NULL.

    @param  pPath    the pPath of the object to be found
    @return         pointer to object, or 0

     - 08-Oct-98   floh    created
     - 04-Oct-98   floh    char * -> const char *
     - 11-Dec-01   floh    bugfix: returned 'pCwd' if "" string given,
                           now return 0
*/
nRoot* nKernelServer::Lookup(const char* pPath)
{
    n_assert(pPath);
    pMutex->Lock();
    nRoot* pCurr = NULL;

    char* nextPathComponent;
    char strBuf[N_MAXPATH];

    // copy pPath to scratch buffer
    char* str = strBuf;
    n_strncpy2(strBuf, pPath, sizeof(strBuf));
	pCurr = (str[0] == '/') ? pRoot : pCwd;

    while ((nextPathComponent = strtok(str, "/")) && pCurr)
    {
        if (str) str = NULL;
        pCurr = pCurr->Find(nextPathComponent);
    }

    pMutex->Unlock();
    return pCurr;
}

//------------------------------------------------------------------------------
/**
    Create a new object and create all missing objects in the hierarchy.
    Create object described by pPath, fill up missing pPath components
    with nRoot objects.

    @param  pClassName   The name of the object
    @param  pPath        Path where the object should be created
    @param  dieOnError  Flag on true creates a n_error message "Aborting\n", on false doesn't

     - 08-Oct-98   floh    created
     - 04-Oct-98   floh    char * -> const char *
     - 01-Dec-98   floh    if object exists, increase ref count
     - 24-Oct-99   floh    don't break on problems, instead return NULL
*/
nRoot*
nKernelServer::CheckCreatePath(const char* pClassName, const char* pPath, bool dieOnError)
{
    n_assert(pClassName);
    n_assert(pPath);

    nRoot* parent = 0;
    nRoot* child  = 0;
    char* curPathComponent;
    char* nextPathComponent;
    char strBuf[N_MAXPATH];

    // copy pPath to scratch buffer
    n_strncpy2(strBuf, pPath, sizeof(strBuf));
	parent = (strBuf[0] == '/') ? pRoot : pCwd;

    curPathComponent = strtok(strBuf, "/");
    if (curPathComponent)
    {
        // for each directory pPath component
        while ((nextPathComponent = strtok(NULL, "/")))
        {
            child = parent->Find(curPathComponent);
            if (!child)
            {
                // subdir doesn't exist, fill up
                child = NewUnnamedObject("nroot");
                if (child)
                {
                    child->SetName(curPathComponent);
                    parent->AddTail(child);
                    child->Initialize();
                }
                else
                {
                    if (dieOnError) n_error("nKernelServer: Couldn't create object '%s' in pPath '%s'.\n", curPathComponent, pPath);
                    else            n_printf("nKernelServer: Couldn't create object '%s' in pPath '%s'.\n", curPathComponent, pPath);
                    return 0;
                }
            }
            parent = child;
            curPathComponent = nextPathComponent;
        }

        // curPathComponent is now name of last pPath component
        child = parent->Find(curPathComponent);
        if (!child)
        {
            // create and link object
            child = NewUnnamedObject(pClassName);
            if (child)
            {
                child->SetName(curPathComponent);
                parent->AddTail(child);
                child->Initialize();
            }
            else
            {
                if (dieOnError) n_error("nKernelServer: Couldn't create object '%s' of class '%s'.\n", pPath, pClassName);
                else            n_printf("nKernelServer: Couldn't create object '%s' of class '%s'.\n", pPath, pClassName);
                return 0;
            }
        }
    }
    else
    {
        if (dieOnError) n_error("nKernelServer: Empty name for new object of class '%s'!\n", pClassName);
        else            n_printf("nKernelServer: Empty name for new object of class '%s'!\n", pClassName);
        return 0;
    }
    return child;
}
//---------------------------------------------------------------------

void nKernelServer::AddClass(const char* superClassName, nClass* cl)
{
	n_assert(superClassName && cl);
	pMutex->Lock();
	nClass* superClass = (nClass*)ClassList.Find(superClassName);
	if (superClass) superClass->AddSubClass(cl);
	else n_error("nKernelServer::AddClass(): Could not open super class '%s'\n", superClassName);
	pMutex->Unlock();
}
//---------------------------------------------------------------------

/**
    Return pointer to class object defined by pClassName. If the class
    is not loaded, 0 is returned

    @param  pClassName   Name of the class
    @return             pointer to class object or 0

     - 08-Oct-98   floh    created
     - 04-Oct-98   floh    char * -> const char *
*/
nClass*
nKernelServer::FindClass(const char* pClassName)
{
    n_assert(pClassName);
    pMutex->Lock();
    nClass* cl = (nClass*)ClassList.Find(pClassName);
    pMutex->Unlock();
    return cl;
}

//------------------------------------------------------------------------------
/**
    Create a Nebula object given a class name and a path in the
    Nebula object hierarchy. This method will abort the Nebula app with
    a fatal error if the object couldn't be created.

    @param  pClassName   Name of the object
    @param  pPath        Path where to create the new object in the hierarchy
    @return             pointer to class object

     - 08-Oct-98   floh    created
     - 04-Oct-98   floh    char * -> const char *
     - 15-Jul-99   floh    uses Link() on object
     - 29-Jul-99   floh    Link() killed
     - 24-Oct-99   floh    throws a fatal error if object could not
                           be created
     - 04-Oct-00   floh    + keep pointer to last created object
*/
nRoot* nKernelServer::New(const char* pClassName, const char* pPath, bool DieOnError)
{
	n_assert(pClassName && pPath);
	pMutex->Lock();
	nRoot* o = CheckCreatePath(pClassName, pPath, DieOnError);
	if (DieOnError) { n_assert(o); }
	else if (!o) n_printf("nKernelServer: Couldn't create object of class '%s'.\n", pClassName);
	pMutex->Unlock();
	return o;
}

//------------------------------------------------------------------------------
/**
    Set the current working object.

    @param  o   pointer to new current working object

     - 08-Oct-98   floh    created
     - 13-May-99   floh    + if NULL pointer given, set pRoot object
*/
void nKernelServer::SetCwd(nRoot* o)
{
    pMutex->Lock();
    pCwd = o ? o : pRoot;
    pMutex->Unlock();
}

//------------------------------------------------------------------------------
/**
    Push current working object on a stack, and set new current working object.

    @param  o   pointer to new current working object

     - 28-Sep-00   floh    created
*/
void
nKernelServer::PushCwd(nRoot* o)
{
    pMutex->Lock();
    CwdStack.Push(pCwd);
    pCwd = o ? o : pRoot;
    pMutex->Unlock();
}

//------------------------------------------------------------------------------
/**
    Pop working object from stack, and set as new working object.
    Return previous working object.

     - 28-Sep-00   floh    created
*/
nRoot*
nKernelServer::PopCwd()
{
    pMutex->Lock();
    nRoot* prevCwd = pCwd;
    pCwd = CwdStack.Pop();
    pMutex->Unlock();
    return prevCwd;
}

//------------------------------------------------------------------------------
/**
    Add a new class package module to the class list. Normally called
    from the n_init() function of a class package.
*/
void
nKernelServer::AddModule(const char* name,
                         bool (*_init_func)(nClass*, nKernelServer*),
                         void* (*_new_func)())
{
    pMutex->Lock();
    nClass* cl = (nClass*)ClassList.Find(name);
    if (!cl)
    {
        cl = n_new(nClass(name, _init_func, _new_func));
        ClassList.AddTail(cl);
    }
    pMutex->Unlock();
}
