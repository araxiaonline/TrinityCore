/*
 * Araxia Event Bus - Event Interface Definitions
 * 
 * This file defines the event interface pattern for the Araxia Event Bus.
 * Events implement IAraxiaEvent and provide their own topic, config check,
 * and JSON serialization. This allows call sites to simply call:
 *     sAraxiaEventBus->Publish(SpawnEvent(...));
 * 
 * The EventBus handles initialization checks and delegates config checks
 * to each event type via IsEnabled().
 */

#ifndef ARAXIA_EVENTS_H
#define ARAXIA_EVENTS_H

#include "Define.h"
#include <string>
#include <sstream>
#include <iomanip>

// Forward declare to avoid circular dependency
class AraxiaEventBusConfig;

// Content type determines topic prefix (world, dungeon, raid, bg, arena)
enum class ContentType : uint8
{
    World = 0,
    Dungeon = 1,
    Raid = 2,
    Battleground = 3,
    Arena = 4
};

// Event context - map/instance information for filtering
struct TC_GAME_API EventContext
{
    uint32 MapId = 0;
    uint32 InstanceId = 0;
    uint32 Difficulty = 0;
    uint32 ZoneId = 0;
    ContentType Type = ContentType::World;
    
    std::string ToJson() const;
};

/*
 * Base interface for all Araxia events.
 * Each event type implements this to define its topic, payload, and config check.
 */
class IAraxiaEvent
{
public:
    virtual ~IAraxiaEvent() = default;
    
    // Returns the full topic string (e.g., "world.player.login")
    virtual std::string GetTopic() const = 0;
    
    // Returns the JSON payload for this event
    virtual std::string GetPayload() const = 0;
    
    // Returns the event context (map, instance, content type)
    virtual EventContext GetContext() const = 0;
    
    // Returns true if this event type is enabled in config
    virtual bool IsEnabled() const = 0;
};

/*
 * Helper to convert ContentType to topic prefix
 */
inline std::string ContentTypeToPrefix(ContentType type)
{
    switch (type)
    {
        case ContentType::Dungeon:     return "dungeon";
        case ContentType::Raid:        return "raid";
        case ContentType::Battleground: return "bg";
        case ContentType::Arena:       return "arena";
        default:                       return "world";
    }
}

// ============================================================================
// Concrete Event Types
// ============================================================================

/*
 * Spawn/Despawn event for creatures
 */
class SpawnEvent : public IAraxiaEvent
{
public:
    SpawnEvent(bool isCreate, uint64 guid, uint32 entry, 
               uint32 mapId, uint32 instanceId, ContentType type,
               float x, float y, float z)
        : _isCreate(isCreate), _guid(guid), _entry(entry)
        , _mapId(mapId), _instanceId(instanceId), _type(type)
        , _x(x), _y(y), _z(z) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".spawn." + (_isCreate ? "create" : "delete");
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"guid\":" << _guid
           << ",\"entry\":" << _entry
           << ",\"x\":" << _x
           << ",\"y\":" << _y
           << ",\"z\":" << _z << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;  // Implemented in .cpp to access config
    
private:
    bool _isCreate;
    uint64 _guid;
    uint32 _entry;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
    float _x, _y, _z;
};

/*
 * Player event (login, logout, death, etc.)
 */
class PlayerEvent : public IAraxiaEvent
{
public:
    PlayerEvent(const std::string& eventType, uint64 playerGuid, 
                const std::string& playerName,
                uint32 mapId, uint32 instanceId, ContentType type)
        : _eventType(eventType), _playerGuid(playerGuid), _playerName(playerName)
        , _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".player." + _eventType;
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\"}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;  // Implemented in .cpp to access config
    
private:
    std::string _eventType;
    uint64 _playerGuid;
    std::string _playerName;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Encounter event (start, end, wipe, etc.)
 */
class EncounterEvent : public IAraxiaEvent
{
public:
    EncounterEvent(const std::string& eventType, uint32 encounterId,
                   uint32 mapId, uint32 instanceId, ContentType type,
                   const std::string& extraJson = "{}")
        : _eventType(eventType), _encounterId(encounterId)
        , _mapId(mapId), _instanceId(instanceId), _type(type)
        , _extraJson(extraJson) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".encounter." + _eventType;
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"encounter_id\":" << _encounterId;
        
        // Merge extra JSON if provided
        if (_extraJson.length() > 2)  // More than just "{}"
        {
            ss << "," << _extraJson.substr(1);  // Strip leading {
        }
        else
        {
            ss << "}";
        }
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;  // Implemented in .cpp to access config
    
private:
    std::string _eventType;
    uint32 _encounterId;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
    std::string _extraJson;
};

/*
 * Combat event (enter, leave)
 * Fired when a player enters or exits combat
 */
class CombatEvent : public IAraxiaEvent
{
public:
    CombatEvent(bool isEntering, uint64 playerGuid, const std::string& playerName,
                uint32 mapId, uint32 instanceId, ContentType type)
        : _isEntering(isEntering), _playerGuid(playerGuid), _playerName(playerName)
        , _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".combat." + (_isEntering ? "enter" : "leave");
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\"}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;  // Implemented in .cpp to access config
    
private:
    bool _isEntering;
    uint64 _playerGuid;
    std::string _playerName;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Loot event (item looted by player)
 * Fired when a player loots an item from a corpse, chest, etc.
 */
class LootEvent : public IAraxiaEvent
{
public:
    LootEvent(uint64 playerGuid, const std::string& playerName,
              uint32 itemId, uint32 itemCount, uint32 sourceEntry,
              uint32 mapId, uint32 instanceId, ContentType type)
        : _playerGuid(playerGuid), _playerName(playerName)
        , _itemId(itemId), _itemCount(itemCount), _sourceEntry(sourceEntry)
        , _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".loot.item";
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\""
           << ",\"item_id\":" << _itemId
           << ",\"item_count\":" << _itemCount
           << ",\"source_entry\":" << _sourceEntry << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;  // Implemented in .cpp to access config
    
private:
    uint64 _playerGuid;
    std::string _playerName;
    uint32 _itemId;
    uint32 _itemCount;
    uint32 _sourceEntry;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Quest event (accept, complete, abandon)
 * Fired when a player accepts, completes, or abandons a quest
 */
class QuestEvent : public IAraxiaEvent
{
public:
    QuestEvent(const std::string& eventType, uint64 playerGuid, 
               const std::string& playerName, uint32 questId,
               uint32 mapId, uint32 instanceId, ContentType type)
        : _eventType(eventType), _playerGuid(playerGuid), _playerName(playerName)
        , _questId(questId), _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".quest." + _eventType;
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\""
           << ",\"quest_id\":" << _questId << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;  // Implemented in .cpp to access config
    
private:
    std::string _eventType;
    uint64 _playerGuid;
    std::string _playerName;
    uint32 _questId;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Zone change event
 * Fired when a player enters a new zone or area
 */
class ZoneChangeEvent : public IAraxiaEvent
{
public:
    ZoneChangeEvent(uint64 playerGuid, const std::string& playerName,
                    uint32 newZoneId, uint32 newAreaId,
                    uint32 oldZoneId, uint32 oldAreaId,
                    uint32 mapId, uint32 instanceId, ContentType type)
        : _playerGuid(playerGuid), _playerName(playerName)
        , _newZoneId(newZoneId), _newAreaId(newAreaId)
        , _oldZoneId(oldZoneId), _oldAreaId(oldAreaId)
        , _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".zone.change";
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\""
           << ",\"new_zone_id\":" << _newZoneId
           << ",\"new_area_id\":" << _newAreaId
           << ",\"old_zone_id\":" << _oldZoneId
           << ",\"old_area_id\":" << _oldAreaId << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.ZoneId = _newZoneId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;
    
private:
    uint64 _playerGuid;
    std::string _playerName;
    uint32 _newZoneId;
    uint32 _newAreaId;
    uint32 _oldZoneId;
    uint32 _oldAreaId;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Party/Group change event
 * Fired when a player joins, leaves, or changes role in a group
 */
class PartyChangeEvent : public IAraxiaEvent
{
public:
    PartyChangeEvent(const std::string& eventType, uint64 playerGuid, 
                     const std::string& playerName, uint64 groupGuid,
                     uint32 mapId, uint32 instanceId, ContentType type)
        : _eventType(eventType), _playerGuid(playerGuid), _playerName(playerName)
        , _groupGuid(groupGuid), _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".party." + _eventType;
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\""
           << ",\"group_guid\":" << _groupGuid << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;
    
private:
    std::string _eventType;  // join, leave, disband, leader_change
    uint64 _playerGuid;
    std::string _playerName;
    uint64 _groupGuid;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Item equip/unequip event
 * Fired when a player equips or unequips an item
 */
class ItemEquipEvent : public IAraxiaEvent
{
public:
    ItemEquipEvent(bool isEquip, uint64 playerGuid, const std::string& playerName,
                   uint32 itemId, uint32 itemSlot,
                   uint32 mapId, uint32 instanceId, ContentType type)
        : _isEquip(isEquip), _playerGuid(playerGuid), _playerName(playerName)
        , _itemId(itemId), _itemSlot(itemSlot)
        , _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".item." + (_isEquip ? "equip" : "unequip");
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\""
           << ",\"item_id\":" << _itemId
           << ",\"slot\":" << _itemSlot << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;
    
private:
    bool _isEquip;
    uint64 _playerGuid;
    std::string _playerName;
    uint32 _itemId;
    uint32 _itemSlot;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Spell cast event
 * Fired when a player casts a spell
 */
class SpellCastEvent : public IAraxiaEvent
{
public:
    SpellCastEvent(uint64 playerGuid, const std::string& playerName,
                   uint32 spellId, uint64 targetGuid, bool targetIsPlayer,
                   uint32 mapId, uint32 instanceId, ContentType type)
        : _playerGuid(playerGuid), _playerName(playerName)
        , _spellId(spellId), _targetGuid(targetGuid), _targetIsPlayer(targetIsPlayer)
        , _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".spell.cast";
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\""
           << ",\"spell_id\":" << _spellId
           << ",\"target_guid\":" << _targetGuid
           << ",\"target_is_player\":" << (_targetIsPlayer ? "true" : "false") << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;
    
private:
    uint64 _playerGuid;
    std::string _playerName;
    uint32 _spellId;
    uint64 _targetGuid;
    bool _targetIsPlayer;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Level up event
 * Fired when a player gains a level
 */
class LevelUpEvent : public IAraxiaEvent
{
public:
    LevelUpEvent(uint64 playerGuid, const std::string& playerName,
                 uint32 oldLevel, uint32 newLevel,
                 uint32 mapId, uint32 instanceId, ContentType type)
        : _playerGuid(playerGuid), _playerName(playerName)
        , _oldLevel(oldLevel), _newLevel(newLevel)
        , _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".player.levelup";
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\""
           << ",\"old_level\":" << _oldLevel
           << ",\"new_level\":" << _newLevel << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;
    
private:
    uint64 _playerGuid;
    std::string _playerName;
    uint32 _oldLevel;
    uint32 _newLevel;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Chat event
 * Fired when a player sends a chat message
 */
class ChatEvent : public IAraxiaEvent
{
public:
    ChatEvent(const std::string& chatType, uint64 playerGuid, 
              const std::string& playerName, const std::string& message,
              uint64 targetGuid, const std::string& targetName,
              uint32 mapId, uint32 instanceId, ContentType type)
        : _chatType(chatType), _playerGuid(playerGuid), _playerName(playerName)
        , _message(message), _targetGuid(targetGuid), _targetName(targetName)
        , _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".chat." + _chatType;
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\""
           << ",\"message\":\"" << EscapeJson(_message) << "\"";
        if (_targetGuid != 0)
        {
            ss << ",\"target_guid\":" << _targetGuid
               << ",\"target_name\":\"" << _targetName << "\"";
        }
        ss << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;
    
private:
    // Helper to escape JSON special characters
    static std::string EscapeJson(const std::string& s)
    {
        std::ostringstream o;
        for (char c : s)
        {
            switch (c)
            {
                case '"': o << "\\\""; break;
                case '\\': o << "\\\\"; break;
                case '\b': o << "\\b"; break;
                case '\f': o << "\\f"; break;
                case '\n': o << "\\n"; break;
                case '\r': o << "\\r"; break;
                case '\t': o << "\\t"; break;
                default:
                    if ('\x00' <= c && c <= '\x1f')
                        o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                    else
                        o << c;
            }
        }
        return o.str();
    }
    
    std::string _chatType;  // say, yell, whisper, party, guild, raid
    uint64 _playerGuid;
    std::string _playerName;
    std::string _message;
    uint64 _targetGuid;
    std::string _targetName;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Achievement event
 * Fired when a player earns an achievement
 */
class AchievementEvent : public IAraxiaEvent
{
public:
    AchievementEvent(uint64 playerGuid, const std::string& playerName,
                     uint32 achievementId,
                     uint32 mapId, uint32 instanceId, ContentType type)
        : _playerGuid(playerGuid), _playerName(playerName)
        , _achievementId(achievementId)
        , _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".achievement.earned";
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\""
           << ",\"achievement_id\":" << _achievementId << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;
    
private:
    uint64 _playerGuid;
    std::string _playerName;
    uint32 _achievementId;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Auction event
 * Fired on auction house activity (create, bid, buyout, expire, cancel)
 */
class AuctionEvent : public IAraxiaEvent
{
public:
    AuctionEvent(const std::string& eventType, uint64 playerGuid, 
                 const std::string& playerName, uint32 auctionId,
                 uint32 itemId, uint32 itemCount, uint64 price,
                 uint32 mapId, uint32 instanceId, ContentType type)
        : _eventType(eventType), _playerGuid(playerGuid), _playerName(playerName)
        , _auctionId(auctionId), _itemId(itemId), _itemCount(itemCount), _price(price)
        , _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".auction." + _eventType;
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\""
           << ",\"auction_id\":" << _auctionId
           << ",\"item_id\":" << _itemId
           << ",\"item_count\":" << _itemCount
           << ",\"price\":" << _price << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;
    
private:
    std::string _eventType;  // create, bid, buyout, expire, cancel
    uint64 _playerGuid;
    std::string _playerName;
    uint32 _auctionId;
    uint32 _itemId;
    uint32 _itemCount;
    uint64 _price;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Mail event
 * Fired when a player sends or receives mail
 */
class MailEvent : public IAraxiaEvent
{
public:
    MailEvent(const std::string& eventType, uint64 playerGuid, 
              const std::string& playerName, uint64 targetGuid,
              const std::string& targetName, uint32 mailId,
              uint64 money, bool hasItems,
              uint32 mapId, uint32 instanceId, ContentType type)
        : _eventType(eventType), _playerGuid(playerGuid), _playerName(playerName)
        , _targetGuid(targetGuid), _targetName(targetName), _mailId(mailId)
        , _money(money), _hasItems(hasItems)
        , _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".mail." + _eventType;
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\""
           << ",\"target_guid\":" << _targetGuid
           << ",\"target_name\":\"" << _targetName << "\""
           << ",\"mail_id\":" << _mailId
           << ",\"money\":" << _money
           << ",\"has_items\":" << (_hasItems ? "true" : "false") << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;
    
private:
    std::string _eventType;  // send, receive
    uint64 _playerGuid;
    std::string _playerName;
    uint64 _targetGuid;
    std::string _targetName;
    uint32 _mailId;
    uint64 _money;
    bool _hasItems;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Trade event
 * Fired when players complete a trade
 */
class TradeEvent : public IAraxiaEvent
{
public:
    TradeEvent(uint64 player1Guid, const std::string& player1Name,
               uint64 player2Guid, const std::string& player2Name,
               uint64 player1Gold, uint64 player2Gold,
               uint32 player1ItemCount, uint32 player2ItemCount,
               uint32 mapId, uint32 instanceId, ContentType type)
        : _player1Guid(player1Guid), _player1Name(player1Name)
        , _player2Guid(player2Guid), _player2Name(player2Name)
        , _player1Gold(player1Gold), _player2Gold(player2Gold)
        , _player1ItemCount(player1ItemCount), _player2ItemCount(player2ItemCount)
        , _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".trade.complete";
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player1_guid\":" << _player1Guid
           << ",\"player1_name\":\"" << _player1Name << "\""
           << ",\"player2_guid\":" << _player2Guid
           << ",\"player2_name\":\"" << _player2Name << "\""
           << ",\"player1_gold\":" << _player1Gold
           << ",\"player2_gold\":" << _player2Gold
           << ",\"player1_item_count\":" << _player1ItemCount
           << ",\"player2_item_count\":" << _player2ItemCount << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;
    
private:
    uint64 _player1Guid;
    std::string _player1Name;
    uint64 _player2Guid;
    std::string _player2Name;
    uint64 _player1Gold;
    uint64 _player2Gold;
    uint32 _player1ItemCount;
    uint32 _player2ItemCount;
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

/*
 * Guild event
 * Fired on guild membership changes (join, leave, promote, demote, create, disband)
 */
class GuildEvent : public IAraxiaEvent
{
public:
    GuildEvent(const std::string& eventType, uint64 playerGuid, 
               const std::string& playerName, uint64 guildId,
               const std::string& guildName, uint32 newRank,
               uint32 mapId, uint32 instanceId, ContentType type)
        : _eventType(eventType), _playerGuid(playerGuid), _playerName(playerName)
        , _guildId(guildId), _guildName(guildName), _newRank(newRank)
        , _mapId(mapId), _instanceId(instanceId), _type(type) {}
    
    std::string GetTopic() const override
    {
        return ContentTypeToPrefix(_type) + ".guild." + _eventType;
    }
    
    std::string GetPayload() const override
    {
        std::ostringstream ss;
        ss << "{\"player_guid\":" << _playerGuid
           << ",\"player_name\":\"" << _playerName << "\""
           << ",\"guild_id\":" << _guildId
           << ",\"guild_name\":\"" << _guildName << "\"";
        if (_newRank != 0xFFFFFFFF)
            ss << ",\"new_rank\":" << _newRank;
        ss << "}";
        return ss.str();
    }
    
    EventContext GetContext() const override
    {
        EventContext ctx;
        ctx.MapId = _mapId;
        ctx.InstanceId = _instanceId;
        ctx.Type = _type;
        return ctx;
    }
    
    bool IsEnabled() const override;
    
private:
    std::string _eventType;  // join, leave, promote, demote, create, disband
    uint64 _playerGuid;
    std::string _playerName;
    uint64 _guildId;
    std::string _guildName;
    uint32 _newRank;  // 0xFFFFFFFF means not applicable
    uint32 _mapId;
    uint32 _instanceId;
    ContentType _type;
};

#endif // ARAXIA_EVENTS_H
