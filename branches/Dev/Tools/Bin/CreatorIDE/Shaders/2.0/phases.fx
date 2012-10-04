//------------------------------------------------------------------------------
//  phases.fx
//
//  Contains renderpath phase shaders.
//
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------

technique tPhaseDepth
{
    pass p0
    {
        ColorWriteEnable = RED;
        AlphaTestEnable  = False;
        ZFunc            = LessEqual;
    }
}

technique tPhaseATestDepth
{
    pass p0
    {
        ColorWriteEnable = RED;
        AlphaTestEnable  = False; //???True?
        ZFunc            = LessEqual;
    }
}

technique tPhaseSky
{
    pass p0
    {
        ZWriteEnable = False;
    }
}

technique tPhaseTerrain
{
    pass p0
    {
        AlphaTestEnable = False;
        ZFunc           = LessEqual;
    }
}

technique tPhaseOpaque
{
    pass p0
    {
        AlphaTestEnable = False;
        ZFunc           = LessEqual;
    }
}

technique tPhaseOpaqueATest
{
    pass p0
    {
        AlphaTestEnable = True;
        ZFunc           = LessEqual;
    }
}

technique tPhaseNoLight
{
    pass p0
    {
        AlphaBlendEnable = False;
        AlphaTestEnable  = False;
        ZFunc            = LessEqual;
    }
}

technique tPhaseAlpha
{
    pass p0
    {
        AlphaBlendEnable = True;
        AlphaTestEnable  = False;
        ZFunc            = LessEqual;
    }
}

technique tPhasePointSprites
{
    pass p0
    {
        AlphaBlendEnable  = True;
        PointSpriteEnable = True;
        PointScaleEnable  = False;
        AlphaTestEnable   = False;
        ZFunc             = LessEqual;
    }
}
