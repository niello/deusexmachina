#pragma once
#ifndef __DEM_L1_QUADTREE_H__
#define __DEM_L1_QUADTREE_H__

#include <mathlib/bbox.h>
#include <util/nfixedarray.h>

//???write loose quadtree? Current variant is not so good.

// Template quadtree spatial partitioning structure.
// Define TObject and TStorage classes to get it working.
// TObject represents element placed in quadtree, TStorage stores objects in a node,
// it can be linked list node, array or smth.

// TObject interface:
// - const vector2& GetCenter() const;
// - const vector2& GetHalfSize() const;
// - CNode* GetQuadTreeNode() const; //???to some wrapper class?
// - void SetQuadTreeNode(CNode*);

// TStorage interface:
// - typedef CElement (for array - TObject, for linked list - list node etc)
// - CElement*	Add(const TObject& Object);
// - void		Remove(const TObject& Object);

namespace Data
{

template<class TObject, class TStorage>
class CQuadTree
{
public:

	class CNode;

	typedef typename TStorage::CElement CElement;

protected:

	template <class T>
	struct ObjTraits
	{
		static void		GetCenter(const T& Object, vector2& Out) { return Object.GetCenter(Out); }
		static void		GetHalfSize(T& Object, vector2& Out) { return Object.GetHalfSize(Out); }
		static CNode*	GetQuadTreeNode(const T& Object) { return Object.GetQuadTreeNode(); }
		static void		SetQuadTreeNode(T& Object, CNode* pNode) { Object.SetQuadTreeNode(pNode); }
		//???or just redefine member access operator?
	};

	template <class T>
	struct ObjTraits<T*>
	{
		static void		GetCenter(const T* Object, vector2& Out) { return Object->GetCenter(Out); }
		static void		GetHalfSize(T* Object, vector2& Out) { return Object->GetHalfSize(Out); }
		static CNode*	GetQuadTreeNode(const T* Object) { return Object->GetQuadTreeNode(); }
		static void		SetQuadTreeNode(T* Object, CNode* pNode) { Object->SetQuadTreeNode(pNode); }
		//???or just redefine member access operator?
	};

	vector2				Center;
	vector2				Size;
	uchar				Depth;
	nFixedArray<CNode>	Nodes;

	void	Build(ushort Col, ushort Row, uchar Level, CNode* pNode, CNode*& pFirstFreeNode);
	CNode*	UpdateObjectCommon(TObject& Object);

public:

	class CNode
	{
	private:

		DWORD							TotalObjCount;	// Total object count inside this node & it's hierarchy

		CQuadTree<TObject, TStorage>*	pOwner; //!!!now only for size, 2 floats!
		CNode*							pParent;
		CNode*							pChild;			// Pointer to first element of CNode[4]

		ushort							Col;
		ushort							Row;
		uchar							Level;

		template<class TObject, class TStorage> friend class CQuadTree;
		
		CElement*	AddObject(TObject& Object);
		CElement*	AddObject(TObject& Object, const vector2& Center, const vector2& HalfSize);
		void		RemoveObject(TObject& Object);
		void		RemoveObject(CElement* pElement);

	public:

		TStorage						Data;

		CNode(): TotalObjCount(0) {}

		bool		Contains(const TObject& Object) const;
		bool		Contains(const vector2& Center, const vector2& HalfSize) const;
		void		GetBounds(bbox3& Box) const;

		CNode*		GetParent() const { return pParent; }
		CNode*		GetChild(DWORD Index) const { n_assert(Index < 4); return pChild + Index; }
		DWORD		GetTotalObjCount() const { return TotalObjCount; }
		bool		HasChildren() const { return pChild != NULL; }
	};

	CQuadTree() {}
	CQuadTree(float CenterX, float CenterZ, float SizeX, float SizeZ, uchar TreeDepth) { Build(CenterX, CenterZ, SizeX, SizeZ, TreeDepth); }

	void		Build(float CenterX, float CenterZ, float SizeX, float SizeZ, uchar TreeDepth);

	CNode*		GetNode(ushort Col, ushort Row, uchar Level);
	CNode*		GetRootNode() { return &Nodes[0]; }

	CElement*	AddObject(TObject& Object) { return Nodes[0].AddObject(Object); }
	CElement*	UpdateObject(TObject& Object);
	void		UpdateObject(CElement*& pElement);
	void		RemoveObject(TObject& Object) { ObjTraits<TObject>::GetQuadTreeNode(Object)->RemoveObject(Object); }
	void		RemoveObject(CElement* pElement) { ObjTraits<TObject>::GetQuadTreeNode(pElement->Object)->RemoveObject(pElement); }

	//!!!Test against circle & 2d box

	const vector2& GetSize() const { return Size; }
};
//---------------------------------------------------------------------

template<class TObject, class TStorage>
void CQuadTree<TObject, TStorage>::Build(float CenterX, float CenterZ, float SizeX, float SizeZ, uchar TreeDepth)
{
	n_assert(!Nodes.Size() && SizeX > 0.f && SizeZ > 0.f && TreeDepth > 0);

	Center.x = CenterX;
	Center.y = CenterZ;
	Size.x = SizeX;
	Size.y = SizeZ;
	Depth = TreeDepth;

	Nodes.SetSize(0x55555555 & ((1 << (Depth << 1)) - 1));

	for (int i = 0; i < Nodes.Size(); i++) Nodes[i].pOwner = this;

	CNode* pFirstFreeNode = &Nodes[1];
	Nodes[0].pParent = NULL;
	Build(0, 0, 0, &Nodes[0], pFirstFreeNode);
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
void CQuadTree<TObject, TStorage>::Build(ushort Col, ushort Row, uchar Level, CNode* pNode, CNode*& pFirstFreeNode)
{
	pNode->Col = Col;
	pNode->Row = Row;
	pNode->Level = Level;

	if (Level < Depth - 1)
	{
		pNode->pChild = pFirstFreeNode;
		pFirstFreeNode += 4;

		for (DWORD i = 0; i < 4; i++)
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

template<class TObject, class TStorage>
inline typename CQuadTree<TObject, TStorage>::CNode* CQuadTree<TObject, TStorage>::GetNode(ushort Col, ushort Row, uchar Level)
{
	// Don't ask. Just believe.
	n_assert(Level < Depth);
	return &Nodes[(0x55555555 & ((1 << (Level << 1)) - 1)) +
		(((1 << (Level - 1)) * (Row >> 1) + (Col >> 1)) << 2) +
		(1 & Col) + ((1 & Row) << 1)];
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
typename CQuadTree<TObject, TStorage>::CNode* CQuadTree<TObject, TStorage>::UpdateObjectCommon(TObject& Object)
{
	vector2 Center, HalfSize;
	ObjTraits<TObject>::GetCenter(Object, Center);
	ObjTraits<TObject>::GetHalfSize(Object, HalfSize);
	
	CNode* pCurrNode = ObjTraits<TObject>::GetQuadTreeNode(Object);
	CNode* pNewNode = pCurrNode;

	while (pNewNode->pParent && !pNewNode->Contains(Center, HalfSize))
	{
		pNewNode->TotalObjCount--;
		pNewNode = pNewNode->pParent;
	}

	while (pNewNode->pChild)
	{
		DWORD i;
		for (i = 0; i < 4; ++i)
			if (pNewNode->pChild[i].Contains(Center, HalfSize))
			{
				pNewNode = pNewNode->pChild + i;
				pNewNode->TotalObjCount++;
				break;
			}

		if (i == 4) break;
	}

	return pNewNode;
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
typename CQuadTree<TObject, TStorage>::CElement* CQuadTree<TObject, TStorage>::UpdateObject(TObject& Object)
{
	CNode* pCurrNode = ObjTraits<TObject>::GetQuadTreeNode(Object);
	CNode* pNewNode = UpdateObjectCommon(Object);

	if (pCurrNode != pNewNode)
	{
		// Here Object can die if it isn't a pointer
		// If so, try this:
		// TObject NewObj = Object;
		// and then operate with NewObj. It causes data copying anyway.

		pCurrNode->Data.Remove(Object);		
		ObjTraits<TObject>::SetQuadTreeNode(Object, pNewNode);
		return pNewNode->Data.Add(Object);
	}

	return NULL;
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
void CQuadTree<TObject, TStorage>::UpdateObject(typename CQuadTree<TObject, TStorage>::CElement*& pElement)
{
	TObject Object = pElement->Object;

	CNode* pCurrNode = ObjTraits<TObject>::GetQuadTreeNode(Object);
	CNode* pNewNode = UpdateObjectCommon(Object);

	if (pCurrNode != pNewNode)
	{
		pCurrNode->Data.Remove(pElement);		
		ObjTraits<TObject>::SetQuadTreeNode(Object, pNewNode);
		pElement = pNewNode->Data.Add(Object);
	}
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
inline typename CQuadTree<TObject, TStorage>::CElement* CQuadTree<TObject, TStorage>::CNode::AddObject(TObject& Object)
{
	vector2 Center, HalfSize;
	ObjTraits<TObject>::GetCenter(Object, Center);
	ObjTraits<TObject>::GetHalfSize(Object, HalfSize);
	return AddObject(Object, Center, HalfSize);
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
typename CQuadTree<TObject, TStorage>::CElement* CQuadTree<TObject, TStorage>::CNode::AddObject(TObject& Object,
																								const vector2& Center,
																								const vector2& HalfSize)
{
	TotalObjCount++;

	if (pChild)
		for (DWORD i = 0; i < 4; i++)
			if (pChild[i].Contains(Center, HalfSize))
				return pChild[i].AddObject(Object, Center, HalfSize);

	ObjTraits<TObject>::SetQuadTreeNode(Object, this);
	return Data.Add(Object);
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
inline void CQuadTree<TObject, TStorage>::CNode::RemoveObject(TObject& Object)
{
	ObjTraits<TObject>::SetQuadTreeNode(Object, NULL);
	Data.Remove(Object);
	CNode* pNode = this;
	while (pNode)
	{
		pNode->TotalObjCount--;
		pNode = pNode->pParent;
	}
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
inline void CQuadTree<TObject, TStorage>::CNode::RemoveObject(typename CQuadTree<TObject, TStorage>::CElement* pElement)
{
	ObjTraits<TObject>::SetQuadTreeNode(pElement->Object, NULL);
	Data.Remove(pElement);
	CNode* pNode = this;
	while (pNode)
	{
		pNode->TotalObjCount--;
		pNode = pNode->pParent;
	}
}
//---------------------------------------------------------------------

// Returns true if node contains the whole object (not only some its part!)
template<class TObject, class TStorage>
inline bool CQuadTree<TObject, TStorage>::CNode::Contains(const TObject& Object) const
{
	vector2 Center, HalfSize;
	ObjTraits<TObject>::GetCenter(Object, Center);
	ObjTraits<TObject>::GetHalfSize(Object, HalfSize);
	return Contains(Center, HalfSize);
}
//---------------------------------------------------------------------

// Returns true if node contains the whole object (not only some its part!)
template<class TObject, class TStorage>
bool CQuadTree<TObject, TStorage>::CNode::Contains(const vector2& Center, const vector2& HalfSize) const
{
	bbox3 Box;
	GetBounds(Box);
	return	Center.x - HalfSize.x >= Box.vmin.x &&
			Center.x + HalfSize.x <= Box.vmax.x &&
			Center.y - HalfSize.y >= Box.vmin.z &&
			Center.y + HalfSize.y <= Box.vmax.z;
}
//---------------------------------------------------------------------

// Returns true if node contains the whole object (not only some its part!)
template<class TObject, class TStorage>
void CQuadTree<TObject, TStorage>::CNode::GetBounds(bbox3& Box) const
{
	float NodeSizeX, NodeSizeZ;
	
	if (Level > 0)
	{
		float SizeQeff = 1.f / (1 << Level);
		NodeSizeX = pOwner->GetSize().x * SizeQeff;
		NodeSizeZ = pOwner->GetSize().y * SizeQeff;
	}
	else
	{
		NodeSizeX = pOwner->GetSize().x;
		NodeSizeZ = pOwner->GetSize().y;
	}

	Box.vmin.x = pOwner->Center.x + Col * NodeSizeX - pOwner->GetSize().x * 0.5f;
	Box.vmin.z = pOwner->Center.y + Row * NodeSizeZ - pOwner->GetSize().y * 0.5f;
	Box.vmax.x = Box.vmin.x + NodeSizeX;
	Box.vmax.z = Box.vmin.z + NodeSizeZ;
}
//---------------------------------------------------------------------

};

#endif
