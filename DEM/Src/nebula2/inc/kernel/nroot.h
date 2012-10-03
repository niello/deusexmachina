#ifndef N_ROOT_H
#define N_ROOT_H
//------------------------------------------------------------------------------
/**
    nRoot defines the basic functionality and interface for
    NOH dependent classes in the Nebula class hierarchy.

    Rules for subclasses:
    - only the default constructor is allowed
    - never use new/delete (or variants like n_new/n_delete) with
      nRoot objects
    - use nKernelServer::New() to create an object and
      the objects Release() method to destroy it

    (C) 1999 RadonLabs GmbH
*/

#include "kernel/nclass.h"
#include "kernel/nreferenced.h"
#include "util/nstring.h"
#include "util/nnode.h"

class nMutex;

class nRoot: public nReferenced, public nNode
{
protected:

	friend class nClass;

	nClass*	pClass;
    nString	name; //!!!can use CStrID!
    nRoot*	pParent;
    nList	childList;
    nMutex*	pMutex;

    /// destructor (DONT CALL DIRECTLY, USE Release() INSTEAD)
    virtual ~nRoot();

public:

	/// constructor (DONT CALL DIRECTLY, USE nKernelServer::New() INSTEAD)
    nRoot();

	nClass* GetClass() const { return pClass; }
    bool IsA(const nClass*) const;
    bool IsA(const char*) const;
	bool IsInstanceOf(const nClass* cl) const { return (cl == pClass); }
    bool IsInstanceOf(const char*) const;

	virtual void Initialize() {}
    virtual bool Release();

	void		SetName(const char* str) { name = str; }
	const char*	GetName() const { return name.Get(); }
    nString		GetFullName() const;
    nString		GetRelPath(const nRoot* other) const;

    void		Sort();
	nRoot*		Find(const char* str);
	void		AddHead(nRoot* n) { n->pParent = this; childList.AddHead(n); }
	void		AddTail(nRoot* n) { n->pParent = this; childList.AddTail(n); }
	nRoot*		RemHead() { nRoot* n = (nRoot*)childList.RemHead(); if (n) n->pParent = NULL; return n; }
	nRoot*		RemTail() { nRoot* n = (nRoot*)childList.RemTail(); if (n) n->pParent = NULL; return n; }
	void		Remove() { nNode::Remove(); pParent = NULL; }

	nRoot*		GetParent() const { return pParent; }
	nRoot*		GetHead() const { return (nRoot*)childList.GetHead(); }
	nRoot*		GetTail() const { return (nRoot*)childList.GetTail(); }
	nRoot*		GetSucc() const { return (nRoot*)nNode::GetSucc(); }
	nRoot*		GetPred() const { return (nRoot*)nNode::GetPred(); }
};

inline bool nRoot::IsA(const nClass* cl) const
{
	nClass* actClass = pClass;
	while (actClass)
	{
		if (actClass == cl) return true;
		actClass = actClass->GetSuperClass();
	}
	return false;
}
//---------------------------------------------------------------------

inline nRoot* nRoot::Find(const char* str)
{
	n_assert(str);

	if (str[0] == '.')
	{
		if (str[1] == 0) return this;
		if ((str[1] == '.') && (str[2] == 0)) return pParent;
	}

	nRoot* child;
	for (child = GetHead(); child; child = child->GetSucc())
		if (child->name == str) return child;

	return 0;
}
//---------------------------------------------------------------------

#endif
