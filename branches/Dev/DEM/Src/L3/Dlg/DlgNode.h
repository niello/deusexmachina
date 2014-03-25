#pragma once
#ifndef __DEM_L3_DLG_NODE_H__
#define __DEM_L3_DLG_NODE_H__

#include <Core/RefCounted.h>
#include <Data/StringID.h>

// Dialogue graph node is associated with optional phrase and offers links to next nodes.
// One of valid links must be selected according to a link selection mode.

namespace Story
{
class CDlgContext;

class CDlgNode: public Core::CRefCounted
{
	__DeclareClass(CDlgNode);

public:

	enum ELinkMode
	{
		Link_Switch = 0,	// First valid link
		Link_Random = 1,	// Random valid link
		Link_Select = 2		// Valid link explicitly selected, for example in answer selection UI
	};

	struct CLink
	{
		CString		Condition;	// Scripted function name
		CString		Action;		// Scripted function name
		CDlgNode*	pTargetNode;
	};

	CStrID			SpeakerEntity; // May be entity UID or dlg context participator alias
	CString			Phrase;
	//float			Timeout; //???here or in associated sound resource?
	ELinkMode		LinkMode;
	CArray<CLink>	Links;

	virtual void	OnEnter(CDlgContext& Context);
	CDlgNode*		FollowLink(CDlgContext& Context);
};

}

#endif
