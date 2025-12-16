/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "AuctionHouseBot.h"
#include "AuctionHouseMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Item.h"
#include "Language.h"
#include "RBAC.h"
#include <map>

using namespace Trinity::ChatCommands;

static std::unordered_map<AuctionQuality, uint32> const ahbotQualityLangIds =
{
    { AUCTION_QUALITY_GRAY,   LANG_AHBOT_QUALITY_GRAY },
    { AUCTION_QUALITY_WHITE,  LANG_AHBOT_QUALITY_WHITE },
    { AUCTION_QUALITY_GREEN,  LANG_AHBOT_QUALITY_GREEN },
    { AUCTION_QUALITY_BLUE,   LANG_AHBOT_QUALITY_BLUE },
    { AUCTION_QUALITY_PURPLE, LANG_AHBOT_QUALITY_PURPLE },
    { AUCTION_QUALITY_ORANGE, LANG_AHBOT_QUALITY_ORANGE },
    { AUCTION_QUALITY_YELLOW, LANG_AHBOT_QUALITY_YELLOW }
};

class ahbot_commandscript : public CommandScript
{
public:
    ahbot_commandscript(): CommandScript("ahbot_commandscript") {}

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable ahbotItemsAmountCommandTable =
        {
            { "gray",       HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_GRAY>,     rbac::RBAC_PERM_COMMAND_AHBOT_ITEMS_GRAY,       Console::Yes },
            { "white",      HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_WHITE>,    rbac::RBAC_PERM_COMMAND_AHBOT_ITEMS_WHITE,      Console::Yes },
            { "green",      HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_GREEN>,    rbac::RBAC_PERM_COMMAND_AHBOT_ITEMS_GREEN,      Console::Yes },
            { "blue",       HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_BLUE>,     rbac::RBAC_PERM_COMMAND_AHBOT_ITEMS_BLUE,       Console::Yes },
            { "purple",     HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_PURPLE>,   rbac::RBAC_PERM_COMMAND_AHBOT_ITEMS_PURPLE,     Console::Yes },
            { "orange",     HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_ORANGE>,   rbac::RBAC_PERM_COMMAND_AHBOT_ITEMS_ORANGE,     Console::Yes },
            { "yellow",     HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_YELLOW>,   rbac::RBAC_PERM_COMMAND_AHBOT_ITEMS_YELLOW,     Console::Yes },
            { "",           HandleAHBotItemsAmountCommand,                                  rbac::RBAC_PERM_COMMAND_AHBOT_ITEMS,            Console::Yes },
        };

        static ChatCommandTable ahbotItemsRatioCommandTable =
        {
            { "alliance",   HandleAHBotItemsRatioHouseCommand<AUCTION_HOUSE_ALLIANCE>,      rbac::RBAC_PERM_COMMAND_AHBOT_RATIO_ALLIANCE,   Console::Yes },
            { "horde",      HandleAHBotItemsRatioHouseCommand<AUCTION_HOUSE_HORDE>,         rbac::RBAC_PERM_COMMAND_AHBOT_RATIO_HORDE,      Console::Yes },
            { "neutral",    HandleAHBotItemsRatioHouseCommand<AUCTION_HOUSE_NEUTRAL>,       rbac::RBAC_PERM_COMMAND_AHBOT_RATIO_NEUTRAL,    Console::Yes },
            { "",           HandleAHBotItemsRatioCommand,                                   rbac::RBAC_PERM_COMMAND_AHBOT_RATIO,            Console::Yes },
        };

        static ChatCommandTable ahbotCommandTable =
        {
            { "items",      ahbotItemsAmountCommandTable },
            { "ratio",      ahbotItemsRatioCommandTable },
            { "rebuild",    HandleAHBotRebuildCommand,  rbac::RBAC_PERM_COMMAND_AHBOT_REBUILD,  Console::Yes },
            { "reload",     HandleAHBotReloadCommand,   rbac::RBAC_PERM_COMMAND_AHBOT_RELOAD,   Console::Yes },
            { "status",     HandleAHBotStatusCommand,   rbac::RBAC_PERM_COMMAND_AHBOT_STATUS,   Console::Yes },
            { "stats",      HandleAHBotStatsCommand,    rbac::RBAC_PERM_COMMAND_AHBOT_STATUS,   Console::Yes },
        };

        static ChatCommandTable commandTable =
        {
            { "ahbot", ahbotCommandTable },
        };

        return commandTable;
    }

    static bool HandleAHBotItemsAmountCommand(ChatHandler* handler, std::array<uint32, MAX_AUCTION_QUALITY> items)
    {
        sAuctionBot->SetItemsAmount(items);

        for (AuctionQuality quality : EnumUtils::Iterate<AuctionQuality>())
            handler->PSendSysMessage(LANG_AHBOT_ITEMS_AMOUNT, handler->GetTrinityString(ahbotQualityLangIds.at(quality)), sAuctionBotConfig->GetConfigItemQualityAmount(quality));

        return true;
    }

    template <AuctionQuality Q>
    static bool HandleAHBotItemsAmountQualityCommand(ChatHandler* handler, uint32 amount)
    {
        sAuctionBot->SetItemsAmountForQuality(Q, amount);
        handler->PSendSysMessage(LANG_AHBOT_ITEMS_AMOUNT, handler->GetTrinityString(ahbotQualityLangIds.at(Q)),
            sAuctionBotConfig->GetConfigItemQualityAmount(Q));

        return true;
    }

    static bool HandleAHBotItemsRatioCommand(ChatHandler* handler, uint32 alliance, uint32 horde, uint32 neutral)
    {
        sAuctionBot->SetItemsRatio(alliance, horde, neutral);

        for (AuctionHouseType type : EnumUtils::Iterate<AuctionHouseType>())
            handler->PSendSysMessage(LANG_AHBOT_ITEMS_RATIO, AuctionBotConfig::GetHouseTypeName(type), sAuctionBotConfig->GetConfigItemAmountRatio(type));
        return true;
    }

    template<AuctionHouseType H>
    static bool HandleAHBotItemsRatioHouseCommand(ChatHandler* handler, uint32 ratio)
    {
        sAuctionBot->SetItemsRatioForHouse(H, ratio);
        handler->PSendSysMessage(LANG_AHBOT_ITEMS_RATIO, AuctionBotConfig::GetHouseTypeName(H), sAuctionBotConfig->GetConfigItemAmountRatio(H));
        return true;
    }

    static bool HandleAHBotRebuildCommand(ChatHandler* /*handler*/, Optional<EXACT_SEQUENCE("all")> all)
    {
        sAuctionBot->Rebuild(all.has_value());
        return true;
    }

    static bool HandleAHBotReloadCommand(ChatHandler* handler)
    {
        sAuctionBot->ReloadAllConfig();
        handler->SendSysMessage(LANG_AHBOT_RELOAD_OK);
        return true;
    }

    static bool HandleAHBotStatusCommand(ChatHandler* handler, Optional<EXACT_SEQUENCE("all")> all)
    {
        std::unordered_map<AuctionHouseType, AuctionHouseBotStatusInfoPerType> statusInfo;
        sAuctionBot->PrepareStatusInfos(statusInfo);

        WorldSession* session = handler->GetSession();

        if (!session)
        {
            handler->SendSysMessage(LANG_AHBOT_STATUS_BAR_CONSOLE);
            handler->SendSysMessage(LANG_AHBOT_STATUS_TITLE1_CONSOLE);
            handler->SendSysMessage(LANG_AHBOT_STATUS_MIDBAR_CONSOLE);
        }
        else
            handler->SendSysMessage(LANG_AHBOT_STATUS_TITLE1_CHAT);

        uint32 fmtId = session ? LANG_AHBOT_STATUS_FORMAT_CHAT : LANG_AHBOT_STATUS_FORMAT_CONSOLE;

        handler->PSendSysMessage(fmtId, handler->GetTrinityString(LANG_AHBOT_STATUS_ITEM_COUNT),
            statusInfo[AUCTION_HOUSE_ALLIANCE].ItemsCount,
            statusInfo[AUCTION_HOUSE_HORDE].ItemsCount,
            statusInfo[AUCTION_HOUSE_NEUTRAL].ItemsCount,
            statusInfo[AUCTION_HOUSE_ALLIANCE].ItemsCount +
            statusInfo[AUCTION_HOUSE_HORDE].ItemsCount +
            statusInfo[AUCTION_HOUSE_NEUTRAL].ItemsCount);

        if (all)
        {
            handler->PSendSysMessage(fmtId, handler->GetTrinityString(LANG_AHBOT_STATUS_ITEM_RATIO),
                sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO),
                sAuctionBotConfig->GetConfig(CONFIG_AHBOT_HORDE_ITEM_AMOUNT_RATIO),
                sAuctionBotConfig->GetConfig(CONFIG_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO),
                sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ALLIANCE_ITEM_AMOUNT_RATIO) +
                sAuctionBotConfig->GetConfig(CONFIG_AHBOT_HORDE_ITEM_AMOUNT_RATIO) +
                sAuctionBotConfig->GetConfig(CONFIG_AHBOT_NEUTRAL_ITEM_AMOUNT_RATIO));

            if (!session)
            {
                handler->SendSysMessage(LANG_AHBOT_STATUS_BAR_CONSOLE);
                handler->SendSysMessage(LANG_AHBOT_STATUS_TITLE2_CONSOLE);
                handler->SendSysMessage(LANG_AHBOT_STATUS_MIDBAR_CONSOLE);
            }
            else
                handler->SendSysMessage(LANG_AHBOT_STATUS_TITLE2_CHAT);

            for (AuctionQuality quality : EnumUtils::Iterate<AuctionQuality>())
                handler->PSendSysMessage(fmtId, handler->GetTrinityString(ahbotQualityLangIds.at(quality)),
                    statusInfo[AUCTION_HOUSE_ALLIANCE].QualityInfo.at(quality),
                    statusInfo[AUCTION_HOUSE_HORDE].QualityInfo.at(quality),
                    statusInfo[AUCTION_HOUSE_NEUTRAL].QualityInfo.at(quality),
                    sAuctionBotConfig->GetConfigItemQualityAmount(quality));
        }

        if (!session)
            handler->SendSysMessage(LANG_AHBOT_STATUS_BAR_CONSOLE);

        return true;
    }

    // AHBot Stats Command - Reports item level distribution for auction house items
    // This is useful for analyzing the effectiveness of item level scaling
    // Usage: .ahbot stats [equipment] - shows item level distribution
    //        equipment flag filters to only weapons/armor
    static bool HandleAHBotStatsCommand(ChatHandler* handler, Optional<EXACT_SEQUENCE("equipment")> equipmentOnly)
    {
        // Item level buckets for distribution analysis
        std::map<uint32, uint32> itemLevelBuckets;  // bucket -> count
        uint32 totalItems = 0;
        uint32 equipmentItems = 0;
        uint32 minItemLevel = UINT32_MAX;
        uint32 maxItemLevel = 0;
        uint64 totalItemLevel = 0;

        // Iterate through all three auction houses
        // AuctionHouseIds: 1=Alliance, 2=Neutral (Goblin), 6=Horde, 7=Neutral (Blackwater)
        std::vector<uint32> auctionHouseIds = { 1, 2, 6, 7 };

        for (uint32 ahId : auctionHouseIds)
        {
            AuctionHouseObject* auctionHouse = sAuctionMgr->GetAuctionsById(ahId);
            if (!auctionHouse)
                continue;

            for (auto itr = auctionHouse->GetAuctionsBegin(); itr != auctionHouse->GetAuctionsEnd(); ++itr)
            {
                AuctionPosting& auction = itr->second;
                for (Item* item : auction.Items)
                {
                    if (!item)
                        continue;

                    ItemTemplate const* proto = item->GetTemplate();
                    if (!proto)
                        continue;

                    bool isEquipment = (proto->GetClass() == ITEM_CLASS_WEAPON || proto->GetClass() == ITEM_CLASS_ARMOR);

                    // Skip non-equipment if filter is active
                    if (equipmentOnly.has_value() && !isEquipment)
                        continue;

                    if (isEquipment)
                        ++equipmentItems;

                    // Get item level from bonus data (includes any scaling bonuses)
                    // Use static GetItemLevel with minimal parameters since AHBot items have no owner
                    uint32 itemLevel = Item::GetItemLevel(proto, *item->GetBonus(), 90, 0, 0, 0, 0, false, 0);

                    ++totalItems;
                    totalItemLevel += itemLevel;

                    if (itemLevel < minItemLevel)
                        minItemLevel = itemLevel;
                    if (itemLevel > maxItemLevel)
                        maxItemLevel = itemLevel;

                    // Bucket by 50 item levels for readable output
                    uint32 bucket = (itemLevel / 50) * 50;
                    itemLevelBuckets[bucket]++;
                }
            }
        }

        if (totalItems == 0)
        {
            handler->SendSysMessage("AHBot Stats: No items found in auction house.");
            return true;
        }

        // Output results
        handler->PSendSysMessage("=== AHBot Item Level Statistics ===");
        handler->PSendSysMessage("Total items analyzed: %u", totalItems);
        if (!equipmentOnly.has_value())
            handler->PSendSysMessage("Equipment items (weapons/armor): %u", equipmentItems);
        handler->PSendSysMessage("Item level range: %u - %u", minItemLevel, maxItemLevel);
        handler->PSendSysMessage("Average item level: %.1f", static_cast<float>(totalItemLevel) / totalItems);
        handler->SendSysMessage("--- Distribution by Item Level ---");

        for (auto const& [bucket, count] : itemLevelBuckets)
        {
            float percentage = (static_cast<float>(count) / totalItems) * 100.0f;
            handler->PSendSysMessage("  ilvl %u-%u: %u items (%.1f%%)", bucket, bucket + 49, count, percentage);
        }

        // Show scaling config status
        handler->SendSysMessage("--- Scaling Configuration ---");
        handler->PSendSysMessage("Scaling Enabled: %s", sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_SCALING_ENABLED) ? "Yes" : "No");
        if (sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_SCALING_ENABLED))
        {
            handler->PSendSysMessage("  Min Target ilvl: %u", sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_SCALING_MIN_ITEM_LEVEL));
            handler->PSendSysMessage("  Max Target ilvl: %u", sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_SCALING_MAX_ITEM_LEVEL));
            handler->PSendSysMessage("  Scaling Chance: %u%%", sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_SCALING_CHANCE));
            handler->PSendSysMessage("  Equipment Only: %s", sAuctionBotConfig->GetConfig(CONFIG_AHBOT_ITEM_SCALING_EQUIPMENT_ONLY) ? "Yes" : "No");
        }

        return true;
    }

};

template bool ahbot_commandscript::HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_GRAY>(ChatHandler* handler, uint32 amount);
template bool ahbot_commandscript::HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_WHITE>(ChatHandler* handler, uint32 amount);
template bool ahbot_commandscript::HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_GREEN>(ChatHandler* handler, uint32 amount);
template bool ahbot_commandscript::HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_BLUE>(ChatHandler* handler, uint32 amount);
template bool ahbot_commandscript::HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_PURPLE>(ChatHandler* handler, uint32 amount);
template bool ahbot_commandscript::HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_ORANGE>(ChatHandler* handler, uint32 amount);
template bool ahbot_commandscript::HandleAHBotItemsAmountQualityCommand<AUCTION_QUALITY_YELLOW>(ChatHandler* handler, uint32 amount);

template bool ahbot_commandscript::HandleAHBotItemsRatioHouseCommand<AUCTION_HOUSE_ALLIANCE>(ChatHandler* handler, uint32 ratio);
template bool ahbot_commandscript::HandleAHBotItemsRatioHouseCommand<AUCTION_HOUSE_HORDE>(ChatHandler* handler, uint32 ratio);
template bool ahbot_commandscript::HandleAHBotItemsRatioHouseCommand<AUCTION_HOUSE_NEUTRAL>(ChatHandler* handler, uint32 ratio);

void AddSC_ahbot_commandscript()
{
    new ahbot_commandscript();
}
