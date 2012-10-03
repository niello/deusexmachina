//------------------------------------------------------------------------------
//  nroot_main.cc
//  (C) 2000 A.Weissflog
//------------------------------------------------------------------------------
#include "kernel/nkernelserver.h"
#include "kernel/nroot.h"
#include "kernel/nmutex.h"

#include <stdlib.h>

nNebulaRootClass(nRoot);

nRoot::nRoot(): pClass(NULL), pParent(NULL)
{
	pMutex = n_new(nMutex);
}
//---------------------------------------------------------------------

nRoot::~nRoot()
{
	n_assert(!RefCount);

	if (this == nKernelServer::Instance()->GetCwd())
		nKernelServer::Instance()->SetCwd(NULL);

	nRoot* pChild;
	while (pChild = GetHead())
		while (!pChild->Release()) ;

	if (pParent) Remove();

	n_delete(pMutex);
}
//---------------------------------------------------------------------

bool nRoot::IsA(const char* className) const
{
	return IsA(nKernelServer::Instance()->FindClass(className));
}
//---------------------------------------------------------------------

bool nRoot::IsInstanceOf(const char* className) const
{
	return IsInstanceOf(nKernelServer::Instance()->FindClass(className));
}
//---------------------------------------------------------------------

bool nRoot::Release()
{
	n_assert(RefCount > 0);
	if (--RefCount) return false;
	pMutex->Lock(); // Do not delete as long as pMutex is set
	n_delete(this);
	return true;
}
//---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
     - 08-Oct-98   floh    created
     - 23-Jan-01   floh    why the f*ck was this method recursive???
     - 24-May-04    floh    returns nString
*/
nString nRoot::GetFullName() const
{
    // build stack of pointers leading from me to root
    const int maxDepth = 128;
    const nRoot* stack[maxDepth];
    const nRoot* cur = this;
    int i = 0;
    do stack[i++] = cur;
    while ((cur = cur->GetParent()) && i < maxDepth);

    // traverse stack in reverse order and build filename
    nString str;
    i--;
    for (; i >= 0; i--)
    {
        const char* curName = stack[i]->GetName();
        str.Append(curName);
        if ((curName[0] != '/') && (i > 0)) str.Append("/");
    }
    return str;
}

//------------------------------------------------------------------------------
/**
    Return shortest relative path leading from 'this' to 'other' object.
    This is a slow operation, unless one object is the pParent of
    the other (this is a special case optimization).

     - 06-Mar-00    floh    created
     - 21-Feb-04    floh    now accepts "other == this" (returns a dot)
     - 24-May-04    floh    rewritten to nString
*/
nString nRoot::GetRelPath(const nRoot* other) const
{
    n_assert(other);

    nString str;
    if (other == this) str = ".";
    else if (other == GetParent()) str = "..";
    else if (other->GetParent() == this) str = other->GetName();
    else
    {
        // normal case
        nArray<const nRoot*> thisHierarchy;
        nArray<const nRoot*> otherHierarchy;

        // for both objects, create lists of all parents up to root
        const nRoot* o = this;

		do thisHierarchy.Insert(0, o);
        while ((o = o->GetParent()));

		o = other;

		do otherHierarchy.Insert(0, o);
        while ((o = o->GetParent()));

        // remove identical parents
        bool running = true;
        do
        {
            if ((thisHierarchy.Size() > 0) && (otherHierarchy.Size() > 0))
            {
                const nRoot* o0 = thisHierarchy[0];
                const nRoot* o1 = otherHierarchy[0];
                if (o0 == o1)
                {
                    thisHierarchy.Erase(0);
                    otherHierarchy.Erase(0);
                }
                else running = false;
            }
            else running = false;
        }
        while (running);

        // create path leading upward from this to the identical pParent
        while (thisHierarchy.Size() > 0)
        {
            str.Append("../");
            thisHierarchy.Erase(thisHierarchy.Size() - 1);
        }
        while (otherHierarchy.Size() > 0)
        {
            str.Append(otherHierarchy[0]->GetName());
            str.Append("/");
            otherHierarchy.Erase(0);
        }

        // eliminate trailing '/'
        str.StripTrailingSlash();
    }

    // done
    return str;
}

//------------------------------------------------------------------------------
/**
    Compare-Hook for qsort() in nRoot::Sort()

     - 18-May-99   floh    created
*/
int __cdecl child_cmp(const void* e0, const void* e1)
{
    nRoot* r0 = *((nRoot**)e0);
    nRoot* r1 = *((nRoot**)e1);
    return strcmp(r1->GetName(), r0->GetName());
}

//------------------------------------------------------------------------------
/**
    Sort pChild objects alphabetically. This is a slow operation.

     - 18-May-99   floh    created
*/
void nRoot::Sort()
{
    int num,i;
    nRoot* c;

    // count pChild objects
    for (num = 0, c = GetHead(); c; c = c->GetSucc(), num++);

    if (num > 0)
    {
        nRoot** c_array = (nRoot**)n_malloc(num * sizeof(nRoot*));
        n_assert(c_array);
        for (i = 0, c = GetHead(); c; c = c->GetSucc(), i++)
            c_array[i] = c;
        qsort(c_array, num, sizeof(nRoot*), child_cmp);

        for (i = 0; i < num; i++)
        {
            c_array[i]->Remove();
            AddHead(c_array[i]);
        }
        n_free(c_array);
    }
}
//---------------------------------------------------------------------
