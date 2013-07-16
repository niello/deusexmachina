#pragma once
#ifndef __DEM_L1_SCENE_BONE_H__
#define __DEM_L1_SCENE_BONE_H__

#include <Scene/NodeAttribute.h>
#include <Math/TransformSRT.h>

// Skeletal animation bone. Manages skinning matrix, which is used by shader through node's shader params.

namespace Scene
{
struct CSPSRecord;

class CBone: public CNodeAttribute
{
	__DeclareClass(CBone);

protected:

	// For use with CNodeAttribute::Flags
	enum
	{
		Bone_Root = 0x0100,		// Root bone, which has no parent
		Bone_Term = 0x0200		// Terminator bone, which has no children
	};

	DWORD				Index;
	Math::CTransformSRT	BindPoseLocal;	//!!!Needed only on init!
	matrix44			BindPoseWorld;	//!!!Needed only on init!
	matrix44			InvBindPose;
	matrix44			SkinMatrix;

	virtual bool	OnAttachToNode(CSceneNode* pSceneNode);
	virtual void	OnDetachFromNode();

public:

	virtual bool	LoadDataBlock(Data::CFourCC FourCC, IO::CBinaryReader& DataReader);
	virtual void	Update();

	//!!!Can be useful for ragdolls!
	//void			GetGlobalAABB(CAABB& OutBox) const;

	DWORD			GetIndex() const { return Index; }
	bool			IsRoot() const { return Flags.Is(Bone_Root); }
	bool			IsTerminal() const { return Flags.Is(Bone_Term); }
	const matrix44&	GetBindPoseMatrix() const { return BindPoseWorld; }
	const matrix44&	GetInvBindPoseMatrix() const { return InvBindPose; }

	//!!!FindChildBone(Name)! // if it is term ret null
	CBone*			GetParentBone() const;
	CBone*			GetRootBone();
};

typedef Ptr<CBone> PBone;

}

#endif
