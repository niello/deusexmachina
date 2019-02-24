#pragma once
#include <CEGUI/RenderQueue.h>
#include <Math/Matrix44.h>

namespace CEGUI
{
class CDEMRenderer;

template <typename T>
class CDEMRenderTarget: public T
{
protected:

	CDEMRenderer&		d_owner;
	Rectf				d_area;
	mutable matrix44	d_matrix;
	mutable bool		d_matrixValid;
	mutable float		d_viewDistance; // is set up at the same time as d_matrix

	void updateMatrix() const;

public:

	CDEMRenderTarget(CDEMRenderer& owner);

	// implement parts of RenderTarget interface
	virtual void			draw(const GeometryBuffer& buffer/*, uint32 drawModeMask = DrawModeMaskAll*/);
	virtual void			draw(const RenderQueue& queue/*, uint32 drawModeMask = DrawModeMaskAll*/) { queue.draw(/*drawModeMask*/); }
	virtual void			setArea(const Rectf& area);
	virtual const Rectf&	getArea() const { return d_area; }
	virtual void			activate();
	virtual void			deactivate() {}
	virtual void			unprojectPoint(const GeometryBuffer& buff, const glm::vec2& p_in, glm::vec2& p_out) const;
};

}
