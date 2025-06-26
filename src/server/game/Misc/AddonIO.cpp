/*
 * Addon messages IO
 * for client-server interface
 */

#include "AddonIO.h"
#include "AccountMgr.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "TransmogrificationMgr.h"
#include "Chat.h"
#include "Mail.h"
#include "Item.h"
#include "CharacterCache.h"
#include <boost/algorithm/string.hpp>

std::unordered_map<std::string, AddonMessageHandler> addonMessagesTable =
{
    //Donate Service
    { "ACMSG_SHOP_BALANCE_REQUEST",                     &AddonIO::HandleShopBalanceRequest                 },
    { "ACMSG_SHOP_ITEM_LIST_REQUEST",                   &AddonIO::HandleShopItemListRequest                },
    { "ACMSG_SHOP_VERSION",                             &AddonIO::HandleShopVersionRequest                 },
    { "ACMSG_SHOP_BUY_ITEM",                            &AddonIO::HandleShopBuyItemRequest                 },
    { "ACMSG_SHOP_SPECIAL_OFFER_LIST_REQUEST",          &AddonIO::HandleShopSpecialOfferListRequest        },
    { "ACMSG_SHOP_COLLECTION_LOAD_REQUEST",             &AddonIO::HandleShopCollectionLoadRequest          },
    { "ACMSG_SHOP_ITEM_COUNT",                          &AddonIO::HandleShopItemCountRequest               },
    { "ACMSG_PREMIUM_INFO_REQUEST",                     &AddonIO::HandlePremiumInfoRequest                 },
    { "ACMSG_PREMIUM_RENEW_REQUEST",                    &AddonIO::HandlePremiumRenewRequest                },
    { "ACMSG_AVERAGE_ITEM_LEVEL_REQUEST",               &AddonIO::HandleAverageItemLevelRequest            },
    // Transmogrification
    { "ACMSG_TRANSMOGRIFICATION_INFO_REQUEST",          &AddonIO::HandleTransmogrificationInfoRequest      },
    { "ACMSG_TRANSMOGRIFICATION_PREPARE_REQUEST",       &AddonIO::HandleTransmogrificationPrepareRequest   },
    { "ACMSG_TRANSMOGRIFICATION_REMOVE",                &AddonIO::HandleTransmogrificationRemove           },
    { "ACMSG_TRANSMOGRIFICATION_APPLY",                 &AddonIO::HandleTransmogrificationApply            },
    //Guild System
    { "ACMSG_GUILD_SPELLS_REQUEST",                     &AddonIO::HandleGuildSpellsRequest                 },
    { "ACMSG_GUILD_LEVEL_REQUEST",                      &AddonIO::HandleGuildLevelRequest                  },
    { "ACMSG_GUILD_ONLINE_REQUEST",                     &AddonIO::HandleGuildOnlineRequest                 },
    { "ACMSG_GUILD_ILVLS_REQUEST",                      &AddonIO::HandleGuildIlvlsRequest                  },
    { "ACMSG_GUILD_EMBLEM_REQUEST",                     &AddonIO::HandleGuildEmblemRequest                 },
    { "ASMSG_GUILD_TEAM",                               &AddonIO::HandleGuildTeamRequest                   }
};

/*********SHOPSERVICE*************/
enum STORE_ENUM {
    STORE_PREMIUM_BUY_1 = 1,
    STORE_PREMIUM_BUY_2 = 2,
    STORE_PREMIUM_BUY_3 = 3,
    STORE_PREMIUM_BUY_4 = 4,

    COST_STORE_PREMIUM_BUY_1 = 5,
    COST_STORE_PREMIUM_BUY_2 = 32,
    COST_STORE_PREMIUM_BUY_3 = 63,
    COST_STORE_PREMIUM_BUY_4 = 99,

    TIME_STORE_PREMIUM_BUY_1 = 86400,
    TIME_STORE_PREMIUM_BUY_2 = 604800,
    TIME_STORE_PREMIUM_BUY_3 = 1209600,
    TIME_STORE_PREMIUM_BUY_4 = 2592000,
};

enum PAID_SERVICE {
    PAID_SERVICE_NAME_CHANGE = 1000000,
    PAID_SERVICE_FACTION_CHANGE = 1000001,
    PAID_SERVICE_RACE_CHANGE = 1000002,
    PAID_SERVICE_GUILDNAME_CHANGE = 1000003,
    PAID_SERVICE_GOLD_BUY = 1000004,
    PAID_SERVICE_ALCHEMY_LEARH = 1000005,
    PAID_SERVICE_BLACKSMITHING_LEARH = 1000006,
    PAID_SERVICE_ENCHANTING_LEARN = 1000007,
    PAID_SERVICE_ENGINEERIN_LEARN = 1000008,
    PAID_SERVICE_JEWELCRAFTING_LEARN = 1000009,
    PAID_SERVICE_HERBALISM_LEARN = 1000010,
    PAID_SERVICE_LEATHERWORKING_LEARN = 1000011,
    PAID_SERVICE_MINING_LEARN = 1000012,
    PAID_SERVICE_SKINNING_LEARN = 1000013,
    PAID_SERVICE_TAILORING_LEARN = 1000014,
    PAID_SERVICE_FISHING_LEARH = 1000015,
    PAID_SERVICE_INSCRIPTION_LEARN = 1000016,
    PAID_SERVICE_COOKING_LEARN = 1000017,
    PAID_SERVICE_FIRST_AID_LEARN = 1000018,
    PAID_SERVICE_LEVELUP = 1000020,
    //MMOTOP
    PAID_SERVICE_PREMIUM_ONE_DAY = 1000019,
};

#define ARMORY_CATEGORYID               4

uint8 ShopProfesstionResponse(Player* pl, SkillType skill)
{
    if (pl->GetLevel() < DEFAULT_MAX_LEVEL)
        return 7; //ERROR_LOW_LEVEL
    else if (pl->PlayerAlreadyHasTwoProfessions(pl) && !pl->IsSecondarySkill(skill))
        return 6; //ERROR_PROF_TWO_IS_EXIST
    else
    {
        if (pl->LearnAllRecipesInProfession(pl, skill))
            return 0; //OK
    }

    return 1; //ERROR
}

bool ShopSendItem(Player* m_sender, Player* m_receiver, std::string m_text, uint32 item, uint32 count)
{
    Player* receiver = m_receiver;

    std::string subject = "Refund";
    std::string text = m_text;

    typedef std::pair<uint32, uint32> ItemPair;
    typedef std::list< ItemPair > ItemPairs;
    ItemPairs items;

    uint32 itemId = item;

    ItemTemplate const* item_proto = sObjectMgr->GetItemTemplate(itemId);
    if (!item_proto)
        return false;

    uint32 itemCount = count;
    if (itemCount < 1 || (item_proto->MaxCount > 0 && itemCount > uint32(item_proto->MaxCount)))
        return false;


    while (itemCount > item_proto->GetMaxStackSize())
    {
        items.push_back(ItemPair(itemId, item_proto->GetMaxStackSize()));
        itemCount -= item_proto->GetMaxStackSize();
    }

    items.push_back(ItemPair(itemId, itemCount));

    if (items.size() > MAX_MAIL_ITEMS)
        return false;


    MailSender sender(MAIL_NORMAL, m_sender->GetSession() ? m_sender->GetSession()->GetPlayer()->GetGUID() : 0, MAIL_STATIONERY_GM);


    MailDraft draft(subject, text);

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    for (ItemPairs::const_iterator itr = items.begin(); itr != items.end(); ++itr)
    {
        if (Item* item = Item::CreateItem(itr->first, itr->second, m_sender->GetSession() ? m_sender->GetSession()->GetPlayer() : 0))
        {
            item->SaveToDB(trans);
            draft.AddItem(item);
        }
    }

    draft.SendMailTo(trans, MailReceiver(receiver), sender);
    CharacterDatabase.CommitTransaction(trans);

    return true;
}

uint8 ShopAddItem(Player* player, Player* receiver, uint32 itemId, uint32 count, uint8 moneyID, uint32 cost, std::string text = "")
{
    // Проверяем, валидны ли игрок и его сессия
    if (!player || !player->GetSession())
        return 1; // Возвращаем ошибку, если игрок или сессия невалидны

    // Получаем шаблон предмета
    ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(itemId);
    if (!itemTemplate)
        return 4; // Предмет не найден

    // Списание стоимости или управление валютой
    if (moneyID == 10) {
        player->ModifyMoney(-cost);
    }
    else {
        if (!player->GetSession()->SetAccountCurrency(cost, moneyID, false))
            return 1; // Ошибка при обработке валюты
        if (moneyID == 1)
            player->GetSession()->WritePurchaseToLogs(player->GetSession(), "BUY ITEM", itemId, count, cost, uint32(time(nullptr)));
    }

    // Проверка места в инвентаре
    ItemPosCountVec dest;
    uint32 noSpaceForCount = 0;
    InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count, &noSpaceForCount);
    if (msg != EQUIP_ERR_OK)
        count -= noSpaceForCount; // Корректируем количество, если не все предметы помещаются

    // Сохраняем новый предмет
    Item* item = player->StoreNewItem(dest, itemId, true);
    if (item) {
        for (ItemPosCountVec::const_iterator itr = dest.begin(); itr != dest.end(); ++itr) {
            if (Item* item1 = player->GetItemByPos(itr->pos))
                item1->SetBinding(false);
        }
    }

    // Отправляем предмет игроку или уведомляем о возврате
    if (count > 0 && item) {
        if (text.empty())
            player->SendNewItem(item, count, false, true);
        else
            ShopSendItem(player, receiver, text, itemId, count);
    }

    // Обрабатываем случай, когда места нет
    if (noSpaceForCount > 0)
        ShopSendItem(player, player, "Return of lost items", itemId, noSpaceForCount);

    return 0;
}

uint8 ShopPaidService(Player* player, uint32 itemId, uint32 count, uint8 moneyID, uint32 cost, bool isProfession)
{
    // Проверяем, валидны ли игрок и его сессия
    if (!player || !player->GetSession())
        return 1;

    auto sess = player->GetSession();
    uint8 p_resp = 0;

    bool vip = AccountMgr::GetVipStatus(player->GetSession()->GetAccountId());

    if (!player->GetSession()->SetAccountCurrency(cost, moneyID, isProfession))
        p_resp = 1;
    else
    {
        uint32 atLoginFlag = 0;
        bool isRaceFaction = false;

        switch (itemId)
        {
        case PAID_SERVICE_NAME_CHANGE:
        {
            atLoginFlag = AT_LOGIN_RENAME;
            player->SetAtLoginFlag(AT_LOGIN_RENAME);
            isRaceFaction = true;
            player->GetSession()->WritePurchaseToLogs(sess, "NAME_CHANGE", 0, 0, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_FACTION_CHANGE:
        {
            atLoginFlag = AT_LOGIN_CHANGE_FACTION;
            player->SetAtLoginFlag(AT_LOGIN_CHANGE_FACTION);
            isRaceFaction = true;
            player->GetSession()->WritePurchaseToLogs(sess, "FACTION_CHANGE", 0, 0, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_RACE_CHANGE:
        {
            atLoginFlag = AT_LOGIN_CHANGE_RACE;
            player->SetAtLoginFlag(AT_LOGIN_CHANGE_RACE);
            isRaceFaction = true;
            player->GetSession()->WritePurchaseToLogs(sess, "RACE_CHANGE", 0, 0, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_GUILDNAME_CHANGE:
            break;
        case PAID_SERVICE_GOLD_BUY:
        {
            player->ModifyMoney(count * GOLD * 1000);
            player->GetSession()->WritePurchaseToLogs(sess, "GOLD_BUY", 0, count, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_ALCHEMY_LEARH:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_ALCHEMY);

            if (p_resp == 0)
            {
                player->GetSession()->WritePurchaseToLogs(sess, "ALCHEMY_LEARN", 0, 0, cost, uint32(time(nullptr)));

                player->removeSpell(28675, false, false);
                player->removeSpell(28677, false, false);
                player->removeSpell(28672, false, false);

            }

            break;
        }
        case PAID_SERVICE_BLACKSMITHING_LEARH:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_BLACKSMITHING);

            if (p_resp == 0)
            {
                player->GetSession()->WritePurchaseToLogs(sess, "BLACKSMITHING_LEARN", 0, 0, cost, uint32(time(nullptr)));
                player->removeSpell(9787, false, false);
                player->removeSpell(17041, false, false);
                player->removeSpell(17040, false, false);
                player->removeSpell(17039, false, false);
                player->removeSpell(9788, false, false);
            }


            break;
        }
        case PAID_SERVICE_ENCHANTING_LEARN:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_ENCHANTING);

            if (p_resp == 0)
                player->GetSession()->WritePurchaseToLogs(sess, "ENCHANTING_LEARN", 0, 0, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_ENGINEERIN_LEARN:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_ENGINEERING);

            if (p_resp == 0)
            {
                player->GetSession()->WritePurchaseToLogs(sess, "ENGINEERING_LEARN", 0, 0, cost, uint32(time(nullptr)));

                player->removeSpell(20222, false, false);
                player->removeSpell(20219, false, false);
            }

            break;
        }
        case PAID_SERVICE_JEWELCRAFTING_LEARN:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_JEWELCRAFTING);

            if (p_resp == 0)
                player->GetSession()->WritePurchaseToLogs(sess, "JEWELCRAFTING_LEARN", 0, 0, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_HERBALISM_LEARN:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_HERBALISM);

            if (p_resp == 0)
            {
                player->GetSession()->WritePurchaseToLogs(sess, "HERBALISM_LEARN", 0, 0, cost, uint32(time(nullptr)));

                player->removeSpell(2369, false, false);
                player->removeSpell(2371, false, false);
            }

            break;
        }
        case PAID_SERVICE_LEATHERWORKING_LEARN:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_LEATHERWORKING);

            if (p_resp == 0)
                player->GetSession()->WritePurchaseToLogs(sess, "LEATHERWORKING_LEARN", 0, 0, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_MINING_LEARN:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_MINING);

            if (p_resp == 0)
                player->GetSession()->WritePurchaseToLogs(sess, "MINING_LEARN", 0, 0, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_SKINNING_LEARN:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_SKINNING);

            if (p_resp == 0)
                player->GetSession()->WritePurchaseToLogs(sess, "SKINNING_LEARN", 0, 0, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_TAILORING_LEARN:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_TAILORING);

            if (p_resp == 0)
                player->GetSession()->WritePurchaseToLogs(sess, "TAILORING_LEARN", 0, 0, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_FISHING_LEARH:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_FISHING);

            if (p_resp == 0)
                player->GetSession()->WritePurchaseToLogs(sess, "FISHING_LEARN", 0, 0, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_INSCRIPTION_LEARN:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_INSCRIPTION);

            if (p_resp == 0)
                player->GetSession()->WritePurchaseToLogs(sess, "INSCRIPTION_LEARN", 0, 0, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_FIRST_AID_LEARN:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_FIRST_AID);

            if (p_resp == 0)
                player->GetSession()->WritePurchaseToLogs(sess, "FIRST_AID_LEARN", 0, 0, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_COOKING_LEARN:
        {
            p_resp = ShopProfesstionResponse(player, SKILL_COOKING);

            if (p_resp == 0)
                player->GetSession()->WritePurchaseToLogs(sess, "COOKING_LEARN", 0, 0, cost, uint32(time(nullptr)));

            break;
        }
        case PAID_SERVICE_PREMIUM_ONE_DAY:
            if (vip)
            {
                player->SetPremiumUnsetdate(TIME_STORE_PREMIUM_BUY_1 + player->GetPremiumUnsetdate());
                AccountMgr::UpdateVipStatus(player->GetSession()->GetAccountId(), TIME_STORE_PREMIUM_BUY_1 + player->GetPremiumUnsetdate());
                player->SetPremiumStatus(true);
            }
            else
            {
                player->SetPremiumUnsetdate(TIME_STORE_PREMIUM_BUY_1 + time(nullptr));
                AccountMgr::SetVipStatus(player->GetSession()->GetAccountId(), TIME_STORE_PREMIUM_BUY_1 + time(nullptr));
                player->SetPremiumStatus(true);
            }
            break;
        case PAID_SERVICE_LEVELUP:
            player->GiveLevel(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
            break;
        default:
            break;
        }

        if (isRaceFaction)
        {
            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
            stmt->SetData(0, uint16(atLoginFlag));
            stmt->SetData(1, player->GetGUID().GetCounter());
            CharacterDatabase.Execute(stmt);
        }

        if (isProfession && p_resp == 0)
            player->GetSession()->SetAccountCurrency(cost, moneyID, false);
    }

    return p_resp;
}
/******************************/

AddonIO::AddonIO() {}
AddonIO::~AddonIO() {}

AddonIO* AddonIO::instance()
{
    static AddonIO instance;
    return &instance;
}

void AddonIO::HandleMessage(Player* player, std::string message)
{
    std::vector<std::string> args;
    boost::split(args, message, boost::is_any_of("\t"));

    if (args.size() != 2)
        return;

    auto itr = addonMessagesTable.find(args[0]);
    if (itr == addonMessagesTable.end())
        return;

    (this->*itr->second)(player, args[1]);
}

/*****************************
********* HANDLERS ***********
******************************/

//Shop below

void AddonIO::HandleShopBalanceRequest(Player* player, std::string /*body*/)
{
    if (!player)
        return;

    auto sess = player->GetSession();

    LOG_ERROR("shop", "Sending ASMSG_SHOP_BALANCE_RESPONSE: {}:{}:{}:{}:{}:{}:{}",
        sess->GetAccountBalance(), sess->GetAccountVote(), 0, 0, 0, 0, 0);

    //outdated, sends %d
    //player->SendAddonMessage("ASMSG_SHOP_BALANCE_RESPONSE\t%d:%d:%d:%d:%d:%d:%d", sess->GetAccountBalance(), sess->GetAccountVote(), 0, 0, 0, 0, 0);

    player->SendAddonMessage(fmt::format("ASMSG_SHOP_BALANCE_RESPONSE\t{}:{}:{}:{}:{}:{}:{}",
        sess->GetAccountBalance(), sess->GetAccountVote(), 0, 0, 0, 0, 0));

}

void AddonIO::HandleShopItemListRequest(Player* player, std::string /*body*/)
{
    if (!player)
        return;

    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENABLE))
        return;

    auto store = sWorld->GetStoreItem();
    auto collection = sWorld->GetStorCollection();

    if (!store.empty())
    {
        for (auto it = store.begin(); it != store.end(); ++it)
        {
            if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(it->second.itemEntry))
            {
                if (it->second.CategoryID == ARMORY_CATEGORYID)
                {
                    if (proto->Class == ITEM_CLASS_ARMOR && proto->SubClass == ITEM_SUBCLASS_ARMOR_PLATE && !player->HasSpell(750))
                        continue;

                    if (proto->Class == ITEM_CLASS_ARMOR && proto->SubClass == ITEM_SUBCLASS_ARMOR_MAIL && !player->HasSpell(8737))
                        continue;

                    if (proto->Class == ITEM_CLASS_ARMOR && proto->SubClass == ITEM_SUBCLASS_ARMOR_LEATHER && !player->HasSpell(9077))
                        continue;

                    if (!(1 << (player->getClass() - 1) & proto->AllowableClass))
                        continue;

                    if (!(1 << (player->getRace() - 1) & proto->AllowableRace))
                        continue;
                }

                LOG_ERROR("shop", "Sending ASMSG_SHOP_ITEM for itemId {} to player {}", it->second.itemEntry, player->GetName());

                //Outdated sends %d
                //player->SendAddonMessage("ASMSG_SHOP_ITEM\t%d:%d:%d:%d:%d:%d:%d:%d:%d:%d:%d", it->first, it->second.itemEntry, it->second.count, it->second.price, it->second.discount, it->second.discountPrice, it->second.creatureEntry, it->second.storeFlags, it->second.CategoryID, it->second.SubCategoryID, it->second.MoneyID);

                player->SendAddonMessage(fmt::format("ASMSG_SHOP_ITEM\t{}:{}:{}:{}:{}:{}:{}:{}:{}:{}:{}",
                    it->first, it->second.itemEntry, it->second.count, it->second.price,
                    it->second.discount, it->second.discountPrice, it->second.creatureEntry,
                    it->second.storeFlags, it->second.CategoryID, it->second.SubCategoryID,
                    it->second.MoneyID));

            }
        }

    }

}

void AddonIO::HandleShopVersionRequest(Player* player, std::string /*body*/)
{
    if (!player)
        return;

    LOG_ERROR("shop", "Sending ASMSG_SHOP_VERSION: {}:{}", sWorld->GetShopVersion(), sWorld->getBoolConfig(CONFIG_SHOP_ENABLE) ? 1 : 0);

    //Outdated sends %d:%d
    //player->SendAddonMessage("ASMSG_SHOP_VERSION\t%d:%d", sWorld->GetShopVersion(), sWorld->getBoolConfig(CONFIG_SHOP_ENABLE) ? 1 : 0);

    player->SendAddonMessage(fmt::format("ASMSG_SHOP_VERSION\t{}:{}",
        sWorld->GetShopVersion(), sWorld->getBoolConfig(CONFIG_SHOP_ENABLE) ? 1 : 0));

}

void AddonIO::HandleShopBuyItemRequest(Player* player, std::string body)
{
    if (!player || !player->GetSession() || body.empty() || !sWorld->getBoolConfig(CONFIG_SHOP_ENABLE))
        return;

    auto sess = player->GetSession();

    uint8 p_resp = 1;
    uint32 item = 0, cost = 0, count = 1, db_count = 1, moneyID = 1, f_cost = 0;
    int32 balance = 0;

    try
    {
        std::vector<std::string> par;
        boost::split(par, body, boost::is_any_of(":"));

        if (!par.empty())
        {
            auto store = sWorld->GetStoreItem();
            if (!store.empty())
            {
                for (auto it = store.begin(); it != store.end(); ++it)
                    if (it->first == std::stoi(par[0]))
                    {
                        item = it->second.itemEntry;
                        cost = it->second.discountPrice;
                        db_count = it->second.count;
                        moneyID = it->second.MoneyID;
                        if (par.size() != 2)
                            count = std::stoi(par[1]);
                        break;
                    }
            }

            if (moneyID == 1)
                balance = sess->GetAccountBalance();
            else
                balance = sess->GetAccountVote();

            if (db_count > 1 && db_count != count)
                count = db_count;

            if (db_count == 1) f_cost = cost * count;
            else f_cost = cost;

            if (balance > 0)
                if (balance >= f_cost)
                {
                    switch (par.size())
                    {
                    case 4:
                    {
                        switch (item)
                        {
                        case PAID_SERVICE_ALCHEMY_LEARH:
                        case PAID_SERVICE_BLACKSMITHING_LEARH:
                        case PAID_SERVICE_ENCHANTING_LEARN:
                        case PAID_SERVICE_ENGINEERIN_LEARN:
                        case PAID_SERVICE_JEWELCRAFTING_LEARN:
                        case PAID_SERVICE_HERBALISM_LEARN:
                        case PAID_SERVICE_LEATHERWORKING_LEARN:
                        case PAID_SERVICE_MINING_LEARN:
                        case PAID_SERVICE_SKINNING_LEARN:
                        case PAID_SERVICE_TAILORING_LEARN:
                        case PAID_SERVICE_FISHING_LEARH:
                        case PAID_SERVICE_INSCRIPTION_LEARN:
                        case PAID_SERVICE_FIRST_AID_LEARN:
                        case PAID_SERVICE_COOKING_LEARN:
                            p_resp = ShopPaidService(player, item, count, moneyID, f_cost, true);
                            break;
                        case PAID_SERVICE_NAME_CHANGE:
                        case PAID_SERVICE_FACTION_CHANGE:
                        case PAID_SERVICE_RACE_CHANGE:
                        case PAID_SERVICE_GUILDNAME_CHANGE:
                        case PAID_SERVICE_GOLD_BUY:
                        case PAID_SERVICE_PREMIUM_ONE_DAY:
                        case PAID_SERVICE_LEVELUP:
                            p_resp = ShopPaidService(player, item, count, moneyID, f_cost, false);
                            break;
                        default:
                            p_resp = ShopAddItem(player, player, item, count, moneyID, f_cost);
                            break;
                        }

                        break;
                    }
                    case 6:
                    {
                        switch (item)
                        {
                        case PAID_SERVICE_NAME_CHANGE:
                        case PAID_SERVICE_FACTION_CHANGE:
                        case PAID_SERVICE_RACE_CHANGE:
                        case PAID_SERVICE_GUILDNAME_CHANGE:
                        case PAID_SERVICE_GOLD_BUY:
                        case PAID_SERVICE_ALCHEMY_LEARH:
                        case PAID_SERVICE_BLACKSMITHING_LEARH:
                        case PAID_SERVICE_ENCHANTING_LEARN:
                        case PAID_SERVICE_ENGINEERIN_LEARN:
                        case PAID_SERVICE_JEWELCRAFTING_LEARN:
                        case PAID_SERVICE_HERBALISM_LEARN:
                        case PAID_SERVICE_LEATHERWORKING_LEARN:
                        case PAID_SERVICE_MINING_LEARN:
                        case PAID_SERVICE_SKINNING_LEARN:
                        case PAID_SERVICE_TAILORING_LEARN:
                        case PAID_SERVICE_FISHING_LEARH:
                        case PAID_SERVICE_INSCRIPTION_LEARN:
                        case PAID_SERVICE_COOKING_LEARN:
                        case PAID_SERVICE_FIRST_AID_LEARN:
                        case PAID_SERVICE_PREMIUM_ONE_DAY:
                        case PAID_SERVICE_LEVELUP:
                            break;
                        default:
                        {
                            Player* target;
                            std::string targetName;
                            ObjectGuid parseGUID = ObjectGuid::Create<HighGuid::Player>(atol(par[3].c_str()));
                            if (sCharacterCache->GetCharacterNameByGuid(parseGUID, targetName))
                                target = ObjectAccessor::FindPlayer(parseGUID);

                            if (target == nullptr)
                            {
                                p_resp = 2; //ERROR_RECEIVER_NOT_FOUND
                                break;
                            }

                            if (player == target)
                            {
                                p_resp = 3; //ERROR_CANNOT_GIFT_TO_SELF
                                break;
                            }

                            p_resp = ShopAddItem(player, target, item, count, moneyID, f_cost, par[5].c_str());

                            break;
                        }
                        }

                        break;
                    }
                    }
                }
        }

        LOG_ERROR("shop", "Sending ASMSG_SHOP_BUY_ITEM_RESPONSE: {}:{}", p_resp, item);

        player->SendAddonMessage(fmt::format("ASMSG_SHOP_BUY_ITEM_RESPONSE\t{}:{}", p_resp, item));

    }
    catch (std::exception /*ex*/)
    {
        return;
    }
}

void AddonIO::HandleShopSpecialOfferListRequest(Player* player, std::string /*body*/)
{
    if (!player)
        return;

    /*auto sess = player->GetSession();
    auto offer = sWorld->GetStoreSpecialOffer();
    auto details = sWorld->GetStoreSpecialDetails();

    uint32 detailId = 0;
    char sub_resp[256];

    if (!details.empty())
    {
        //offerId|title|detailTitle|price|data: {itemId<role><count>:..:}
        for (auto it = details.begin(); it != details.end(); ++it)
            if (it->first == detailId)
            {
                char a[256];
                snprintf(a, 256, "%i<%d><%d>:", it->second.itemID, it->second.role, it->second.count);
                strcat(sub_resp, a);
            }

        player->SendAddonMessage(sub_resp);
    }*/
}

void AddonIO::HandleShopCollectionLoadRequest(Player* player, std::string /*body*/)
{
    if (!player)
        return;

    auto store = sWorld->GetStoreItem();

    if (!store.empty())
        for (auto it = store.begin(); it != store.end(); ++it)
            if (it->second.creatureEntry != 0)
                player->GetSession()->ShopCreatureOpcode(it->second.creatureEntry);
}

void AddonIO::HandleShopItemCountRequest(Player* player, std::string /*body*/)
{
    if (!player)
        return;

    if (!sWorld->getBoolConfig(CONFIG_SHOP_ENABLE))
        return;

    LOG_ERROR("shop", "Sending ASMSG_SHOP_ITEM_COUNT: {}", sWorld->GetStoreItems());

    //Old (sends %d literally)
    //player->SendAddonMessage("ASMSG_SHOP_ITEM_COUNT\t%d", sWorld->GetStoreItems());

    player->SendAddonMessage(fmt::format("ASMSG_SHOP_ITEM_COUNT\t{}",
        sWorld->GetStoreItems()));

}

//Premium below

void AddonIO::HandlePremiumInfoRequest(Player* player, std::string /*body*/)
{
    if (!player)
        return;

    int remaining = player->IsPremium() ? (player->GetPremiumUnsetdate() - uint32(time(nullptr))) : 0;

    LOG_ERROR("shop", "Sending ASMSG_PREMIUM_INFO_RESPONSE: {}", remaining);

    //Outdated sends %d
    //player->SendAddonMessage("ASMSG_PREMIUM_INFO_RESPONSE\t%d", remaining);

    player->SendAddonMessage(fmt::format("ASMSG_PREMIUM_INFO_RESPONSE\t{}", remaining));

}

void AddonIO::HandlePremiumRenewRequest(Player* player, std::string body)
{
    if (!player || !player->GetSession() || body.empty() || !sWorld->getBoolConfig(CONFIG_SHOP_ENABLE))
        return;

    try
    {
        auto mes = std::stoi(body);

        auto sess = player->GetSession();
        uint32 cost = 0, prem_time = 0;
        uint8 p_resp = 1;

        switch (mes)
        {
        case STORE_PREMIUM_BUY_1:
            cost = COST_STORE_PREMIUM_BUY_1;
            prem_time = TIME_STORE_PREMIUM_BUY_1;
            break;
        case STORE_PREMIUM_BUY_2:
            cost = COST_STORE_PREMIUM_BUY_2;
            prem_time = TIME_STORE_PREMIUM_BUY_2;
            break;
        case STORE_PREMIUM_BUY_3:
            cost = COST_STORE_PREMIUM_BUY_3;
            prem_time = TIME_STORE_PREMIUM_BUY_3;
            break;
        case STORE_PREMIUM_BUY_4:
            cost = COST_STORE_PREMIUM_BUY_4;
            prem_time = TIME_STORE_PREMIUM_BUY_4;
            break;
        default:
            break;
        }

        if (cost && sess->GetAccountBalance() >= cost)
        {
            if (sess->SetAccountCurrency(cost, 1, false))
            {
                bool vip = AccountMgr::GetVipStatus(sess->GetAccountId());
                if (vip)
                {
                    player->SetPremiumUnsetdate(prem_time + player->GetPremiumUnsetdate());
                    AccountMgr::UpdateVipStatus(sess->GetAccountId(), prem_time + player->GetPremiumUnsetdate());
                    player->SetPremiumStatus(true);
                }
                else
                {
                    player->SetPremiumUnsetdate(prem_time + time(nullptr));
                    AccountMgr::SetVipStatus(sess->GetAccountId(), prem_time + time(nullptr));
                    player->SetPremiumStatus(true);
                }
            }

            sess->WritePurchaseToLogs(sess, "PREMIUM", prem_time, 0, cost, uint32(time(nullptr)));

            p_resp = 0; //ok
        }

        LOG_ERROR("shop", "Sending ASMSG_PREMIUM_RENEW_RESPONSE: {}", p_resp);

        //Outdated sends %d
        //player->SendAddonMessage("ASMSG_PREMIUM_RENEW_RESPONSE\t%u", p_resp);

        player->SendAddonMessage(fmt::format("ASMSG_PREMIUM_RENEW_RESPONSE\t{}", p_resp));

    }
    catch (std::exception /*ex*/)
    {
        return;
    }

}

//Transmog below

void AddonIO::HandleTransmogrificationInfoRequest(Player* player, std::string body)
{
    if (!player || body.empty())
        return;

    ObjectGuid guid = ObjectGuid::Empty;

    try
    {
        uint64 value = std::stoull(body, nullptr, 16);
        guid = ObjectGuid(value);
    }
    catch (std::exception /*ex*/)
    {
        return;
    }

    Player* target = ObjectAccessor::GetPlayer(*player, guid);
    if (!target)
        return;

    std::stringstream response;
    response << "ASMSG_TRANSMOGRIFICATION_INFO_RESPONSE\t" << guid.GetRawValue() << ";" << sTransmogrificationMgr->GenerateTransmogrificationInfoFor(target);
}

void AddonIO::HandleTransmogrificationPrepareRequest(Player* player, std::string body)
{
    if (!player || body.empty())
        return;

    StringVector args; // 0 - slot, 1 - transEntry
    boost::split(args, body, boost::is_any_of(":"));

    if (args.size() != 2)
        return;

    uint8 slot = 0;
    uint32 transEntry = 0;

    try
    {
        slot = std::stoi(args[0]) - 1;
        transEntry = std::stoi(args[1]);

        if (slot < EQUIPMENT_SLOT_START || slot > EQUIPMENT_SLOT_END)
            throw std::exception();
    }
    catch (std::exception /*ex*/)
    {
        return;
    }

    sTransmogrificationMgr->HandleTransmogrificationPrepareRequestFrom(player, slot, transEntry);
}

void AddonIO::HandleTransmogrificationRemove(Player* player, std::string body)
{
    if (!player || body.empty())
        return;

    uint8 slot = 0;

    try
    {
        slot = std::stoi(body) - 1;
        if (slot < EQUIPMENT_SLOT_START || slot > EQUIPMENT_SLOT_END)
            throw std::exception();
    }
    catch (std::exception /*ex*/)
    {
        return;
    }

    sTransmogrificationMgr->HandleTransmogrificationRemoveRequestFrom(player, slot);
}

void AddonIO::HandleTransmogrificationApply(Player* player, std::string body)
{
    if (!player || body.empty())
        return;

    std::map<uint8, uint32> data;

    StringVector slotEntryPairs;
    boost::split(slotEntryPairs, body, boost::is_any_of(";"));

    if (slotEntryPairs.empty())
        return;

    try
    {
        for (StringVector::const_iterator itr = slotEntryPairs.begin(); itr != slotEntryPairs.end(); ++itr)
        {
            StringVector pair;
            boost::split(pair, *itr, boost::is_any_of(":"));
            if (pair.size() != 2)
                continue;

            uint8 slot = std::stoi(pair[0]) - 1;
            uint32 transEntry = std::stoi(pair[1]);

            if (slot < EQUIPMENT_SLOT_START || slot > EQUIPMENT_SLOT_END)
                throw std::exception();

            data.emplace(slot, transEntry);
        }
    }
    catch (std::exception /*ex*/)
    {
        return;
    }

    sTransmogrificationMgr->HandleTransmogrificationApplyRequestFrom(player, data);
}

//GUILD BELOW

void AddonIO::HandleGuildSpellsRequest(Player* player, std::string /*body*/)
{
    if (!player)
        return;

    if (!sGuildPerkSpellsStore.empty())
    {
        std::string response = "ASMSG_GUILD_SPELLS_RESPONSE\t";
        for (auto it = sGuildPerkSpellsStore.begin(); it != sGuildPerkSpellsStore.end(); ++it)
            response += std::to_string(it->second) + ":" + std::to_string(it->first) + ",";

        LOG_ERROR("guild", "Sending Guild Spell Response: {}", response);

        player->SendAddonMessage(response.c_str());
    }
}

void AddonIO::HandleGuildLevelRequest(Player* player, std::string /*body*/)
{
    if (!player)
        return;

    if (Guild* guild = player->GetGuild())
    {
        uint8 lvl = guild->GetLevel() == GUILD_MAX_LEVEL ? (GUILD_MAX_LEVEL - 1) : guild->GetLevel();
        uint32 xp_for_old_lvl = lvl != 0 ? sWorld->GetXpForNextLevel(lvl - 1) : 0;
        uint32 xp_for_next_lvl = sWorld->GetXpForNextLevel(lvl);
        uint32 totalxp = xp_for_next_lvl - xp_for_old_lvl;
        uint32 xp = guild->GetCurrentXP() - xp_for_old_lvl;
        uint32 dailyCap = sWorld->getIntConfig(CONFIG_GUILD_DAILY_XP_CAP);

        //player->SendAddonMessage("ASMSG_GUILD_LEVEL_INFO\t%d:%d:%d:%d:%d", guild->GetLevel(), xp, totalxp, guild->GetGuildTodayXP(), dailyCap);
        //Rewriting line above , testing with buffer
        std::string message = fmt::format("ASMSG_GUILD_LEVEL_INFO\t{}:{}:{}:{}:{}",
            guild->GetLevel(), xp, totalxp, guild->GetGuildTodayXP(), dailyCap);

        LOG_ERROR("guild", "Sent HandleGuildLevelRequest message: {}", message);

        player->SendAddonMessage(message);

    }
}

void AddonIO::HandleGuildOnlineRequest(Player* player, std::string /*body*/)
{
    if (!player)
        return;

    if (Guild* guild = player->GetGuild())
    {
        int online = guild->GetOnlineMembers();
        int total = guild->GetMemberCount();

        LOG_ERROR("guild", "Sending ASMSG_GUILD_PLAYERS_COUNT: {}:{}", online, total);

        //Outdated sends %d:%d
        //player->SendAddonMessage("ASMSG_GUILD_PLAYERS_COUNT\t%d:%d", online, total);

        player->SendAddonMessage(fmt::format("ASMSG_GUILD_PLAYERS_COUNT\t{}:{}",
            guild->GetOnlineMembers(), guild->GetMemberCount()));

    }
}

void AddonIO::HandleGuildIlvlsRequest(Player* player, std::string /*body*/)
{
    if (!player)
        return;

    if (Guild* guild = player->GetGuild())
    {
        std::string response = "ASMSG_GUILD_PLAYERS_ILVL\t";
        std::unordered_map<uint32, Guild::Member> members = guild->GetMembers();
        for (const auto& itr : members)
            response += itr.second.GetName() + ":" + std::to_string(itr.second.GetAverageLvl()) + "|";

        LOG_ERROR("guild", "Sending ASMSG_GUILD_PLAYERS_ILVL: {}", response);

        player->SendAddonMessage(response.c_str());
    }
}

void AddonIO::HandleGuildEmblemRequest(Player* player, std::string /*body*/)
{
    if (!player)
        return;

    if (Guild* guild = player->GetGuild())
    {
        EmblemInfo emblem = guild->GetEmblemInfo();

        int s = emblem.GetStyle();
        int c = emblem.GetColor();
        int bs = emblem.GetBorderStyle();
        int bc = emblem.GetBorderColor();
        int bg = emblem.GetBackgroundColor();

        LOG_ERROR("guild", "Sending ASMSG_PLAYER_GUILD_EMBLEM_INFO: {}:{}:{}:{}:{}", s, c, bs, bc, bg);

        //Outdated sends %d:%d:%d:%d:%d
        //player->SendAddonMessage("ASMSG_PLAYER_GUILD_EMBLEM_INFO\t%d:%d:%d:%d:%d", s, c, bs, bc, bg);

        player->SendAddonMessage(fmt::format("ASMSG_PLAYER_GUILD_EMBLEM_INFO\t{}:{}:{}:{}:{}",
            emblem.GetStyle(), emblem.GetColor(), emblem.GetBorderStyle(),
            emblem.GetBorderColor(), emblem.GetBackgroundColor()));

    }
}

void AddonIO::HandleGuildTeamRequest(Player* player, std::string /*body*/)
{
    if (!player)
        return;

    uint8 factionIcon = 1;
    switch (player->getRace())
    {
    case RACE_DRAENEI:
    case RACE_HUMAN:
    case RACE_GNOME:
    case RACE_DWARF:
    case RACE_NIGHTELF:
        //case RACE_VOIDELF:
        factionIcon = 0;
        [[fallthrough]];
    default:
        break;
    }

    LOG_ERROR("guild", "Sent HandleGuildTeamRequest message: {}", factionIcon);

    //Outdated sends %d
    //player->SendAddonMessage("ASMSG_GUILD_TEAM\t%d", factionIcon);

    player->SendAddonMessage(fmt::format("ASMSG_GUILD_TEAM\t{}", factionIcon));

}

void AddonIO::HandleAverageItemLevelRequest(Player* player, std::string body)
{
    if (!player)
        return;

    if (body.empty())
        return;

    ObjectGuid guid = ObjectGuid::Empty;

    try
    {
        uint64 value = std::stoull(body, nullptr, 16);
        guid = ObjectGuid(value);
    }
    catch (std::exception /*ex*/)
    {
        LOG_ERROR("guild", "ACMSG_AVERAGE_ITEM_LEVEL_REQUEST: Received invalid parameters!");
        player->SendAddonMessage("ASMSG_AVERAGE_ITEM_LEVEL_RESPONSE\t-1");
        return;
    }

    Player* target = ObjectAccessor::FindPlayer(guid);
    if (!target)
    {
        LOG_ERROR("guild", "ACMSG_AVERAGE_ITEM_LEVEL_REQUEST: !target triggered!");
        player->SendAddonMessage("ASMSG_AVERAGE_ITEM_LEVEL_RESPONSE\t-1");
        return;
    }

    uint32 ilvl = static_cast<uint32>(std::floor(target->GetAverageItemLevel()));

    LOG_ERROR("guild", "HandleAverageItemLevelRequest - Sending {}", ilvl);

    //outdated
    //player->SendAddonMessage("ASMSG_AVERAGE_ITEM_LEVEL_RESPONSE\t%u", static_cast<uint32>(std::floor(target->GetAverageItemLevel())));
    player->SendAddonMessage(fmt::format("ASMSG_AVERAGE_ITEM_LEVEL_RESPONSE\t{}", ilvl));

}
