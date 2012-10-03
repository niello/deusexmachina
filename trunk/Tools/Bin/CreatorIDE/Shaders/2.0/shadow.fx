//------------------------------------------------------------------------------
//  shadow_static.fx
//
//  Shadow volume shader for static geometry.
//
//  (C) 2005 Radon Labs GmbH
//------------------------------------------------------------------------------
shared float4x4 ModelViewProjection;

int StencilFrontZFailOp = 1;    // int: front faces stencil depth fail operation
int StencilFrontPassOp  = 1;    // int: front faces stencil pass operation
int StencilBackZFailOp  = 1;    // int: front faces stencil depth fail operation
int StencilBackPassOp   = 1;    // int: front faces stencil depth fail operation

float3 ModelLightPos;       // the light's position in model space
shared int    LightType;    // 0 == Point, 1 == Directional
shared float  LightRange;   // light range, only meaningfull for point lights

static float DirExtrudeLen = 100000.0f;

//------------------------------------------------------------------------------
/**
    Vertex shader for static shadow volumes. This will do the extrusion
    on the GPU. The extrude vector's x component is either 0.0 if the vector
    should not be extruded, or 1.0 if the vector should be extruded.

    If the light is a directional light, ModelLightPos does not contain a
    position, but instead the light direction in model space.

    NOTE: the non-extruded component could also be a very small number to
    shift the shadow volume a little bit away from the light source.

    NOTE: the extrusion may clip against the far plane, this case is not
    currently handled at all (should we even care??).
*/
float4 vsMain(float4 position : POSITION) : POSITION
{
    // compute the extrusion vector
    float4 extrudeVec = 0;
    float extrudeLen;
    if (1 == LightType)
    {
        // handle directional light
        extrudeVec.xyz = normalize(-ModelLightPos);
        extrudeLen = DirExtrudeLen;
    }
    else
    {
        // handle point light
        extrudeVec.xyz = normalize(position.xyz - ModelLightPos);
        extrudeLen = LightRange;
    }
    float4 pos = float4(position.xyz + extrudeVec * extrudeLen * position.w + extrudeVec * 0.0025f, 1.0f);
    return mul(pos, ModelViewProjection);
}

//------------------------------------------------------------------------------
/**
    A pixel shader for debug visualization.
*/
float4 psMain() : COLOR
{
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

//------------------------------------------------------------------------------
/**
    1-Pass-Technique for GPUs which can do 2-sided stencil operations.
*/
technique SinglePass
{
    pass p0
    {
        ZEnable             = true;
        FogEnable           = false;
        AlphaTestEnable     = false;

        ZWriteEnable        = false;
        ZFunc               = LessEqual;
        TwoSidedStencilMode = true;

        CullMode            = None;

        StencilFunc         = Always;
        StencilZFail        = <StencilFrontZFailOp>;
        StencilPass         = <StencilFrontPassOp>;

        Ccw_StencilFunc     = Always;
        Ccw_StencilZFail    = <StencilBackZFailOp>;
        Ccw_StencilPass     = <StencilBackPassOp>;

        VertexShader        = compile VS_PROFILE vsMain();

        ColorWriteEnable    = 0;
        AlphaBlendEnable    = false;
        StencilEnable       = true;
        PixelShader         = compile PS_PROFILE psMain();
    }
}

//------------------------------------------------------------------------------
/**
    2-Pass-Technique for older GPUs.
*/
technique TwoPass
{
    pass FrontFace
    {
        ColorWriteEnable    = 0;
        AlphaBlendEnable    = false;
        StencilEnable       = true;
        ZEnable             = true;
        FogEnable           = false;
        AlphaTestEnable     = false;

        ZWriteEnable        = false;
        ZFunc               = LessEqual;

        CullMode            = Cw;

        StencilFunc         = Always;
        StencilZFail        = <StencilFrontZFailOp>;
        StencilPass         = <StencilFrontPassOp>;

        VertexShader        = compile VS_PROFILE vsMain();
        PixelShader         = compile PS_PROFILE psMain();
    }
    pass BackFace
    {
        CullMode = Ccw;
        StencilZFail = <StencilBackZFailOp>;
        StencilPass = <StencilBackPassOp>;
    }
}
