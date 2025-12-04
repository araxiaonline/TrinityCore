# Araxia Event Bus Architecture

## Overview

A unified internal event bus that connects all major subsystems:
- **C++ Core** (TrinityCore)
- **Eluna** (Lua scripting)
- **MCP Server** (AI assistant)
- **AMS** (client addon communication)

## Goals

1. **Any component can publish events** - DB changes, player actions, errors, etc.
2. **Any component can subscribe** - MCP sees player targets, Eluna sees MCP commands
3. **Decoupled architecture** - Components don't need to know about each other
4. **Audit trail** - All events logged for debugging
5. **Real-time** - Low latency for interactive debugging

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         EVENT BUS (C++)                              │
│                                                                      │
│  EventBus::Publish("player.target.changed", {guid, name, entry})    │
│  EventBus::Subscribe("player.*", callback)                          │
│                                                                      │
├─────────────┬─────────────┬─────────────┬─────────────┬─────────────┤
│   C++ Core  │    Eluna    │     MCP     │     AMS     │   Logging   │
│             │             │             │             │             │
│ Publishes:  │ Publishes:  │ Publishes:  │ Publishes:  │ Subscribes: │
│ - DB errors │ - Script    │ - AI cmds   │ - Client    │ - All events│
│ - Spawns    │   events    │ - Queries   │   messages  │             │
│ - Combat    │ - Handlers  │             │ - Target    │             │
│             │             │             │   changes   │             │
│ Subscribes: │ Subscribes: │ Subscribes: │ Subscribes: │             │
│ - MCP cmds  │ - All       │ - All       │ - MCP msgs  │             │
└─────────────┴─────────────┴─────────────┴─────────────┴─────────────┘
```

## Event Categories

### Player Events
- `player.login` / `player.logout`
- `player.target.changed` - **Key for MCP to see your target!**
- `player.position.changed`
- `player.command` - GM commands executed

### Creature Events
- `creature.spawn` / `creature.despawn`
- `creature.property.changed` - Wander distance, movement type, etc.
- `creature.combat.start` / `creature.combat.end`

### Database Events
- `db.query.start` / `db.query.complete` / `db.query.error`
- `db.execute.start` / `db.execute.complete`

### MCP Events
- `mcp.tool.called` - AI called a tool
- `mcp.message.sent` - AI sent message to client
- `mcp.error` - MCP operation failed

### AMS Events
- `ams.message.received` - Client sent message
- `ams.message.sent` - Server sent to client
- `ams.handler.error` - Handler failed

## Implementation Phases

### Phase 1: Core Infrastructure
```cpp
// EventBus.h
class EventBus {
public:
    static void Publish(const std::string& event, const json& data);
    static void Subscribe(const std::string& pattern, EventCallback callback);
    static void Unsubscribe(const std::string& pattern);
    
    // Store recent events for MCP to query
    static std::vector<Event> GetRecentEvents(size_t count = 100);
};
```

### Phase 2: Eluna Integration
```lua
-- Lua API
EventBus.Publish("custom.event", {data = "value"})
EventBus.Subscribe("player.*", function(event, data)
    print("Player event:", event, data.name)
end)
```

### Phase 3: MCP Integration
New MCP tools:
- `event_subscribe` - Subscribe to event patterns
- `event_history` - Get recent events
- `event_publish` - Publish an event

### Phase 4: Auto-Publishing
Hook into TrinityCore to auto-publish events:
- Player target changes → `player.target.changed`
- Creature spawns → `creature.spawn`
- DB errors → `db.error`

## Example: MCP Sees Player Target

With the event bus:

```
Player targets Scarlet Sentry
    ↓
C++ Core publishes "player.target.changed"
    ↓
Event Bus routes to all subscribers
    ↓
MCP subscriber stores in ElunaSharedData
    ↓
AI calls shared_data_read("mcp_current_target")
    ↓
AI sees: {name: "Scarlet Sentry", entry: 4283, guid: "..."}
```

## Benefits

1. **No polling** - Events pushed in real-time
2. **Flexible** - Add new event types without changing core
3. **Debuggable** - Full event history for troubleshooting
4. **Extensible** - Easy to add new subscribers/publishers
5. **Unified** - One system instead of multiple ad-hoc bridges

## Storage

Events stored in `ElunaSharedData` for cross-component access:
- `event_bus_history` - Recent events (ring buffer, JSON array)
- `event_bus_subscriptions` - Active subscriptions
- `mcp_current_target` - Special: current player target for MCP

## Security

- Event data sanitized before storage
- Subscription patterns validated
- Rate limiting on publish
- Audit log of all events
