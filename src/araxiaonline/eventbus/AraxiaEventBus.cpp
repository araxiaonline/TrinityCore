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

#include "AraxiaEventBus.h"
#include "Log.h"
#include "Timer.h"

#include <zmq.hpp>
#include <sstream>
#include <chrono>

/*
 * AraxiaEventBus Implementation
 * 
 * Uses ZeroMQ PUB/SUB pattern for event distribution.
 * Publisher socket binds to pubEndpoint (default :5555)
 * Subscriber socket connects to subEndpoint (default :5556)
 * 
 * Thread safety:
 * - Main thread queues messages via Publish()
 * - Worker thread handles actual ZMQ I/O
 * - Update() processes inbound messages on main thread
 */

AraxiaEventBus::AraxiaEventBus() = default;

AraxiaEventBus::~AraxiaEventBus()
{
    Shutdown();
}

AraxiaEventBus* AraxiaEventBus::Instance()
{
    static AraxiaEventBus instance;
    return &instance;
}

bool AraxiaEventBus::Initialize(const std::string& pubEndpoint, const std::string& subEndpoint)
{
    if (_initialized)
    {
        TC_LOG_WARN("araxia.eventbus", "AraxiaEventBus already initialized");
        return true;
    }
    
    _pubEndpoint = pubEndpoint;
    _subEndpoint = subEndpoint;
    
    try
    {
        // Create ZMQ context with 1 I/O thread
        _context = std::make_unique<zmq::context_t>(1);
        
        // Create publisher socket
        _publisher = std::make_unique<zmq::socket_t>(*_context, zmq::socket_type::pub);
        _publisher->bind(_pubEndpoint);
        
        // Create subscriber socket
        _subscriber = std::make_unique<zmq::socket_t>(*_context, zmq::socket_type::sub);
        _subscriber->connect(_subEndpoint);
        
        // Subscribe to command.* topics by default
        _subscriber->set(zmq::sockopt::subscribe, "command.");
        
        // Set socket options for non-blocking receives
        _subscriber->set(zmq::sockopt::rcvtimeo, 100); // 100ms timeout
        
        TC_LOG_INFO("araxia.eventbus", "AraxiaEventBus initialized - pub: {}, sub: {}", 
                    _pubEndpoint, _subEndpoint);
        
        // Start worker thread
        _running = true;
        _workerThread = std::thread(&AraxiaEventBus::WorkerThread, this);
        
        _initialized = true;
        return true;
    }
    catch (const zmq::error_t& e)
    {
        TC_LOG_ERROR("araxia.eventbus", "Failed to initialize AraxiaEventBus: {}", e.what());
        return false;
    }
}

void AraxiaEventBus::Shutdown()
{
    if (!_initialized)
        return;
    
    TC_LOG_INFO("araxia.eventbus", "Shutting down AraxiaEventBus...");
    
    _running = false;
    
    if (_workerThread.joinable())
        _workerThread.join();
    
    _subscriber.reset();
    _publisher.reset();
    _context.reset();
    
    _initialized = false;
    
    TC_LOG_INFO("araxia.eventbus", "AraxiaEventBus shutdown complete");
}

std::string EventContext::ToJson() const
{
    std::ostringstream ss;
    ss << "{\"map_id\":" << MapId 
       << ",\"instance_id\":" << InstanceId
       << ",\"difficulty\":" << Difficulty
       << ",\"zone_id\":" << ZoneId
       << ",\"type\":" << static_cast<int>(Type) << "}";
    return ss.str();
}

std::string AraxiaEventBus::BuildEnvelope(const std::string& topic, const EventContext& context, 
                                          const std::string& payload)
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    std::ostringstream ss;
    ss << "{\"v\":1"
       << ",\"topic\":\"" << topic << "\""
       << ",\"ts\":" << ms
       << ",\"source\":\"worldserver\""
       << ",\"context\":" << context.ToJson()
       << ",\"payload\":" << payload << "}";
    return ss.str();
}

void AraxiaEventBus::Publish(const std::string& topic, const std::string& jsonPayload)
{
    EventContext ctx;
    Publish(topic, ctx, jsonPayload);
}

void AraxiaEventBus::Publish(const std::string& topic, const EventContext& context, 
                             const std::string& jsonPayload)
{
    if (!_initialized)
        return;
    
    std::string envelope = BuildEnvelope(topic, context, jsonPayload);
    
    // Queue for worker thread
    {
        std::lock_guard<std::mutex> lock(_outboundMutex);
        _outboundQueue.push({topic, envelope});
    }
}

std::string AraxiaEventBus::ContentTypeToPrefix(ContentType type)
{
    switch (type)
    {
        case ContentType::World:        return "world";
        case ContentType::Dungeon:      return "dungeon";
        case ContentType::Raid:         return "raid";
        case ContentType::Battleground: return "bg";
        case ContentType::Arena:        return "arena";
        default:                        return "world";
    }
}

void AraxiaEventBus::PublishSpawnEvent(ContentType type, bool isCreate, uint64 guid, uint32 entry,
                                       uint32 mapId, uint32 instanceId, float x, float y, float z)
{
    std::string prefix = ContentTypeToPrefix(type);
    std::string topic = prefix + ".spawn." + (isCreate ? "create" : "delete");
    
    // Debug: Log spawn events to verify they're being called
    TC_LOG_DEBUG("araxia.eventbus", "PublishSpawnEvent: {} entry={} guid={} map={}", 
                 topic, entry, guid, mapId);
    
    EventContext ctx;
    ctx.MapId = mapId;
    ctx.InstanceId = instanceId;
    ctx.Type = type;
    
    std::ostringstream payload;
    payload << "{\"guid\":" << guid
            << ",\"entry\":" << entry
            << ",\"x\":" << x
            << ",\"y\":" << y
            << ",\"z\":" << z << "}";
    
    Publish(topic, ctx, payload.str());
}

void AraxiaEventBus::PublishEncounterEvent(ContentType type, const std::string& eventType,
                                           uint32 encounterId, uint32 mapId, uint32 instanceId,
                                           const std::string& extraJson)
{
    std::string prefix = ContentTypeToPrefix(type);
    std::string topic = prefix + ".encounter." + eventType;
    
    EventContext ctx;
    ctx.MapId = mapId;
    ctx.InstanceId = instanceId;
    ctx.Type = type;
    
    std::ostringstream payload;
    payload << "{\"encounter_id\":" << encounterId;
    
    // Merge extra JSON if provided
    if (extraJson.length() > 2) // More than just "{}"
    {
        // Strip leading { from extraJson and append
        payload << "," << extraJson.substr(1);
    }
    else
    {
        payload << "}";
    }
    
    Publish(topic, ctx, payload.str());
}

void AraxiaEventBus::PublishPlayerEvent(const std::string& eventType, uint64 playerGuid,
                                        const std::string& playerName, const EventContext& context)
{
    std::string prefix = ContentTypeToPrefix(context.Type);
    std::string topic = prefix + ".player." + eventType;
    
    std::ostringstream payload;
    payload << "{\"player_guid\":" << playerGuid
            << ",\"player_name\":\"" << playerName << "\"}";
    
    Publish(topic, context, payload.str());
}

void AraxiaEventBus::Subscribe(const std::string& topicPrefix, EventHandler handler)
{
    std::lock_guard<std::mutex> lock(_handlersMutex);
    _handlers.push_back({topicPrefix, handler});
    
    // Also subscribe at ZMQ level
    if (_subscriber)
    {
        try
        {
            _subscriber->set(zmq::sockopt::subscribe, topicPrefix);
        }
        catch (const zmq::error_t& e)
        {
            TC_LOG_ERROR("araxia.eventbus", "Failed to subscribe to {}: {}", topicPrefix, e.what());
        }
    }
}

void AraxiaEventBus::Unsubscribe(const std::string& topicPrefix)
{
    std::lock_guard<std::mutex> lock(_handlersMutex);
    _handlers.erase(
        std::remove_if(_handlers.begin(), _handlers.end(),
            [&topicPrefix](const auto& pair) { return pair.first == topicPrefix; }),
        _handlers.end());
    
    // Also unsubscribe at ZMQ level
    if (_subscriber)
    {
        try
        {
            _subscriber->set(zmq::sockopt::unsubscribe, topicPrefix);
        }
        catch (const zmq::error_t& e)
        {
            TC_LOG_ERROR("araxia.eventbus", "Failed to unsubscribe from {}: {}", topicPrefix, e.what());
        }
    }
}

void AraxiaEventBus::WorkerThread()
{
    TC_LOG_INFO("araxia.eventbus", "Event bus worker thread started");
    
    while (_running)
    {
        // Process outbound messages
        {
            std::lock_guard<std::mutex> lock(_outboundMutex);
            while (!_outboundQueue.empty())
            {
                auto& msg = _outboundQueue.front();
                try
                {
                    // ZMQ PUB sends topic as first frame
                    std::string fullMsg = msg.first + " " + msg.second;
                    zmq::message_t zmqMsg(fullMsg.data(), fullMsg.size());
                    _publisher->send(zmqMsg, zmq::send_flags::none);
                    TC_LOG_DEBUG("araxia.eventbus", "Published: {} ({} bytes)", msg.first, fullMsg.size());
                }
                catch (const zmq::error_t& e)
                {
                    TC_LOG_ERROR("araxia.eventbus", "Failed to publish message: {}", e.what());
                }
                _outboundQueue.pop();
            }
        }
        
        // Process inbound messages
        ProcessInboundMessages();
        
        // Small sleep to prevent busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    TC_LOG_INFO("araxia.eventbus", "Event bus worker thread stopped");
}

void AraxiaEventBus::ProcessInboundMessages()
{
    zmq::message_t msg;
    
    while (true)
    {
        try
        {
            auto result = _subscriber->recv(msg, zmq::recv_flags::dontwait);
            if (!result)
                break;
            
            std::string data(static_cast<char*>(msg.data()), msg.size());
            
            // Parse topic from message (format: "topic payload")
            size_t spacePos = data.find(' ');
            if (spacePos != std::string::npos)
            {
                std::string topic = data.substr(0, spacePos);
                std::string payload = data.substr(spacePos + 1);
                
                // Queue for main thread processing
                std::lock_guard<std::mutex> lock(_inboundMutex);
                _inboundQueue.push({topic, payload});
            }
        }
        catch (const zmq::error_t& e)
        {
            if (e.num() != EAGAIN)
                TC_LOG_ERROR("araxia.eventbus", "Error receiving message: {}", e.what());
            break;
        }
    }
}

void AraxiaEventBus::Update(uint32 /*diff*/)
{
    if (!_initialized)
        return;
    
    // Process inbound messages on main thread
    std::vector<std::pair<std::string, std::string>> messages;
    {
        std::lock_guard<std::mutex> lock(_inboundMutex);
        while (!_inboundQueue.empty())
        {
            messages.push_back(_inboundQueue.front());
            _inboundQueue.pop();
        }
    }
    
    // Dispatch to handlers
    std::lock_guard<std::mutex> lock(_handlersMutex);
    for (const auto& msg : messages)
    {
        for (const auto& handler : _handlers)
        {
            // Check if topic matches handler prefix
            if (msg.first.compare(0, handler.first.length(), handler.first) == 0)
            {
                try
                {
                    handler.second(msg.first, msg.second);
                }
                catch (const std::exception& e)
                {
                    TC_LOG_ERROR("araxia.eventbus", "Handler exception for topic {}: {}", 
                                 msg.first, e.what());
                }
            }
        }
    }
}

ContentType AraxiaEventBus::GetContentTypeForMap(uint32 mapId)
{
    // Use callback to resolve map type (set by game code with DB2 access)
    // This decouples the eventbus library from game-specific DB2 stores
    if (_mapTypeResolver)
        return _mapTypeResolver(mapId);
    
    // Fallback to World if no resolver set
    return ContentType::World;
}
