//------------------------------------------------------------------------------
//  nd3d9occlusionquery.cc
//  (C) 2005 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "gfx2/nd3d9occlusionquery.h"
#include "gfx2/nshader2.h"

//------------------------------------------------------------------------------
/**
*/
nD3D9OcclusionQuery::nD3D9OcclusionQuery() :
    queryArray(256, 256),
    inBegin(false)
{
    this->queryArray.SetFlags(nArray<IDirect3DQuery9*>::DoubleGrowSize);
}

//------------------------------------------------------------------------------
/**
*/
nD3D9OcclusionQuery::~nD3D9OcclusionQuery()
{
    this->Clear();

    // release shader
    if (this->refShader.isvalid())
    {
        this->refShader->Release();
        this->refShader.invalidate();
    }
}

//------------------------------------------------------------------------------
/**
    This clears all queries regardless of whether they'd been finished or not.
*/
void
nD3D9OcclusionQuery::Clear()
{
    int i;
    int num = this->queryArray.Size();
    for (i = 0; i < num; i++)
    {
        Query& query = this->queryArray[i];
        if (0 != query.d3dQuery)
        {
            query.d3dQuery->Release();
            query.d3dQuery = 0;
        }
        query.userData = 0;
    }
    this->queryArray.Reset();
}

//------------------------------------------------------------------------------
/**
    Begin occlusion queries. This initializes a the shader and calls
    BeginShapes() on the gfx server.
*/
void
nD3D9OcclusionQuery::Begin()
{
    n_assert(!this->inBegin);
    nGfxServer2* gfxServer = nGfxServer2::Instance();

    // clear current mesh (LEAVE THIS IN! IT'S IMPORTANT SO THAT
    // Nebula2's REDUDANCY DETECTOR WON'T BE FOOLED!)
    gfxServer->SetMesh(0, 0);

    // initialize shader
    if (!this->refShader.isvalid())
    {
        nShader2* shd = gfxServer->NewShader("shaders:occlusionquery.fx");
        shd->SetFilename("shaders:occlusionquery.fx");
        bool occlusionShaderLoaded = shd->Load();
        n_assert(occlusionShaderLoaded);
        this->refShader = shd;
    }

    // clear any existing queries
    this->Clear();

    // activate shader
    gfxServer->SetShader(this->refShader.get());
    int numPasses = this->refShader->Begin(true);
    n_assert(1 == numPasses);
    this->refShader->BeginPass(0);

    this->inBegin = true;
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9OcclusionQuery::AddShapeQuery(nGfxServer2::ShapeType type, const matrix44& modelMatrix, const void* userData)
{
    n_assert(this->inBegin);
    HRESULT hr;

    // create a new query
    nD3D9Server* d3d9Server = (nD3D9Server*)nGfxServer2::Instance();
    IDirect3DDevice9* d3d9Dev = d3d9Server->GetD3DDevice();
    n_assert(d3d9Dev);

    IDirect3DQuery9* d3dQuery = 0;
    hr = d3d9Dev->CreateQuery(D3DQUERYTYPE_OCCLUSION, &d3dQuery);
    if (SUCCEEDED(hr))
    {
        // store query so we can check its status later
        Query newQuery;
        newQuery.d3dQuery = d3dQuery;
        newQuery.userData = userData;
        this->queryArray.Append(newQuery);

        // start the query
        hr = d3dQuery->Issue(D3DISSUE_BEGIN);
        n_assert(SUCCEEDED(hr));

        // render the shape to check for
        d3d9Server->DrawShapeNS(type, modelMatrix);

        // tell the query that we're done, note that this is an asynchronous
        // query, so we'll get the result later in GetOcclusionStatus()
        hr = d3dQuery->Issue(D3DISSUE_END);
        n_assert(SUCCEEDED(hr));
    }
    else if (D3DERR_NOTAVAILABLE)
    {
        // hmm, maybe no occlusion queries on this device...
        // just store a null pointer, we'll just simulate a failed query later on
        Query newQuery;
        newQuery.d3dQuery = 0;
        newQuery.userData = userData;
        this->queryArray.Append(newQuery);
    }
    else
    {
        // an error...
        n_dxtrace(hr, "nD3D9OcclusionQuery::AddShapeQuery(): CreateQuery failed!");
    }
}

//------------------------------------------------------------------------------
/**
*/
void
nD3D9OcclusionQuery::End()
{
    n_assert(this->inBegin);
    this->refShader->EndPass();
    this->refShader->End();
    this->inBegin = false;
}

//------------------------------------------------------------------------------
/**
*/
int
nD3D9OcclusionQuery::GetNumQueries() const
{
    n_assert(!this->inBegin);
    return this->queryArray.Size();
}

//------------------------------------------------------------------------------
/**
    Get user data associated with a query.
*/
const void*
nD3D9OcclusionQuery::GetUserData(int queryIndex)
{
    return this->queryArray[queryIndex].userData;
}

//------------------------------------------------------------------------------
/**
    Returns the occlusion status of a query defined by index. This method
    may wait for the query to finish, so it's wise to queue as many
    queries as possible before checking their status.
*/
bool
nD3D9OcclusionQuery::GetOcclusionStatus(int queryIndex)
{
    n_assert(!this->inBegin);
    Query& query = this->queryArray[queryIndex];
    IDirect3DQuery9* d3dQuery = query.d3dQuery;
    if (0 == d3dQuery)
    {
        // special case: no occlusion query available on this device,
        // always return that we're not occluded
        return false;
    }
    // an occlusion query returns the number of pixels which have passed
    // the z test in a DWORD
    DWORD numVisiblePixels = 0;
    n_assert(d3dQuery->GetDataSize() == sizeof(DWORD));

	// note: this method may return S_OK, S_FALSE or D3DERR_DEVICELOST.
    // S_FALSE is not considered an error!
    HRESULT hr;
    do
    {
        hr = d3dQuery->GetData(&numVisiblePixels, sizeof(DWORD), D3DGETDATA_FLUSH);
    }
    while (hr == S_FALSE);

    // return true if we're fully occluded
    return (numVisiblePixels == 0);
}
