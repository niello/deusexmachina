#pragma once
#include <Data/VarStorage.h>

namespace DEM::Flow
{
constexpr U32 EmptyActionID = 0;

struct CConditionData;
struct CFlowLink;
struct CFlowActionData;
class CFlowPlayer;
using PFlowAsset = Ptr<class CFlowAsset>;
using CFlowVarStorage = CVarStorage<bool, int, float, std::string, CStrID>;
}
