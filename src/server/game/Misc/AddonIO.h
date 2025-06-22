
/*
 * Addon messages IO
 * for client-server interface
 */

#ifndef _ADDONIO_H
#define _ADDONIO_H

#include "Player.h"

typedef std::vector<std::string> StringVector;

class AC_GAME_API AddonIO
{
private:
    AddonIO();
    ~AddonIO();

public:
    static AddonIO* instance();

    void HandleMessage(Player* player, std::string message);

    //client msg
    void HandleShopBalanceRequest(Player* player, std::string body);
    void HandleShopItemListRequest(Player* player, std::string body);
    void HandleShopVersionRequest(Player* player, std::string body);
    void HandleShopBuyItemRequest(Player* player, std::string body);
    void HandleShopSpecialOfferListRequest(Player* player, std::string body);
    void HandleShopCollectionLoadRequest(Player* player, std::string body);
    void HandleShopItemCountRequest(Player* player, std::string body);
    void HandlePremiumInfoRequest(Player* player, std::string body);
    void HandlePremiumRenewRequest(Player* player, std::string body);
    void HandleAverageItemLevelRequest(Player* player, std::string body);

    void HandleTransmogrificationInfoRequest(Player* player, std::string body);
    void HandleTransmogrificationPrepareRequest(Player* player, std::string body);
    void HandleTransmogrificationRemove(Player* player, std::string body);
    void HandleTransmogrificationApply(Player* player, std::string body);

    void HandleGuildSpellsRequest(Player* player, std::string body);
    void HandleGuildLevelRequest(Player* player, std::string body);
    void HandleGuildOnlineRequest(Player* player, std::string body);
    void HandleGuildIlvlsRequest(Player* player, std::string body);
    void HandleGuildEmblemRequest(Player* player, std::string body);
    void HandleGuildTeamRequest(Player* player, std::string body);
};

typedef void(AddonIO::* AddonMessageHandler)(Player*, std::string);

#define sAddonIO AddonIO::instance()

#endif // _ADDONIO_H