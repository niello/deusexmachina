//------------------------------------------------------------------------------
//  occlusionquery.fx
//
//  Simple shader for occlusion queries. Just renders the passed geometry
//  with zwrite and color writes disabled.
//------------------------------------------------------------------------------
shared float4x4 ModelViewProjection;

//------------------------------------------------------------------------------
/**
    The vertex shader.
*/
float4 vsMain(float4 position : POSITION) : POSITION
{
    return mul(position, ModelViewProjection);
}

//------------------------------------------------------------------------------
/**
    A dummy pixel shader
*/
float4 psMain() : COLOR
{
    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}

//------------------------------------------------------------------------------
technique t0
{
    pass p0
    {
        ColorWriteEnable = 0;
        ZWriteEnable     = False;
        AlphaBlendEnable = False;
        AlphaTestEnable  = False;
        FogEnable        = False;
        ZEnable          = True;
        ZFunc            = LessEqual;
        StencilEnable    = False;
        CullMode         = Cw;

        VertexShader = compile VS_PROFILE vsMain();
        PixelShader  = compile PS_PROFILE psMain();
    }
}






