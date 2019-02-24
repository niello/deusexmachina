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
	mutable Render::PVertexBuffer	d_vertexBuffer;
	mutable UPTR					d_bufferSize = 0;
	mutable bool					d_bufferIsSync = false;
	mutable glm::mat4				d_matrix;

public:

	CDEMGeometryBuffer(CDEMRenderer& owner, RefCounted<RenderMaterial> renderMaterial);
	//virtual ~CDEMGeometryBuffer() { }

	// Implement GeometryBuffer interface.
	virtual void			draw(/*uint32 drawModeMask = DrawModeMaskAll*/) const override;
	virtual void			appendGeometry(const float* vertex_data, std::size_t array_size) override;
};

}
