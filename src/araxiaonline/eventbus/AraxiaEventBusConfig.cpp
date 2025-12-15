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
    _publishEndpoint = sConfigMgr->GetStringDefault("Araxia.EventBus.PublishEndpoint"sv, "tcp://*:5555"sv);
    _subscribeEndpoint = sConfigMgr->GetStringDefault("Araxia.EventBus.SubscribeEndpoint"sv, "tcp://127.0.0.1:5556"sv);
    _enableSpawnEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableSpawnEvents"sv, true);
    _enableEncounterEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnableEncounterEvents"sv, true);
    _enablePlayerEvents = sConfigMgr->GetBoolDefault("Araxia.EventBus.EnablePlayerEvents"sv, true);
    
    TC_LOG_INFO("araxia.eventbus", "EventBus config loaded:");
    TC_LOG_INFO("araxia.eventbus", "  PublishEndpoint: {}", _publishEndpoint);
    TC_LOG_INFO("araxia.eventbus", "  SubscribeEndpoint: {}", _subscribeEndpoint);
    TC_LOG_INFO("araxia.eventbus", "  SpawnEvents: {}", _enableSpawnEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  EncounterEvents: {}", _enableEncounterEvents ? "enabled" : "disabled");
    TC_LOG_INFO("araxia.eventbus", "  PlayerEvents: {}", _enablePlayerEvents ? "enabled" : "disabled");
}
