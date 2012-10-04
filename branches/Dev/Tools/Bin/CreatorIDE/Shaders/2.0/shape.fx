//------------------------------------------------------------------------------
//  fixed/shape.fx
//
//  Used for debug visualization.
//
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
shared float4x4 Model;
shared float4x4 View;                   // the view matrix
shared float4x4 Projection;             // the projection matrix

float4x4 TextureTransform0 = { 1.0, 0.0, 0.0, 0.0,
                               0.0, 1.0, 0.0, 0.0,
                               0.0, 0.0, 1.0, 0.0,
                               0.0, 0.0, 0.0, 1.0 };
float4 MatDiffuse;

//------------------------------------------------------------------------------
technique t0
{
    pass p0
    {
        WorldTransform[0]       = <Model>;
    	TextureTransform[0]     = <TextureTransform0>;
        ViewTransform           = <View>;
    	ProjectionTransform     = <Projection>;

        ColorWriteEnable        = RED | GREEN | BLUE | ALPHA;
        NormalizeNormals        = True;
        ZEnable                 = True;
        ZFunc                   = LessEqual;
        StencilEnable           = False;
        DepthBias               = 0.0;
        PixelShader             = NULL;
        SpecularEnable	        = False;

        Ambient                 = {0.8, 0.8, 0.8, 1.0 };
        LightEnable[0]          = False;
        LightAmbient[0]         = { 1.0, 1.0, 1.0, 1.0 };
        LightDiffuse[0]         = { 1.0, 1.0, 1.0, 1.0 };
        LightSpecular[0]        = { 1.0, 1.0, 1.0, 1.0 };
        LightPosition[0]        = { 0.0, 0.0, 0.0 };
        LightRange[0]           = 500000.0;
        LightAttenuation0[0]    = 1.0;
        LightAttenuation1[0]    = 0.0;
        LightAttenuation2[0]    = 0.0;

        // LightType must be the last light state that is set!
        LightType[0]            = POINT;

        ZWriteEnable            = False;
        AlphaBlendEnable        = True;
        AlphaTestEnable         = False;
        Lighting                = True;
        VertexShader            = NULL;
        FogEnable               = False;

        CullMode                = None;
        SrcBlend                = SrcAlpha;
        DestBlend               = InvSrcAlpha;
        MaterialAmbient	        = <MatDiffuse>;
        MaterialDiffuse         = <MatDiffuse>;
        MaterialSpecular        = { 0.0, 0.0, 0.0, 0.0 };

        FVF                     = XYZ | NORMAL | TEX1;

        Texture[0]              = NULL;
        TexCoordIndex[0]        = 0;
        TextureTransformFlags[0] = Count2;

        ColorOp[0]              = SelectArg1;
        ColorArg1[0]            = Current;

        ColorOp[1]              = Disable;

        AlphaOp[0]              = SelectArg1;
        AlphaArg1[0]            = Diffuse;

	    AlphaOp[1]              = Disable;

        //FillMode              = Wireframe;
    }
}
