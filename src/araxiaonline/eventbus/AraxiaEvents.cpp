/*
 * Araxia Event Bus - Event Implementation
 * 
 * Implements the IsEnabled() methods for each event type.
 * These are in a separate .cpp to avoid circular dependencies with config.
 */

#include "AraxiaEvents.h"
#include "AraxiaEventBusConfig.h"

bool SpawnEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsSpawnEventsEnabled();
}

bool PlayerEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsPlayerEventsEnabled();
}

bool EncounterEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsEncounterEventsEnabled();
}

bool CombatEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsCombatEventsEnabled();
}

bool LootEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsLootEventsEnabled();
}

bool QuestEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsQuestEventsEnabled();
}

bool ZoneChangeEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsZoneEventsEnabled();
}

bool PartyChangeEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsPartyEventsEnabled();
}

bool ItemEquipEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsItemEventsEnabled();
}

bool SpellCastEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsSpellEventsEnabled();
}

bool LevelUpEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsLevelEventsEnabled();
}

bool ChatEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsChatEventsEnabled();
}

bool AchievementEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsAchievementEventsEnabled();
}

bool AuctionEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsAuctionEventsEnabled();
}

bool MailEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsMailEventsEnabled();
}

bool TradeEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsTradeEventsEnabled();
}

bool GuildEvent::IsEnabled() const
{
    return sAraxiaEventBusConfig->IsGuildEventsEnabled();
}
