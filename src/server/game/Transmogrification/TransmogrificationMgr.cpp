/*
 * Transmogrification
 */

#include "TransmogrificationMgr.h"
#include "ObjectMgr.h"
#include "Bag.h"
#include "Item.h"

TransmogrificationMgr::TransmogrificationMgr() {}
TransmogrificationMgr::~TransmogrificationMgr() {}

TransmogrificationMgr* TransmogrificationMgr::instance()
{
    static TransmogrificationMgr instance;
    return &instance;
}

void TransmogrificationMgr::LoadFromDB()
{
    uint32 oldMSTime = getMSTime();

    _transmogrificationStore.clear();

    //                                                      0       1
    QueryResult result = CharacterDatabase.Query("SELECT item, transEntry FROM item_transmogrification");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 transmogrified items. DB table `item_transmogrification` is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 item = fields[0].Get<uint32>();
        uint32 transEntry = fields[1].Get<uint32>();

        _transmogrificationStore.emplace(item, transEntry);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} transmogrified items in {} ms", uint32(_transmogrificationStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

uint32 TransmogrificationMgr::GetItemTransmogrification(ObjectGuid::LowType guid) const
{
    TransmogrificationContainer::const_iterator itr = _transmogrificationStore.find(guid);
    if (itr != _transmogrificationStore.end())
        return itr->second;

    return 0;
}

void TransmogrificationMgr::UpdateItemTransmogrification(ObjectGuid::LowType guid, uint32 transEntry)
{
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_TRANSMOGRIFICATION_INFO);
    stmt->SetData(0, guid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_TRANSMOGRIFICATION_INFO);
    stmt->SetData(0, guid);
    stmt->SetData(1, transEntry);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);

    _transmogrificationStore[guid] = transEntry;
}

void TransmogrificationMgr::RemoveItemTransmogrification(ObjectGuid::LowType guid)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_TRANSMOGRIFICATION_INFO);
    stmt->SetData(0, guid);
    CharacterDatabase.Execute(stmt);

    _transmogrificationStore.erase(guid);
}

void TransmogrificationMgr::RemoveAllTransmogrificationByEntry(Player* player, uint32 entry)
{
    if (player->GetItemCount(entry, true) > 1)
        return;

    auto transmogrifyRemover = [this, entry](Item* pItem) {
        if (GetItemTransmogrification(pItem->GetGUID().GetCounter()) == entry)
        {
            RemoveItemTransmogrification(pItem->GetGUID().GetCounter());
            UpdateItem(pItem);
        }
        };
    for (uint8 i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; i++)
        if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            transmogrifyRemover(pItem);

    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
        if (Bag* pBag = player->GetBagByPos(i))
            for (uint32 i = 0; i < pBag->GetBagSize(); ++i)
                if (Item* pItem = pBag->GetItemByPos(i))
                    transmogrifyRemover(pItem);

    // checking every item from 39 to 74 (including bank bags)
    for (uint8 i = BANK_SLOT_ITEM_START; i < BANK_SLOT_BAG_END; ++i)
        if (Item* pItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            transmogrifyRemover(pItem);

    for (uint8 i = BANK_SLOT_BAG_START; i < BANK_SLOT_BAG_END; ++i)
        if (Bag* pBag = player->GetBagByPos(i))
            for (uint32 i = 0; i < pBag->GetBagSize(); ++i)
                if (Item* pItem = pBag->GetItemByPos(i))
                    transmogrifyRemover(pItem);
}

void TransmogrificationMgr::SendTransmogrificationMenuCloseTo(Player* player)
{
    player->SendAddonMessage("ASMSG_TRANSMOGRIFICATION_MENU_CLOSE");
}

void TransmogrificationMgr::HandleTransmogrificationPrepareRequestFrom(Player* player, uint8 pos, uint32 transEntry)
{
    uint32 result = TRANSMOGRIFICATION_ERROR_OK;
    uint32 cost = 0;

    Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, pos);
    ItemTemplate const* trans = sObjectMgr->GetItemTemplate(transEntry);
    if (!item || !trans)
        result = TRANSMOGRIFICATION_ERROR_DONT_REPORT;
    else
        result = CanBeTransmogrifiedBy(player, item, trans);

    if (result == TRANSMOGRIFICATION_ERROR_OK)
        cost = GetPriceForItem(item->GetTemplate());

    LOG_ERROR("transmog", "Sent TRANSMOGRIFICATION_PREPARE_RESPONSE: {}:{}:{}:{}", pos + 1, transEntry, result, cost);

    player->SendAddonMessage(fmt::format(
        "ASMSG_TRANSMOGRIFICATION_PREPARE_RESPONSE\t{}:{}:{}:{}",
        pos + 1, transEntry, result, cost));

}

void TransmogrificationMgr::HandleTransmogrificationRemoveRequestFrom(Player* player, uint8 pos)
{
    Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, pos);
    if (!item)
        return;

    ObjectGuid::LowType guid = item->GetGUID().GetCounter();

    if (GetItemTransmogrification(guid) == 0)
        return;

    RemoveItemTransmogrification(guid);
    UpdateItem(item);

    player->SendAddonMessage("ASMSG_TRANSMOGRIFICATION_REMOVE_RESPONSE");
}

void TransmogrificationMgr::HandleTransmogrificationApplyRequestFrom(Player* player, std::map<uint8, uint32> data)
{
    if (data.empty())
        return;

    uint32 result = 0;
    int32 price = 0;
    std::unordered_map<Item*, uint32> transmogrification;

    for (std::map<uint8, uint32>::const_iterator itr = data.begin(); itr != data.end(); ++itr)
    {
        uint8 pos = itr->first;
        uint32 transEntry = itr->second;

        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, pos);
        ItemTemplate const* trans = sObjectMgr->GetItemTemplate(transEntry);
        if (!item || !trans || CanBeTransmogrifiedBy(player, item, trans) != TRANSMOGRIFICATION_ERROR_OK)
        {
            result = 1;
            transmogrification.clear();
            break;
        }

        price += GetPriceForItem(item->GetTemplate());
        transmogrification.emplace(item, transEntry);
    }

    if (result == 0)
    {
        if (player->HasEnoughMoney(price))
        {
            for (std::unordered_map<Item*, uint32>::const_iterator itr = transmogrification.begin(); itr != transmogrification.end(); ++itr)
            {
                if (Item* item = player->GetItemByEntry(itr->second))
                {
                    item->SetBinding(true);
                    player->RemoveTradeableItem(item);
                    item->ClearSoulboundTradeable(player);
                    item->SetNotRefundable(player);

                    itr->first->ClearSoulboundTradeable(player);
                    itr->first->SetNotRefundable(player);
                }
                UpdateItemTransmogrification(itr->first->GetGUID().GetCounter(), itr->second);
                UpdateItem(itr->first);
            }

            player->ModifyMoney(-price);
        }
        else
            result = 1;
    }

    LOG_ERROR("transmog", "Sent TRANSMOGRIFICATION_APPLY_RESPONSE: {}", result);

    player->SendAddonMessage(fmt::format("ASMSG_TRANSMOGRIFICATION_APPLY_RESPONSE\t{}", result));

}

std::string TransmogrificationMgr::GenerateTransmogrificationInfoFor(Player* player) const
{
    ASSERT(player);

    std::stringstream info;

    for (uint8 i = EQUIPMENT_SLOT_START; i < EQUIPMENT_SLOT_END; ++i)
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!item)
            continue;

        uint32 transEntry = GetItemTransmogrification(item->GetGUID().GetCounter());
        if (!transEntry)
            continue;

        info << uint32(i + 1) << ":" << transEntry << ";";
    }

    return info.str();
}

void TransmogrificationMgr::UpdateItem(Item* item)
{
    if (!item)
        return;

    if (!item->IsEquipped())
        return;

    Player* owner = item->GetOwner();
    if (!owner)
        return;

    owner->SetVisibleItemSlot(item->GetSlot(), item);
    if (owner->IsInWorld())
        item->SendUpdateToPlayer(owner);
}

uint32 TransmogrificationMgr::GetPriceForItem(ItemTemplate const* proto) const
{
    static std::map<uint32, uint32> const priceByItemLevel =
    {
        { 200, 50 * GOLD }, { 232, 100 * GOLD }, { 251, 150 * GOLD }, { 277, 300 * GOLD }, { 290, 500 * GOLD }, { 303, 1000 * GOLD }
    };

    uint32 itemLevel = proto->ItemLevel;
    uint32 price = 50 * GOLD;

    for (std::map<uint32, uint32>::const_iterator itr = priceByItemLevel.begin(); itr != priceByItemLevel.end(); ++itr)
    {
        if (itemLevel > itr->first)
            price = itr->second;
    }

    return price;
}

uint32 TransmogrificationMgr::CanBeTransmogrifiedBy(Player* player, Item* source, ItemTemplate const* trans) const
{
    if (!player || !source || !trans)
        return TRANSMOGRIFICATION_ERROR_DONT_REPORT;

    if (player->GetItemCount(trans->ItemId) == 0)
        return TRANSMOGRIFICATION_ERROR_DONT_REPORT;

    ItemTemplate const* sourceProto = source->GetTemplate();

    if (sourceProto->ItemId == trans->ItemId)
        return TRANSMOGRIFICATION_ERROR_CANNOT_ITEM_SELF;

    if (sourceProto->DisplayInfoID == trans->DisplayInfoID)
        return TRANSMOGRIFICATION_ERROR_SAME_APPEARANCE;

    if (sourceProto->Class != ITEM_CLASS_ARMOR && sourceProto->Class != ITEM_CLASS_WEAPON)
        return TRANSMOGRIFICATION_ERROR_CANNOT_BE_TRANSED;

    if (trans->Class != ITEM_CLASS_ARMOR && trans->Class != ITEM_CLASS_WEAPON)
        return TRANSMOGRIFICATION_ERROR_CANNOT_USE_FOR_TRANS;

    /*if (sourceProto->Class != trans->Class)
        return TRANSMOGRIFICATION_ERROR_CANNOT_USE_WITH_THIS_ITEM;*/

        /*if (sourceProto->Quality != ITEM_QUALITY_UNCOMMON && sourceProto->Quality != ITEM_QUALITY_RARE && sourceProto->Quality != ITEM_QUALITY_EPIC)
            return TRANSMOGRIFICATION_ERROR_CANNOT_BE_TRANSED;

        if (trans->Quality != ITEM_QUALITY_POOR && trans->Quality != ITEM_QUALITY_NORMAL && trans->Quality != ITEM_QUALITY_UNCOMMON && trans->Quality != ITEM_QUALITY_RARE && trans->Quality != ITEM_QUALITY_EPIC)
            return TRANSMOGRIFICATION_ERROR_CANNOT_USE_FOR_TRANS;*/

    if (sourceProto->InventoryType == INVTYPE_BAG ||
        sourceProto->InventoryType == INVTYPE_RELIC ||
        sourceProto->InventoryType == INVTYPE_BODY ||
        sourceProto->InventoryType == INVTYPE_FINGER ||
        sourceProto->InventoryType == INVTYPE_TRINKET ||
        sourceProto->InventoryType == INVTYPE_AMMO ||
        sourceProto->InventoryType == INVTYPE_QUIVER)
        return TRANSMOGRIFICATION_ERROR_CANNOT_BE_TRANSED;

    if (trans->InventoryType == INVTYPE_BAG ||
        trans->InventoryType == INVTYPE_RELIC ||
        trans->InventoryType == INVTYPE_BODY ||
        trans->InventoryType == INVTYPE_FINGER ||
        trans->InventoryType == INVTYPE_TRINKET ||
        trans->InventoryType == INVTYPE_AMMO ||
        trans->InventoryType == INVTYPE_QUIVER)
        return TRANSMOGRIFICATION_ERROR_CANNOT_USE_FOR_TRANS;

    if (sourceProto->SubClass != trans->SubClass)
    {
        if (sourceProto->Class == ITEM_CLASS_WEAPON && trans->Class != ITEM_CLASS_WEAPON)
            return TRANSMOGRIFICATION_ERROR_CANNOT_USE_WITH_THIS_ITEM;

        /*if (!((sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_AXE || sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_MACE || sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_SWORD || sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_FIST) &&
            (trans->SubClass == ITEM_SUBCLASS_WEAPON_AXE || trans->SubClass == ITEM_SUBCLASS_WEAPON_MACE || trans->SubClass == ITEM_SUBCLASS_WEAPON_SWORD || trans->SubClass == ITEM_SUBCLASS_WEAPON_FIST))
            &&
            !((sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_AXE2 || sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_MACE2 || sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_POLEARM || sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_SWORD2) &&
                (trans->SubClass == ITEM_SUBCLASS_WEAPON_AXE2 || trans->SubClass == ITEM_SUBCLASS_WEAPON_MACE2 || trans->SubClass == ITEM_SUBCLASS_WEAPON_POLEARM || trans->SubClass == ITEM_SUBCLASS_WEAPON_SWORD2))
            &&
            !((sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_BOW || sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_GUN || sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_CROSSBOW) &&
                (trans->SubClass == ITEM_SUBCLASS_WEAPON_BOW || trans->SubClass == ITEM_SUBCLASS_WEAPON_GUN || trans->SubClass == ITEM_SUBCLASS_WEAPON_CROSSBOW))
            &&
            !((sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_STAFF || sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_MACE2 || sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_POLEARM) &&
                (trans->SubClass == ITEM_SUBCLASS_WEAPON_STAFF || trans->SubClass == ITEM_SUBCLASS_WEAPON_MACE2 || trans->SubClass == ITEM_SUBCLASS_WEAPON_POLEARM))
            &&
            !((sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER || sourceProto->SubClass == ITEM_SUBCLASS_WEAPON_SWORD) &&
                (trans->SubClass == ITEM_SUBCLASS_WEAPON_DAGGER || trans->SubClass == ITEM_SUBCLASS_WEAPON_SWORD)))
            return TRANSMOGRIFICATION_ERROR_CANNOT_USE_WITH_THIS_ITEM;*/
    }

    if (sourceProto->InventoryType != trans->InventoryType)
    {
        if (sourceProto->Class == ITEM_CLASS_WEAPON &&
            !((trans->SubClass != ITEM_SUBCLASS_WEAPON_FIST && (trans->InventoryType == INVTYPE_WEAPON || trans->InventoryType == INVTYPE_WEAPONMAINHAND || trans->InventoryType == INVTYPE_WEAPONOFFHAND)) &&
                (sourceProto->SubClass != ITEM_SUBCLASS_WEAPON_FIST && (sourceProto->InventoryType == INVTYPE_WEAPON || sourceProto->InventoryType == INVTYPE_WEAPONMAINHAND || sourceProto->InventoryType == INVTYPE_WEAPONOFFHAND)))
            &&
            !((trans->InventoryType == INVTYPE_RANGED || trans->InventoryType == INVTYPE_RANGEDRIGHT) &&
                (sourceProto->InventoryType == INVTYPE_RANGED || sourceProto->InventoryType == INVTYPE_RANGEDRIGHT)))
            return TRANSMOGRIFICATION_ERROR_CANNOT_USE_WITH_THIS_ITEM;

        if (sourceProto->Class == ITEM_CLASS_ARMOR &&
            !((sourceProto->InventoryType == INVTYPE_CHEST || sourceProto->InventoryType == INVTYPE_ROBE) &&
                (trans->InventoryType == INVTYPE_CHEST || trans->InventoryType == INVTYPE_ROBE)))
            return TRANSMOGRIFICATION_ERROR_CANNOT_USE_WITH_THIS_ITEM;
    }

    /*if (!trans->IsAllowableToEquipFor(player))
        return TRANSMOGRIFICATION_ERROR_NOT_ALLOWABLE;*/

    return TRANSMOGRIFICATION_ERROR_OK;
}
