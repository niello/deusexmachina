#pragma once
#ifndef __DEM_L1_QUADTREE_H__
#define __DEM_L1_QUADTREE_H__

#include <Math/AABB.h>
#include <Data/FixedArray.h>

// Template quadtree spatial partitioning structure.
// Define TObject and TStorage classes to get it working.
// TObject represents element placed in a quadtree.
// TStorage stores that objects in a quadtree node. It can be a linked list, an array or smth.

// TStorage interface requirements:
// - typedef CIterator (defined as CHandle by a quadtree, is not necessary to use, but can be used for faster lookup in a storage)
// - CIterator			Add(const TObject& Object); or (TObject Object)
// - CIterator			Find(const TObject& Object); or (TObject Object)
// - any-return-type	Remove(CIterator It);
// - CIterator			End()

//!!!???write loose quadtree!? Current variant is not so good.
//???create child nodes on demand, using node pool?

namespace Data
{

template<class TObject, class TStorage>
class CQuadTree
{
public:

	class CNode;

	typedef typename TStorage::CIterator CHandle;

protected:

	vector2				Center;
	vector2				Size;
	U8					Depth;
	CFixedArray<CNode>	Nodes;

	static CHandle	MoveElement(TStorage& From, TStorage& To, CHandle Handle);

	void			Build(U16 Col, U16 Row, U8 Level, CNode* pNode, CNode*& pFirstFreeNode);
	CNode*			RelocateAndUpdateCounters(CNode* pCurrNode, float CenterX, float CenterZ, float HalfSizeX, float HalfSizeZ) const;

public:

	class CNode
	{
	protected:

		UPTR							TotalObjCount;	// Total object count inside this node & its hierarchy

		CQuadTree<TObject, TStorage>*	pOwner;			// Now used only for getting position and size
		CNode*							pParent;
		CNode*							pChild;			// Pointer to the first element of CNode[4]

		U16								Col;
		U16								Row;
		U8								Level;

		template<class TObject, class TStorage> friend class CQuadTree;

	public:

		TStorage						Data;

		CNode(): TotalObjCount(0) {}
		
		void	RemoveByValue(const TObject& Object) { RemoveByHandle(Data.Find(Object)); }
		void	RemoveByHandle(CHandle Handle);

		bool	Contains(float CenterX, float CenterZ, float HalfSizeX, float HalfSizeZ) const;
		bool	SharesSpaceWith(const CNode& Other) const;
		void	GetBounds(CAABB& Box) const;

		U8		GetLevel() const { return Level; }
		CNode*	GetParent() const { return pParent; }
		CNode*	GetChild(UPTR Index) const { n_assert(Index < 4); return pChild + Index; }
		UPTR	GetTotalObjCount() const { return TotalObjCount; }
		bool	HasChildren() const { return pChild != NULL; }

		//CQuadTree<TObject, TStorage>*	GetOwner() { return pOwner; }
	};

	CQuadTree() {}
	CQuadTree(float CenterX, float CenterZ, float SizeX, float SizeZ, U8 TreeDepth) { Build(CenterX, CenterZ, SizeX, SizeZ, TreeDepth); }

	void			Build(float CenterX, float CenterZ, float SizeX, float SizeZ, U8 TreeDepth);
	void			AddObject(const TObject& Object, float CenterX, float CenterZ, float HalfSizeX, float HalfSizeZ, CNode*& pOutNode, CHandle* pOutHandle = NULL);
	void			UpdateObject(const TObject& Object, float CenterX, float CenterZ, float HalfSizeX, float HalfSizeZ, CNode*& pInOutNode, CHandle* pOutHandle = NULL);
	void			UpdateHandle(CHandle& InOutHandle, float CenterX, float CenterZ, float HalfSizeX, float HalfSizeZ, CNode*& pInOutNode);

	CNode*			GetNode(UPTR Col, UPTR Row, UPTR Level) const;
	CNode*			GetRootNode() const { return &Nodes[0]; }
	const vector2&	GetSize() const { return Size; }

	//!!!Test against circle (center, r) & 2d box (two corners or center & half)
	//???or visitors?
};
//---------------------------------------------------------------------

template<class TObject, class TStorage>
inline void CQuadTree<TObject, TStorage>::CNode::RemoveByHandle(typename CQuadTree<TObject, TStorage>::CHandle Handle)
{
	if (Handle == Data.End()) return;
	Data.Remove(Handle);
	CNode* pNode = this;
	while (pNode)
	{
		--pNode->TotalObjCount;
		pNode = pNode->pParent;
	}
}
//---------------------------------------------------------------------

// Returns true if node contains the whole object (not partially!)
template<class TObject, class TStorage>
inline bool CQuadTree<TObject, TStorage>::CNode::Contains(float CenterX, float CenterZ, float HalfSizeX, float HalfSizeZ) const
{
	CAABB Box;
	GetBounds(Box); //???or better to cache box XZ?
	return	CenterX - HalfSizeX >= Box.Min.x &&
			CenterX + HalfSizeX <= Box.Max.x &&
			CenterZ - HalfSizeZ >= Box.Min.z &&
			CenterZ + HalfSizeZ <= Box.Max.z;
}
//---------------------------------------------------------------------

// Returns true if nodes share common space (intersect)
template<class TObject, class TStorage>
inline bool CQuadTree<TObject, TStorage>::CNode::SharesSpaceWith(const CNode& Other) const
{
	if (this == &Other) OK;
	if (Level == Other.Level) FAIL;

	// If not the same node and not at equal levels, check if one node is a N-level parent of another
	const CQuadTree<TObject, TStorage>::CNode* pMin;
	const CQuadTree<TObject, TStorage>::CNode* pMax;
	if (Level < Other.Level)
	{
		pMin = this;
		pMax = &Other;
	}
	else
	{
		pMin = &Other;
		pMax = this;
	}

	U8 LevelDiff = pMax->Level - pMin->Level;
	while (LevelDiff)
	{
		pMax = pMax->pParent;
		--LevelDiff;
	}

	return pMax == pMin;
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
void CQuadTree<TObject, TStorage>::CNode::GetBounds(CAABB& Box) const
{
	float NodeSizeX, NodeSizeZ;

	const vector2& RootSize = pOwner->GetSize();

	if (Level > 0)
	{
		float SizeCoeff = 1.f / (1 << Level);
		NodeSizeX = RootSize.x * SizeCoeff;
		NodeSizeZ = RootSize.y * SizeCoeff;
	}
	else
	{
		NodeSizeX = RootSize.x;
		NodeSizeZ = RootSize.y;
	}

	Box.Min.x = pOwner->Center.x + Col * NodeSizeX - RootSize.x * 0.5f;
	Box.Min.z = pOwner->Center.y + Row * NodeSizeZ - RootSize.y * 0.5f;
	Box.Max.x = Box.Min.x + NodeSizeX;
	Box.Max.z = Box.Min.z + NodeSizeZ;
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
void CQuadTree<TObject, TStorage>::Build(float CenterX, float CenterZ, float SizeX, float SizeZ, U8 TreeDepth)
{
	n_assert(!Nodes.GetCount() && SizeX > 0.f && SizeZ > 0.f && TreeDepth > 0);

	Center.x = CenterX;
	Center.y = CenterZ;
	Size.x = SizeX;
	Size.y = SizeZ;
	Depth = TreeDepth;

	Nodes.SetSize(0x55555555 & ((1 << (Depth << 1)) - 1));

	for (UPTR i = 0; i < Nodes.GetCount(); ++i) Nodes[i].pOwner = this;

	CNode* pFirstFreeNode = &Nodes[1];
	Nodes[0].pParent = NULL;
	Build(0, 0, 0, &Nodes[0], pFirstFreeNode);
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
void CQuadTree<TObject, TStorage>::Build(U16 Col, U16 Row, U8 Level, CNode* pNode, CNode*& pFirstFreeNode)
{
	pNode->Col = Col;
	pNode->Row = Row;
	pNode->Level = Level;

	if (Level < Depth - 1)
	{
		pNode->pChild = pFirstFreeNode;
		pFirstFreeNode += 4;

		for (UPTR i = 0; i < 4; i++)
			pNode->pChild[i].pParent = pNode;

		Col <<= 1;
		Row <<= 1;
		++Level;

		Build(Col,		Row,		Level, pNode->pChild,		pFirstFreeNode);
		Build(Col + 1,	Row,		Level, pNode->pChild + 1,	pFirstFreeNode);
		Build(Col,		Row + 1,	Level, pNode->pChild + 2,	pFirstFreeNode);
		Build(Col + 1,	Row + 1,	Level, pNode->pChild + 3,	pFirstFreeNode);
	}
	else pNode->pChild = NULL;
}
//---------------------------------------------------------------------

// Returns a node that contains given dimensions and updates counters likewise one object is moved from the
// current node to the new one. If object is added (not moved), root node should be passed as a current node.
template<class TObject, class TStorage>
typename CQuadTree<TObject, TStorage>::CNode*
CQuadTree<TObject, TStorage>::RelocateAndUpdateCounters(typename CQuadTree<TObject, TStorage>::CNode* pCurrNode,
														float CenterX, float CenterZ, float HalfSizeX, float HalfSizeZ) const
{
	n_assert_dbg(pCurrNode);

	CNode* pNewNode = pCurrNode;

	while (pNewNode->pParent && !pNewNode->Contains(CenterX, CenterZ, HalfSizeX, HalfSizeZ))
	{
		--pNewNode->TotalObjCount;
		pNewNode = pNewNode->pParent;
	}

	while (pNewNode->pChild)
	{
		UPTR i;
		for (i = 0; i < 4; ++i)
			if (pNewNode->pChild[i].Contains(CenterX, CenterZ, HalfSizeX, HalfSizeZ))
			{
				pNewNode = pNewNode->pChild + i;
				++pNewNode->TotalObjCount;
				break;
			}

		if (i == 4) break; // No child contains the object, this node is the one we search for
	}

	return pNewNode;
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
inline void CQuadTree<TObject, TStorage>::AddObject(const TObject& Object, float CenterX, float CenterZ, float HalfSizeX, float HalfSizeZ,
													CNode*& pOutNode, CHandle* pOutHandle)
{
	pOutNode = RelocateAndUpdateCounters(GetRootNode(), CenterX, CenterZ, HalfSizeX, HalfSizeZ);
	CHandle Handle = pOutNode->Data.Add(Object);
	if (pOutHandle) *pOutHandle = Handle;
}
//---------------------------------------------------------------------

// Moves an element from one collection to another. This implementation is smart pointer safe and is suitable
// for a majority of collections. Some collections, like linked lists with a shared node pool, require another implementation.
template<class TObject, class TStorage>
inline typename CQuadTree<TObject, TStorage>::CHandle CQuadTree<TObject, TStorage>::MoveElement(TStorage& From, TStorage& To, CHandle Handle)
{
	CHandle NewHandle = To.Add(*Handle);
	From.Remove(Handle);		
	return NewHandle;
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
void CQuadTree<TObject, TStorage>::UpdateObject(const TObject& Object, float CenterX, float CenterZ, float HalfSizeX, float HalfSizeZ,
												CNode*& pInOutNode, typename CQuadTree<TObject, TStorage>::CHandle* pOutHandle)
{
	CNode* pCurrNode = pInOutNode;
	if (pCurrNode)
	{
		CHandle CurrHandle = pCurrNode->Data.Find(Object);
		if (CurrHandle == pCurrNode->Data.End()) return;
		CNode* pNewNode = RelocateAndUpdateCounters(pCurrNode, CenterX, CenterZ, HalfSizeX, HalfSizeZ);
		if (pNewNode == pCurrNode)
		{
			if (pOutHandle) *pOutHandle = CurrHandle;
		}
		else
		{
			CHandle NewHandle = MoveElement(pCurrNode->Data, pNewNode->Data, CurrHandle);
			pInOutNode = pNewNode;
			if (pOutHandle) *pOutHandle = NewHandle;
		}
	}
	else AddObject(Object, CenterX, CenterZ, HalfSizeX, HalfSizeZ, pInOutNode, pOutHandle);
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
void CQuadTree<TObject, TStorage>::UpdateHandle(typename CQuadTree<TObject, TStorage>::CHandle& InOutHandle,
												float CenterX, float CenterZ, float HalfSizeX, float HalfSizeZ, CNode*& pInOutNode)
{
	CNode* pNewNode = RelocateAndUpdateCounters(pInOutNode, CenterX, CenterZ, HalfSizeX, HalfSizeZ);

	if (pInOutNode != pNewNode)
	{
		CHandle NewHandle = MoveElement(pInOutNode->Data, pNewNode->Data, InOutHandle);
		pInOutNode = pNewNode;
		InOutHandle = NewHandle;
	}
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
inline typename CQuadTree<TObject, TStorage>::CNode* CQuadTree<TObject, TStorage>::GetNode(UPTR Col, UPTR Row, UPTR Level) const
{
	// Don't ask. Just believe.
	n_assert(Level < Depth);
	return &Nodes[(0x55555555 & ((1 << (Level << 1)) - 1)) +
		(((1 << (Level - 1)) * (Row >> 1) + (Col >> 1)) << 2) +
		(1 & Col) + ((1 & Row) << 1)];
}
//---------------------------------------------------------------------

};

#endif
