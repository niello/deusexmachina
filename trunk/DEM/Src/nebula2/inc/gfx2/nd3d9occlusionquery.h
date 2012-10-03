#ifndef N_D3D9OCCLUSIONQUERY_H
#define N_D3D9OCCLUSIONQUERY_H
//------------------------------------------------------------------------------
/**
    @class nD3D9OcclusionQuery
    @ingroup Gfx2

    Direct3D9 version of nOcclusionQuery.

    (C) 2005 Radon Labs GmbH
*/
#include "gfx2/nocclusionquery.h"
#include "util/narray.h"
#include "kernel/nref.h"
#include "gfx2/nd3d9server.h"

class nShader2;

//------------------------------------------------------------------------------
class nD3D9OcclusionQuery : public nOcclusionQuery
{
public:
    /// begin issuing occlusion queries
    virtual void Begin();
    /// add a bounding box query
    virtual void AddShapeQuery(nGfxServer2::ShapeType type, const matrix44& modelMatrix, const void* userData);
    /// finish issuing queries
    virtual void End();
    /// get number of issued queries
    virtual int GetNumQueries() const;
    /// get user data associated with a query
    virtual const void* GetUserData(int queryIndex);
    /// get occlusion status for a issued query (true is occluded)
    virtual bool GetOcclusionStatus(int queryIndex);
    /// clear all queries
    virtual void Clear();

private:
    friend class nD3D9Server;

    /// constructor
    nD3D9OcclusionQuery();
    /// destructor
    virtual ~nD3D9OcclusionQuery();

    struct Query
    {
        IDirect3DQuery9* d3dQuery;
        const void* userData;
    };

    nArray<Query> queryArray;
    nRef<nShader2> refShader;
    bool inBegin;
};
//------------------------------------------------------------------------------
#endif