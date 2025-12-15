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

#ifndef ARAXIA_EVENT_BUS_H
#define ARAXIA_EVENT_BUS_H

/*
 * AraxiaEventBus - ZeroMQ-based pub/sub event bus for server events
 * 
 * This provides real-time event publishing for:
 * - Spawn/despawn events (world, dungeon, raid, bg contexts)
 * - Encounter events (start, end, spell casts, phase transitions)
 * - Player events (login, logout, death)
 * - Command subscription (reload, announce, etc.)
 * 
 * Events are published with context-aware topics:
 *   world.spawn.create, dungeon.encounter.start, raid.encounter.spell_cast, etc.
 * 
 * Message format is JSON with envelope:
 *   { "v": 1, "topic": "...", "ts": ..., "source": "worldserver", "context": {...}, "payload": {...} }
 * 
 * See: openspec/changes/add-zeromq-event-bus/design.md
 */

#include "Define.h"
#include "AraxiaEvents.h"
#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

// Forward declare ZMQ types to avoid including zmq.hpp in header
namespace zmq {
    class context_t;
    class socket_t;
}

// Subscription handler callback
using EventHandler = std::function<void(const std::string& topic, const std::string& payload)>;

// Map type resolver callback - set by game code to provide DB2 lookup
using MapTypeResolver = std::function<ContentType(uint32 mapId)>;

class TC_GAME_API AraxiaEventBus
{
public:
    static AraxiaEventBus* Instance();
    
    // Lifecycle
    bool Initialize(const std::string& pubEndpoint, const std::string& subEndpoint);
    void Shutdown();
    bool IsInitialized() const { return _initialized; }
    
    // Publishing - event interface (preferred)
    // Usage: sAraxiaEventBus->Publish(SpawnEvent(...));
    // The event handles its own topic, payload, and config check
    void Publish(const IAraxiaEvent& event);
    
    // Publishing - generic (for custom events)
    void Publish(const std::string& topic, const std::string& jsonPayload);
    void Publish(const std::string& topic, const EventContext& context, const std::string& jsonPayload);
    
    // Subscribing
    void Subscribe(const std::string& topicPrefix, EventHandler handler);
    void Unsubscribe(const std::string& topicPrefix);
    
    // Processing - call from world update loop
    void Update(uint32 diff);
    
    // Utility
    static std::string ContentTypeToPrefix(ContentType type);
    ContentType GetContentTypeForMap(uint32 mapId);
    
    // Set map type resolver (called from game code with DB2 access)
    void SetMapTypeResolver(MapTypeResolver resolver) { _mapTypeResolver = resolver; }
    
private:
    AraxiaEventBus();
    ~AraxiaEventBus();
    
    // Prevent copying
    AraxiaEventBus(const AraxiaEventBus&) = delete;
    AraxiaEventBus& operator=(const AraxiaEventBus&) = delete;
    
    // Build message envelope
    std::string BuildEnvelope(const std::string& topic, const EventContext& context, 
                              const std::string& payload);
    
    // Background worker for ZMQ I/O
    void WorkerThread();
    void ProcessInboundMessages();
    
    // ZMQ resources
    std::unique_ptr<zmq::context_t> _context;
    std::unique_ptr<zmq::socket_t> _publisher;
    std::unique_ptr<zmq::socket_t> _subscriber;
    
    // Worker thread
    std::thread _workerThread;
    std::atomic<bool> _running{false};
    std::atomic<bool> _initialized{false};
    
    // Outbound message queue (main thread -> worker)
    std::mutex _outboundMutex;
    std::queue<std::pair<std::string, std::string>> _outboundQueue;
    
    // Inbound message queue (worker -> main thread)
    std::mutex _inboundMutex;
    std::queue<std::pair<std::string, std::string>> _inboundQueue;
    
    // Subscription handlers
    std::mutex _handlersMutex;
    std::vector<std::pair<std::string, EventHandler>> _handlers;
    
    // Endpoints
    std::string _pubEndpoint;
    std::string _subEndpoint;
    
    // Map type resolver callback
    MapTypeResolver _mapTypeResolver;
};

#define sAraxiaEventBus AraxiaEventBus::Instance()

#endif // ARAXIA_EVENT_BUS_H
