/*
 * Transmogrification
 */

#ifndef _TRANSMOGRIFICATION_H
#define _TRANSMOGRIFICATION_H

#include "Common.h"
#include "Player.h"

enum TransmogrificationError
{
    TRANSMOGRIFICATION_ERROR_OK = 0,
    TRANSMOGRIFICATION_ERROR_DONT_REPORT = 1,
    TRANSMOGRIFICATION_ERROR_CANNOT_ITEM_SELF = 2,
    TRANSMOGRIFICATION_ERROR_SAME_APPEARANCE = 3,
    TRANSMOGRIFICATION_ERROR_CANNOT_USE_FOR_TRANS = 4,
    TRANSMOGRIFICATION_ERROR_CANNOT_USE_WITH_THIS_ITEM = 5,
    TRANSMOGRIFICATION_ERROR_CANNOT_BE_TRANSED = 6,
    TRANSMOGRIFICATION_ERROR_NOT_ALLOWABLE = 7
};

typedef std::unordered_map<ObjectGuid::LowType /*itemGuid*/, uint32 /*transEntry*/> TransmogrificationContainer;

class AC_GAME_API TransmogrificationMgr
{
private:
    TransmogrificationMgr();
    ~TransmogrificationMgr();
    
public:
    static TransmogrificationMgr * instance();
    
        void LoadFromDB();
    
        uint32 GetItemTransmogrification(ObjectGuid::LowType guid) const;
    void UpdateItemTransmogrification(ObjectGuid::LowType guid, uint32 transEntry);
    void RemoveItemTransmogrification(ObjectGuid::LowType guid);
    
        void RemoveAllTransmogrificationByEntry(Player * player, uint32 entry);
    void SendTransmogrificationMenuCloseTo(Player* player);
    
        void HandleTransmogrificationPrepareRequestFrom(Player * player, uint8 pos, uint32 transEntry);
    void HandleTransmogrificationRemoveRequestFrom(Player* player, uint8 pos);
    void HandleTransmogrificationApplyRequestFrom(Player* player, std::map<uint8, uint32> data);
    
        std::string GenerateTransmogrificationInfoFor(Player * player) const;
    
private:
    void UpdateItem(Item * item);
    uint32 GetPriceForItem(ItemTemplate const* proto) const;
    uint32 CanBeTransmogrifiedBy(Player* player, Item* source, ItemTemplate const* trans) const;
    
        TransmogrificationContainer _transmogrificationStore;
};

#define sTransmogrificationMgr TransmogrificationMgr::instance()

#endif // _TRANSMOGRIFICATION_H