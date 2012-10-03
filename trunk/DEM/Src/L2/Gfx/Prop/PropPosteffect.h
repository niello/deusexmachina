//#ifndef PROPERTIES_POSTEFFECTPROPERTY_H
//#define PROPERTIES_POSTEFFECTPROPERTY_H
////------------------------------------------------------------------------------
///**
//    @class Properties::PostEffectProperty
//
//    The PostEffectProperty manages a number of attributes which influence
//    post-effect rendering.
//
//    FIXME: at the moment, only one PostEffectProperty object per level is
//    supported. In the future one should be able to place several post-effect
//    objects into the level which each have an influence bubble.
//
//    (C) 2005 Radon Labs GmbH
//*/
//#include "game/property.h"
//#include <db/Attributes.h>
//
////------------------------------------------------------------------------------
//namespace Attr
//{
//    // camera specific attributes
//    DeclareFloat(Saturation);                // color saturation
//    DeclareVector4(Luminance);               // luminance vector for color->bw conversion
//    DeclareVector4(Balance);                 // color balance
//    DeclareFloat(BrightPassThreshold);       // hdr bright pass threshold
//    DeclareFloat(BrightPassOffset);          // hdr bright pass offset
//    DeclareFloat(BloomScale);                // hdr bloom scale
//    DeclareVector4(FogColor);                // fog color
//    DeclareFloat(FogNearDist);               // fog near distance
//    DeclareFloat(FogFarDist);                // fog far distance
//    DeclareFloat(FocusDist);                 // focus distance
//    DeclareFloat(FocusLength);               // focus length
//    DeclareFloat(NoiseIntensity);            // film grain noise intensity
//    DeclareFloat(NoiseScale);                // film grain noise scale
//    DeclareFloat(NoiseFrequency);            // film grain noise frequency
//};
//
////------------------------------------------------------------------------------
//namespace Properties
//{
//class PostEffectProperty : public Game::CProperty
//{
//    DeclareRTTI;
//	DeclareFactory(PostEffectProperty);
//
//public:
//    /// constructor
//    PostEffectProperty();
//    /// destructor
//    virtual ~PostEffectProperty();
//    /// not active for sleeping entities
//    virtual int GetActiveEntityPools() const;
//    /// setup default entity attributes
//    virtual void GetAttributes(nArray<DB::CAttrID>& Attrs);
//    /// called before scene is rendered
//    virtual void OnRender();
//};
//
//RegisterFactory(PostEffectProperty);
//
//};
////------------------------------------------------------------------------------
//#endif
