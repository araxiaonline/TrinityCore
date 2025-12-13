/*
 * AraxiaCore - Central update hook for all Araxia systems
 * 
 * This singleton provides a single integration point with World::Update().
 * Other Araxia systems register callbacks here instead of each needing
 * their own hook into the core engine.
 * 
 * Usage:
 *   // During initialization
 *   sAraxiaCore->RegisterUpdateCallback("MySystem", [](uint32 diff) {
 *       // Called every world update tick
 *   });
 *   
 *   // During shutdown
 *   sAraxiaCore->UnregisterUpdateCallback("MySystem");
 */

#ifndef ARAXIA_CORE_H
#define ARAXIA_CORE_H

#include "Define.h"
#include <functional>
#include <string>
#include <map>
#include <mutex>

namespace Araxia
{

// Callback signature for update hooks
using UpdateCallback = std::function<void(uint32 diff)>;

/**
 * AraxiaCore - Singleton that provides World::Update hooks for Araxia systems.
 * 
 * Systems can register callbacks that will be called every world tick.
 * This is useful for:
 * - Processing async database callbacks
 * - Timed events
 * - Session management
 * - Any periodic updates
 */
class TC_GAME_API AraxiaCore
{
public:
    static AraxiaCore* Instance();
    
    // Lifecycle
    void Initialize();
    void Shutdown();
    
    // Called from World::Update() - dispatches to all registered callbacks
    void Update(uint32 diff);
    
    // Callback registration
    void RegisterUpdateCallback(const std::string& name, UpdateCallback callback);
    void UnregisterUpdateCallback(const std::string& name);
    bool HasCallback(const std::string& name) const;
    
    // Stats
    size_t GetCallbackCount() const { return _callbacks.size(); }
    
private:
    AraxiaCore() = default;
    ~AraxiaCore() = default;
    
    std::map<std::string, UpdateCallback> _callbacks;
    mutable std::mutex _callbackMutex;
    bool _initialized{false};
};

} // namespace Araxia

#define sAraxiaCore Araxia::AraxiaCore::Instance()

#endif // ARAXIA_CORE_H
