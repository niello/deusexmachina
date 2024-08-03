#pragma once
#include <CEGUI/GeometryBuffer.h>
#include <Render/RenderFwd.h>

namespace CEGUI
{
class CDEMRenderer;

class CDEMGeometryBuffer: public GeometryBuffer
{
protected:

	CDEMRenderer&					d_owner;
	Render::PVertexLayout			d_vertexLayout;
	mutable Render::PVertexBuffer	d_vertexBuffer;
	mutable UPTR					d_bufferSize = 0;
	mutable bool					d_bufferIsSync = false;
	mutable glm::mat4				d_matrix;

	virtual void onGeometryChanged() override { d_bufferIsSync = false; }

public:

	CDEMGeometryBuffer(CDEMRenderer& owner, RefCounted<RenderMaterial> renderMaterial);

	void setVertexLayout(Render::PVertexLayout newLayout);

	// Implement GeometryBuffer interface.
	virtual void draw(std::uint32_t drawModeMask = DrawModeMaskAll) const override;
};

}
