#include <Game/ECS/Entity.h>

// ECS systems and utils required for vendor logic (shop item regeneration etc)

namespace Resources
{
	class CResourceManager;
}

namespace DEM::Game
{
	class CGameWorld;
	class CGameSession;
}

namespace DEM::RPG
{

struct CVendorCoeffs
{
	float BuyFromVendorCoeff = 1.f;
	float SellToVendorCoeff = 1.f;
};

struct CItemPrices
{
	U32 BuyFromVendorPrice = 0;
	U32 BuyFromVendorQuantity = 0; // For items with unit price lower than 1 this contains a stack size for the BuyFromVendorPrice
	U32 SellToVendorPrice = 0;
	U32 SellToVendorQuantity = 0; // For items with unit price lower than 1 this contains a stack size for the SellToVendorPrice
};

void        InitVendors(Game::CGameWorld& World, Resources::CResourceManager& ResMgr);
void        RegenerateGoods(Game::CGameSession& Session, Game::HEntity VendorID, bool Force);
void        GetItemPrices(Game::CGameSession& Session, Game::HEntity VendorID, Game::HEntity BuyerID, const std::set<Game::HEntity>& ItemIDs, std::map<Game::HEntity, CVendorCoeffs>& Out);
CItemPrices GetItemStackUnitPrices(Game::CGameSession& Session, Game::HEntity StackID, const CVendorCoeffs& Coeffs);
U32         CalcStackPrice(U32 UnitPrice, U32 UnitQuantity, U32 Count, bool VendorGoods);
I32         GatherMoneyFromPouch(Game::CGameSession& Session, U32 TotalCost, const std::map<Game::HEntity, CVendorCoeffs>& PriceCoeffs, const std::map<Game::HEntity, U32>& UsedMoney, std::map<size_t, U32>& Out);

}
