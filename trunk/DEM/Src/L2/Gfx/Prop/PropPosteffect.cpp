////------------------------------------------------------------------------------
////  properties/posteffectproperty.h
////  (C) 2005 Radon Labs GmbH
////------------------------------------------------------------------------------
//#include "properties/posteffectproperty.h"
//#include "variable/nvariableserver.h"
//#include "game/entity.h"
//#include <DB/DBServer.h>
//
//namespace Attr
//{
//    DefineFloat(Saturation);
//    DefineVector4(Luminance);
//    DefineVector4(Balance);
//    DefineFloat(BrightPassThreshold);
//    DefineFloat(BrightPassOffset);
//    DefineFloat(BloomScale);
//    DefineVector4(FogColor);
//    DefineFloat(FogNearDist);
//    DefineFloat(FogFarDist);
//    DefineFloat(FocusDist);
//    DefineFloat(FocusLength);
//    DefineFloat(NoiseIntensity);
//    DefineFloat(NoiseScale);
//    DefineFloat(NoiseFrequency);
//};
//
//BEGIN_ATTRS_REGISTRATION(posteffectproperty)
//    RegisterFloat(Saturation, ReadOnly);
//    RegisterVector4(Luminance, ReadOnly);
//    RegisterVector4(Balance, ReadOnly);
//    RegisterFloat(BrightPassThreshold, ReadOnly);
//    RegisterFloat(BrightPassOffset, ReadOnly);
//    RegisterFloat(BloomScale, ReadOnly);
//    RegisterVector4(FogColor, ReadOnly);
//    RegisterFloat(FogNearDist, ReadOnly);
//    RegisterFloat(FogFarDist, ReadOnly);
//    RegisterFloat(FocusDist, ReadOnly);
//    RegisterFloat(FocusLength, ReadOnly);
//    RegisterFloat(NoiseIntensity, ReadOnly);
//    RegisterFloat(NoiseScale, ReadOnly);
//    RegisterFloat(NoiseFrequency, ReadOnly);
//END_ATTRS_REGISTRATION
//
//namespace Properties
//{
//ImplementRTTI(Properties::PostEffectProperty, Game::CProperty);
//ImplementFactory(Properties::PostEffectProperty);
//
//using namespace Game;
//
////------------------------------------------------------------------------------
///**
//*/
//PostEffectProperty::PostEffectProperty()
//{
//    // empty
//}
//
////------------------------------------------------------------------------------
///**
//*/
//PostEffectProperty::~PostEffectProperty()
//{
//    // empty
//}
//
////------------------------------------------------------------------------------
///**
//    Makes only sense for live entities.
//*/
//int
//PostEffectProperty::GetActiveEntityPools() const
//{
//    return Game::LivePool;
//}
//
////------------------------------------------------------------------------------
///**
//*/
//void
//PostEffectProperty::GetAttributes(nArray<DB::CAttrID>& Attrs)
//{
// //   CProperty::GetAttributes(nArray<DB::CAttrID>& Attrs);
//	//Attrs.Append(Attr::MaxVelocity);
//	//Attrs.Append(Attr::Moving);
//	//Attrs.Append(Attr::MaxVelocity);
//	//Attrs.Append(Attr::Moving);
//	//Attrs.Append(Attr::MaxVelocity);
//	//Attrs.Append(Attr::Moving);
//	//Attrs.Append(Attr::MaxVelocity);
//	//Attrs.Append(Attr::Moving);
//	//Attrs.Append(Attr::MaxVelocity);
//	//Attrs.Append(Attr::Moving);
//
//	//GetEntity()->Set<float>(Attr::Saturation, 1.0f);
// //   GetEntity()->Set<vector4>(Attr::Luminance, vector4(0.299f, 0.587f, 0.114f, 0.0f));
// //   GetEntity()->Set<vector4>(Attr::Balance, vector4(1.3f, 1.092f, 0.923f, 0.0f));
// //   GetEntity()->Set<float>(Attr::BrightPassThreshold, 0.2f);
// //   GetEntity()->Set<float>(Attr::BrightPassOffset, 2.2f);
// //   GetEntity()->Set<float>(Attr::BloomScale, 0.3f);
// //   GetEntity()->Set<vector4>(Attr::FogColor, vector4(0.065f, 0.104f, 0.12f, 1.0f));
// //   GetEntity()->Set<float>(Attr::FogNearDist, 0.0f);
// //   GetEntity()->Set<float>(Attr::FogFarDist, 200.0f);
// //   GetEntity()->Set<float>(Attr::FocusDist, 0.0f);
// //   GetEntity()->Set<float>(Attr::FocusLength, 180.0f);
// //   GetEntity()->Set<float>(Attr::NoiseIntensity, 0.02f);
// //   GetEntity()->Set<float>(Attr::NoiseScale, 11.86f);
// //   GetEntity()->Set<float>(Attr::NoiseFrequency, 20.22f);
//}
//
////------------------------------------------------------------------------------
///**
//    Apply the post-effect rendering attributes to Nebula2.
//*/
//void
//PostEffectProperty::OnRender()
//{
//    // update global Nebula2 display variables
//    nVariableServer* varServer = nVariableServer::Instance();
//    varServer->SetFloatVariable("Saturation", GetEntity()->Get<float>(Attr::Saturation));
//    varServer->SetVectorVariable("Luminance", GetEntity()->Get<vector4>(Attr::Luminance));
//    varServer->SetVectorVariable("Balance", GetEntity()->Get<vector4>(Attr::Balance));
//    varServer->SetFloatVariable("HdrBrightPassThreshold", GetEntity()->Get<float>(Attr::BrightPassThreshold));
//    varServer->SetFloatVariable("HdrBrightPassOffset", GetEntity()->Get<float>(Attr::BrightPassOffset));
//    varServer->SetFloatVariable("HdrBloomScale", GetEntity()->Get<float>(Attr::BloomScale));
//    varServer->SetVectorVariable("FogColor", GetEntity()->Get<vector4>(Attr::FogColor));
//    vector4 fogAttrs;
//    fogAttrs.x = GetEntity()->Get<float>(Attr::FogNearDist);
//    fogAttrs.y = GetEntity()->Get<float>(Attr::FogFarDist);
//    varServer->SetVectorVariable("FogDistances", fogAttrs);
//    vector4 focusAttrs;
//    focusAttrs.x = GetEntity()->Get<float>(Attr::FocusDist);
//    focusAttrs.y = GetEntity()->Get<float>(Attr::FocusLength);
//    varServer->SetVectorVariable("CameraFocus", focusAttrs);
//    varServer->SetFloatVariable("NoiseIntensity", GetEntity()->Get<float>(Attr::NoiseIntensity));
//    varServer->SetFloatVariable("NoiseScale", GetEntity()->Get<float>(Attr::NoiseScale));
//    varServer->SetFloatVariable("NoiseFrequency", GetEntity()->Get<float>(Attr::NoiseFrequency));
//}
//
//} // namespace Properties
