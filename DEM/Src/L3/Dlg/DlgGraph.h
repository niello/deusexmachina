#pragma once
#ifndef __DEM_L3_DLG_GRAPH_H__
#define __DEM_L3_DLG_GRAPH_H__

#include <Dlg/DlgNode.h>
#include <Scripting/ScriptObject.h>

// Dialogue graph is a full stateless reusable resource containing dialogue graph that
// can be traversed by the dialogue manager in a certain context, and associated settings.

namespace Scripting
{
	typedef Ptr<class CScriptObject> PScriptObject;
}

namespace Story
{
typedef Ptr<class CDlgNode> PDlgNode;

class CDlgGraph: public Core::CRefCounted
{
public:

	CDlgNode*					StartNode;
	CArray<PDlgNode>			Nodes;
	CString						ScriptFile;
	Scripting::PScriptObject	ScriptObj;	// Stateless. Object is needed to avoid name clashes and to clear script at the end.

	CDlgGraph(): StartNode(NULL) {}
};

typedef Ptr<CDlgGraph> PDlgGraph;

}

#endif