#pragma once
#ifndef __DEM_L1_RENDER_SHAPE_RENDERER_H__
#define __DEM_L1_RENDER_SHAPE_RENDERER_H__

#include <Render/Renderer.h>

// Renderer for debug shapes

//!!!use instancing per shape type, use one instance buffer with offset for shape types!

namespace Render
{

class CShapeRenderer: public IRenderer
{
public:

	//!!!void AddLineList/Strip! or as AddShape
	void			AddShape(/*type, transform etc, see currend and N3 renderers*/);

	/*
    void DeleteShapesByThreadId(Threading::ThreadId threadId); - called automatically after rendering
    void AddShape(const CoreGraphics::RenderShape& shape);
    void AddShapes(const Util::Array<CoreGraphics::RenderShape>& shapeArray);
    void AddWireFrameBox(const Math::bbox& boundingBox, const Math::float4& color, Threading::ThreadId threadId);

        Box,
        Sphere,
        Cylinder,
        Torus,
        Primitives,
        IndexedPrimitives,

    Threading::ThreadId threadId;
    Type shapeType;
    Math::matrix44 modelTransform;
    PrimitiveTopology::Code topology;
    SizeT numPrimitives;
    SizeT vertexWidth;
    SizeT numVertices;
    IndexType::Code indexType;
    Math::float4 color;
    IndexT vertexDataOffset;
    Ptr<IO::MemoryStream> dataStream;       // contains vertex/index data
	*/

	virtual void	AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects) { /*No acceptable scene node attrs*/ }
	virtual void	Render();
};

typedef Ptr<CShapeRenderer> PShapeRenderer;

}

#endif
