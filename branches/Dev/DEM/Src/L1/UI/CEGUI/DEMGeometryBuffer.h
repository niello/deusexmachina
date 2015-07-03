#pragma once
#ifndef __DEM_L1_CEGUI_GEOMETRY_BUFFER_H__
#define __DEM_L1_CEGUI_GEOMETRY_BUFFER_H__

#include <CEGUI/GeometryBuffer.h>
#include <Math/Matrix44.h>
#include <Data/RefCounted.h>
#include <Data/Array.h>
#include <CEGUI/Quaternion.h>

namespace Render
{
	typedef Ptr<class CVertexBuffer> PVertexBuffer;
	typedef Ptr<class CTexture> PTexture;
}

namespace CEGUI
{
class CDEMRenderer;
class CDEMTexture;

class CDEMGeometryBuffer: public GeometryBuffer
{
protected:

	struct D3DVertex
	{
		FLOAT x, y, z; // Transformed
		DWORD diffuse;
		float tu, tv;
	};

	struct BatchInfo
	{
		Render::PTexture	texture;
		uint				vertexCount;
		bool				clip;
	};

	CDEMRenderer&					d_owner;
	CDEMTexture*					d_activeTexture;
	mutable Render::PVertexBuffer	d_vertexBuffer;
	mutable UINT					d_bufferSize;
	mutable bool					d_bufferIsSync;
	CArray<BatchInfo>				d_batches;
	CArray<D3DVertex>				d_vertices;
	Rectf							d_clipRect;
	bool							d_clippingActive;
	RenderEffect*					d_effect;
	mutable matrix44				d_matrix;
	mutable bool					d_matrixValid;

	Vector3f						d_translation;
	Quaternion						d_rotation;
	Vector3f						d_pivot;

	void updateMatrix() const;
	void syncHardwareBuffer() const;

public:

	CDEMGeometryBuffer(CDEMRenderer& owner);
	//virtual ~CDEMGeometryBuffer() { }

	const matrix44*			getMatrix() const;

	// Implement GeometryBuffer interface.
	virtual void			draw() const;
	virtual void			setTranslation(const Vector3f& v) { d_translation = v; d_matrixValid = false; }
	virtual void			setRotation(const Quaternion& r) { d_rotation = r; d_matrixValid = false; }
	virtual void			setPivot(const Vector3f& p) { d_pivot = p; d_matrixValid = false; }
	virtual void			setClippingRegion(const Rectf& region);
	virtual void			appendVertex(const Vertex& vertex) { appendGeometry(&vertex, 1); }
	virtual void			appendGeometry(const Vertex* const vbuff, uint vertex_count);
	virtual void			setActiveTexture(Texture* texture);
	virtual void			reset();
	virtual Texture*		getActiveTexture() const;
	virtual uint			getVertexCount() const { return d_vertices.GetCount(); }
	virtual uint			getBatchCount() const { return d_batches.GetCount(); }
	virtual void			setRenderEffect(RenderEffect* effect) { d_effect = effect; }
	virtual RenderEffect*	getRenderEffect() { return d_effect; }
	virtual void			setClippingActive(const bool active) { d_clippingActive = active; }
	virtual bool			isClippingActive() const { return d_clippingActive; }
};

}

#endif