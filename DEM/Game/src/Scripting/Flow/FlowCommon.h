#pragma once
#include <Data/Ptr.h>

namespace DEM::Flow
{
constexpr U32 EmptyActionID = 0;

struct CConditionData;
struct CFlowLink;
struct CFlowActionData;
class CFlowPlayer;
using PFlowAsset = Ptr<class CFlowAsset>;
}
