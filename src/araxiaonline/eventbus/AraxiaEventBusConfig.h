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

#ifndef ARAXIA_EVENT_BUS_CONFIG_H
#define ARAXIA_EVENT_BUS_CONFIG_H

/*
 * AraxiaEventBusConfig - Configuration loading for ZeroMQ event bus
 * 
 * Reads settings from worldserver.conf:
 *   Araxia.EventBus.PublishEndpoint
 *   Araxia.EventBus.SubscribeEndpoint
 *   Araxia.EventBus.Enable*Events (Spawn, Encounter, Player, Combat, Loot, Quest,
 *                                  Zone, Party, Item, Spell, Level, Chat,
 *                                  Achievement, Auction, Mail, Trade, Guild)
 */

#include "Define.h"
#include <string>

class TC_GAME_API AraxiaEventBusConfig
{
public:
    static AraxiaEventBusConfig* Instance();
    
    // Load configuration from worldserver.conf
    void LoadConfig();
    
    // Getters
    std::string GetPublishEndpoint() const { return _publishEndpoint; }
    std::string GetSubscribeEndpoint() const { return _subscribeEndpoint; }
    bool IsSpawnEventsEnabled() const { return _enableSpawnEvents; }
    bool IsEncounterEventsEnabled() const { return _enableEncounterEvents; }
    bool IsPlayerEventsEnabled() const { return _enablePlayerEvents; }
    bool IsCombatEventsEnabled() const { return _enableCombatEvents; }
    bool IsLootEventsEnabled() const { return _enableLootEvents; }
    bool IsQuestEventsEnabled() const { return _enableQuestEvents; }
    bool IsZoneEventsEnabled() const { return _enableZoneEvents; }
    bool IsPartyEventsEnabled() const { return _enablePartyEvents; }
    bool IsItemEventsEnabled() const { return _enableItemEvents; }
    bool IsSpellEventsEnabled() const { return _enableSpellEvents; }
    bool IsLevelEventsEnabled() const { return _enableLevelEvents; }
    bool IsChatEventsEnabled() const { return _enableChatEvents; }
    bool IsAchievementEventsEnabled() const { return _enableAchievementEvents; }
    bool IsAuctionEventsEnabled() const { return _enableAuctionEvents; }
    bool IsMailEventsEnabled() const { return _enableMailEvents; }
    bool IsTradeEventsEnabled() const { return _enableTradeEvents; }
    bool IsGuildEventsEnabled() const { return _enableGuildEvents; }
    
private:
    AraxiaEventBusConfig() = default;
    
    std::string _publishEndpoint = "tcp://*:5555";
    std::string _subscribeEndpoint = "tcp://127.0.0.1:5556";
    bool _enableSpawnEvents = true;
    bool _enableEncounterEvents = true;
    bool _enablePlayerEvents = true;
    bool _enableCombatEvents = true;
    bool _enableLootEvents = true;
    bool _enableQuestEvents = true;
    bool _enableZoneEvents = true;
    bool _enablePartyEvents = true;
    bool _enableItemEvents = true;
    bool _enableSpellEvents = false;  // Disabled by default - very high volume
    bool _enableLevelEvents = true;
    bool _enableChatEvents = true;
    bool _enableAchievementEvents = true;
    bool _enableAuctionEvents = true;
    bool _enableMailEvents = true;
    bool _enableTradeEvents = true;
    bool _enableGuildEvents = true;
};

#define sAraxiaEventBusConfig AraxiaEventBusConfig::Instance()

#endif // ARAXIA_EVENT_BUS_CONFIG_H
