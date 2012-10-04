//------------------------------------------------------------------------------
//  passes.fx
//
//  Render path passes.
//
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
shared float4x4 View;           // the view matrix
shared float4x4 Projection;     // the projection matrix

technique tPassDepth
{
    pass p0
    {
        ViewTransform       = <View>;
        ProjectionTransform = <Projection>;
        ColorWriteEnable    = 0;
        NormalizeNormals    = True;
        ZEnable             = True;
        ZWriteEnable        = True;
        StencilEnable       = False;
        DepthBias           = 0.0f;
        FogEnable           = False;
        AlphaBlendEnable    = False;
        AlphaFunc           = GreaterEqual;
        ScissorTestEnable   = False;
    }
}

technique tPassColor
{
    pass p0
    {
        ViewTransform       = <View>;
        ProjectionTransform = <Projection>;
        ColorWriteEnable    = RED|GREEN|BLUE|ALPHA;
        NormalizeNormals    = True;
        ZEnable             = True;
        ZWriteEnable        = False;
        StencilEnable       = False;
        DepthBias           = 0.0f;
        FogEnable           = False;
        AlphaBlendEnable    = True;
        AlphaTestEnable     = True;
        AlphaFunc           = GreaterEqual;
        SrcBlend            = One;
        DestBlend           = One;
        ScissorTestEnable   = True;
        //FillMode         = Wireframe;
    }
}

technique tPassEnvironment
{
    pass p0
    {
        ViewTransform       = <View>;
        ProjectionTransform = <Projection>;
        ColorWriteEnable    = RED|GREEN|BLUE|ALPHA;
        NormalizeNormals    = True;
        ZEnable             = True;
        ZWriteEnable        = True;
        StencilEnable       = False;
        DepthBias           = 0.0f;
        FogEnable           = False;
        AlphaBlendEnable    = True;
        AlphaTestEnable     = True;
        AlphaFunc           = GreaterEqual;
        SrcBlend            = One;
        DestBlend           = One;
        ScissorTestEnable   = True;
        //FillMode         = Wireframe;
    }
}
