#pragma once
#ifndef __DEM_L1_QUADTREE_H__
#define __DEM_L1_QUADTREE_H__

#include <mathlib/bbox.h>
#include <Data/FixedArray.h>

#ifdef GetObject
#undef GetObject
#endif

//???write loose quadtree? Current variant is not so good.
//???create child nodes on demand, using node pool?

// Template quadtree spatial partitioning structure.
// Define TObject and TStorage classes to get it working.
// TObject represents element placed in quadtree, TStorage stores objects in a node,
// it can be linked list node, array or smth.

// TObject interface:
// - void	GetCenter(vector2& Out) const;
// - void	GetHalfSize(vector2& Out) const;
// - CNode*	GetQuadTreeNode() const; //???to some wrapper class?
// - void	SetQuadTreeNode(CNode*);

// TStorage interface:
// - typedef CElement (for array - TObject, for linked list - list node etc)
// - TObject&	CElement::GetObject(); // or TObject CElement::GetObject();
// - CElement*	Add(const TObject& Object);
// - void		RemoveByValue(const TObject& Object);
// - void		RemoveElement(CElement* pElement);

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
		typedef T& Ref;
		static T*		GetPtr(T& Object) { return &Object; }
		static const T*	GetPtr(const T& Object) { return &Object; }
		static T&		GetRef(T& Object) { return Object; }
		static bool		IsValid(const T& Object) { OK; }
	};

	template <class T>
	struct ObjTraits<T*>
	{
		typedef T& Ref;
		static T*		GetPtr(T* Object) { return Object; }
		static const T*	GetPtr(const T* Object) { return Object; }
		static T&		GetRef(T* const& Object) { return *Object; }
		static bool		IsValid(const T* Object) { return Object != NULL; }
	};

	template <class T, template <class> class SP>
	struct ObjTraits<SP<T>>
	{
		typedef T& Ref;
		static T*		GetPtr(T* Object) { return Object; }
		static const T*	GetPtr(const T* Object) { return Object; }
		static T&		GetRef(T* const& Object) { return *Object; }
		static bool		IsValid(const T* Object) { return Object.IsValid(); }
	};

	typedef ObjTraits<TObject> TObjTraits;

	vector2				Center;
	vector2				Size;
	uchar				Depth;
	CFixedArray<CNode>	Nodes;

	void	Build(ushort Col, ushort Row, uchar Level, CNode* pNode, CNode*& pFirstFreeNode);
	CNode*	FindContainingNode(const TObject& Object) const;

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
		void		RemoveElement(CElement* pElement);

	public:

		TStorage						Data;

		CNode(): TotalObjCount(0) {}

		bool		Contains(const TObject& Object) const;
		bool		Contains(const vector2& Center, const vector2& HalfSize) const;
		bool		SharesSpaceWith(const CNode& Other) const;
		void		GetBounds(bbox3& Box) const;

		uchar		GetLevel() const { return Level; }
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
	void		UpdateElement(CElement*& pElement);
	void		RemoveObject(TObject& Object) { TObjTraits::GetPtr(Object)->GetQuadTreeNode()->RemoveObject(Object); }
	void		RemoveElement(CElement* pElement) { TObjTraits::GetPtr(pElement->GetObject())->GetQuadTreeNode()->RemoveElement(pElement); }

	//!!!Test against circle & 2d box

	const vector2& GetSize() const { return Size; }
};
//---------------------------------------------------------------------

template<class TObject, class TStorage>
void CQuadTree<TObject, TStorage>::Build(float CenterX, float CenterZ, float SizeX, float SizeZ, uchar TreeDepth)
{
	n_assert(!Nodes.GetCount() && SizeX > 0.f && SizeZ > 0.f && TreeDepth > 0);

	Center.x = CenterX;
	Center.y = CenterZ;
	Size.x = SizeX;
	Size.y = SizeZ;
	Depth = TreeDepth;

	Nodes.SetSize(0x55555555 & ((1 << (Depth << 1)) - 1));

	for (DWORD i = 0; i < Nodes.GetCount(); ++i) Nodes[i].pOwner = this;

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
typename CQuadTree<TObject, TStorage>::CNode* CQuadTree<TObject, TStorage>::FindContainingNode(const TObject& Object) const
{
	vector2 Center, HalfSize;
	TObjTraits::GetPtr(Object)->GetCenter(Center);
	TObjTraits::GetPtr(Object)->GetHalfSize(HalfSize);

	CNode* pCurrNode =TObjTraits::GetPtr(Object)->GetQuadTreeNode();
	CNode* pNewNode = pCurrNode;

	while (pNewNode->pParent && !pNewNode->Contains(Center, HalfSize))
	{
		--pNewNode->TotalObjCount;
		pNewNode = pNewNode->pParent;
	}

	while (pNewNode->pChild)
	{
		DWORD i;
		for (i = 0; i < 4; ++i)
			if (pNewNode->pChild[i].Contains(Center, HalfSize))
			{
				pNewNode = pNewNode->pChild + i;
				++pNewNode->TotalObjCount;
				break;
			}

		if (i == 4) break; // No child contains the object, add right to this node
	}

	return pNewNode;
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
typename CQuadTree<TObject, TStorage>::CElement* CQuadTree<TObject, TStorage>::UpdateObject(TObject& Object)
{
	CNode* pCurrNode = TObjTraits::GetPtr(Object)->GetQuadTreeNode();
	CNode* pNewNode = FindContainingNode(Object);

	if (pCurrNode == pNewNode) return NULL; //???return pCurrNode->Data.GetElementByValue(Object);?

	TObjTraits::GetPtr(Object)->SetQuadTreeNode(pNewNode);
	CElement* pNewElm = pNewNode->Data.Add(Object);
	pCurrNode->Data.RemoveByValue(Object);		
	return pNewElm;
}
//---------------------------------------------------------------------

// NB: pElement is in-out arg
template<class TObject, class TStorage>
void CQuadTree<TObject, TStorage>::UpdateElement(typename CQuadTree<TObject, TStorage>::CElement*& pElement)
{
	//???is there a better way?
	const TObject& Object = pElement->GetObject();
	TObjTraits::Ref ObjRef = TObjTraits::GetRef(pElement->GetObject());

	CNode* pCurrNode = ObjRef.GetQuadTreeNode();
	CNode* pNewNode = FindContainingNode(Object);

	if (pCurrNode != pNewNode)
	{
		ObjRef.SetQuadTreeNode(pNewNode);
		CElement* pNewElm = pNewNode->Data.Add(Object);
		pCurrNode->Data.RemoveElement(pElement);		
		pElement = pNewElm;
	}
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
inline typename CQuadTree<TObject, TStorage>::CElement* CQuadTree<TObject, TStorage>::CNode::AddObject(TObject& Object)
{
	vector2 Center, HalfSize;
	TObjTraits::GetPtr(Object)->GetCenter(Center);
	TObjTraits::GetPtr(Object)->GetHalfSize(HalfSize);
	return AddObject(Object, Center, HalfSize);
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
typename CQuadTree<TObject, TStorage>::CElement* CQuadTree<TObject, TStorage>::CNode::AddObject(TObject& Object,
																								const vector2& Center,
																								const vector2& HalfSize)
{
	++TotalObjCount;

	if (pChild)
		for (DWORD i = 0; i < 4; i++)
			if (pChild[i].Contains(Center, HalfSize))
				return pChild[i].AddObject(Object, Center, HalfSize);

	TObjTraits::GetPtr(Object)->SetQuadTreeNode(this);
	return Data.Add(Object);
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
inline void CQuadTree<TObject, TStorage>::CNode::RemoveObject(TObject& Object)
{
	TObjTraits::GetPtr(Object)->SetQuadTreeNode(NULL);
	Data.RemoveByValue(Object);
	CNode* pNode = this;
	while (pNode)
	{
		--pNode->TotalObjCount;
		pNode = pNode->pParent;
	}
}
//---------------------------------------------------------------------

template<class TObject, class TStorage>
inline void CQuadTree<TObject, TStorage>::CNode::RemoveElement(typename CQuadTree<TObject, TStorage>::CElement* pElement)
{
	TObjTraits::GetPtr(pElement->GetObject())->SetQuadTreeNode(NULL);
	Data.RemoveElement(pElement);
	CNode* pNode = this;
	while (pNode)
	{
		--pNode->TotalObjCount;
		pNode = pNode->pParent;
	}
}
//---------------------------------------------------------------------

// Returns true if node contains the whole object (not only some its part!)
template<class TObject, class TStorage>
inline bool CQuadTree<TObject, TStorage>::CNode::Contains(const TObject& Object) const
{
	vector2 Center, HalfSize;
	TObjTraits::GetPtr(Object)->GetCenter(Center);
	TObjTraits::GetPtr(Object)->GetHalfSize(HalfSize);
	return Contains(Center, HalfSize);
}
//---------------------------------------------------------------------

// Returns true if node contains the whole object (not only some its part!)
template<class TObject, class TStorage>
inline bool CQuadTree<TObject, TStorage>::CNode::Contains(const vector2& Center, const vector2& HalfSize) const
{
	bbox3 Box;
	GetBounds(Box);
	return	Center.x - HalfSize.x >= Box.vmin.x &&
			Center.x + HalfSize.x <= Box.vmax.x &&
			Center.y - HalfSize.y >= Box.vmin.z &&
			Center.y + HalfSize.y <= Box.vmax.z;
}
//---------------------------------------------------------------------

// Returns true if nodes share common space (intersect)
template<class TObject, class TStorage>
inline bool CQuadTree<TObject, TStorage>::CNode::SharesSpaceWith(const CNode& Other) const
{
	if (this == &Other) OK;
	if (Level == Other.Level) FAIL;

	// If not the same node and not at equal levels, check if one node is a N-level parent of another
	const Scene::CSPSNode* pMin;
	const Scene::CSPSNode* pMax;
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

	while (pMax->Level > pMin->Level)
		pMax = pMax->pParent;

	return pMax == pMin;
}
//---------------------------------------------------------------------

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
