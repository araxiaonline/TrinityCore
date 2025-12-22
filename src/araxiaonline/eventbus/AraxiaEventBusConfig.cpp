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

#include "AraxiaEventBusConfig.h"
#include "Config.h"
#include "Log.h"

using namespace std::string_view_literals;

AraxiaEventBusConfig* AraxiaEventBusConfig::Instance()
{
    static AraxiaEventBusConfig instance;
    return &instance;
}

void AraxiaEventBusConfig::LoadConfig()
{
    // Use quiet=true to suppress "Missing name" warnings since we have sensible defaults
    // Config can be set in worldserver.conf or worldserver.conf.d/*.conf
    _publishEndpoint = sConfigMgr->GetStringDefault("Araxia.EventBus.PublishEndpoint"sv, "tcp://*:5555"sv, true);
    _subscribeEndpoint = sConfigMgr->GetStringDefault("Araxia.EventBus.SubscribeEndpoint"sv, "tcp://127.0.0.1:5556"sv, true);
    _enableSpawnEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableSpawnEvents"sv, true, true);
    _enableEncounterEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableEncounterEvents"sv, true, true);
    _enablePlayerEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnablePlayerEvents"sv, true, true);
    _enableCombatEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableCombatEvents"sv, true, true);
    _enableLootEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableLootEvents"sv, true, true);
    _enableQuestEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableQuestEvents"sv, true, true);
    _enableZoneEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableZoneEvents"sv, true, true);
    _enablePartyEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnablePartyEvents"sv, true, true);
    _enableItemEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableItemEvents"sv, true, true);
    _enableSpellEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableSpellEvents"sv, false, true);  // High volume - disabled by default
    _enableLevelEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableLevelEvents"sv, true, true);
    _enableChatEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableChatEvents"sv, true, true);
    _enableAchievementEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableAchievementEvents"sv, true, true);
    _enableAuctionEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableAuctionEvents"sv, true, true);
    _enableMailEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableMailEvents"sv, true, true);
    _enableTradeEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableTradeEvents"sv, true, true);
    _enableGuildEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableGuildEvents"sv, true, true);
    
    TC_LOG_INFO("araxia.eventbus", "EventBus config loaded:");
    TC_LOG_INFO("araxia.eventbus", "  PublishEndpoint: {}", _publishEndpoint);
    TC_LOG_INFO("araxia.eventbus", "  SubscribeEndpoint: {}", _subscribeEndpoint);
    TC_LOG_INFO("araxia.eventbus", "  SpawnEvents: {}", _enableSpawnEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  EncounterEvents: {}", _enableEncounterEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  PlayerEvents: {}", _enablePlayerEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  CombatEvents: {}", _enableCombatEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  LootEvents: {}", _enableLootEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  QuestEvents: {}", _enableQuestEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  ZoneEvents: {}", _enableZoneEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  PartyEvents: {}", _enablePartyEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  ItemEvents: {}", _enableItemEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  SpellEvents: {}", _enableSpellEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  LevelEvents: {}", _enableLevelEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  ChatEvents: {}", _enableChatEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  AchievementEvents: {}", _enableAchievementEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  AuctionEvents: {}", _enableAuctionEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  MailEvents: {}", _enableMailEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  TradeEvents: {}", _enableTradeEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  GuildEvents: {}", _enableGuildEvents ? "enabled" : "disabled");
}
