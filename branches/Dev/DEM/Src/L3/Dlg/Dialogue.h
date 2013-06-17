#pragma once
#ifndef __DEM_L3_DIALOGUE_H__
#define __DEM_L3_DIALOGUE_H__

#include <Dlg/DlgNode.h>
#include <Scripting/ScriptObject.h>

// Dialogue contains all necessary data to talk - set of dlg nodes, script name

namespace Scripting
{
	typedef Ptr<class CScriptObject> PScriptObject;
}

namespace Story
{
using namespace Scripting;

class CDlgNode;

class CDialogue: public Core::CRefCounted
{
public:

	CDlgNode*				StartNode;
	nArray<Ptr<CDlgNode>>	Nodes;
	nString					ScriptFile;
	PScriptObject			ScriptObj;	// Object is needed to avoid name clashes and to clear script at the end

	CDialogue(): StartNode(NULL) {}
};

typedef Ptr<class CDialogue> PDialogue;

}

#endif
