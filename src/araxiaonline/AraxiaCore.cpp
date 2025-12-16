/*
 * AraxiaCore Implementation
 */

#include "AraxiaCore.h"
#include "Log.h"

namespace Araxia
{

AraxiaCore* AraxiaCore::Instance()
{
    static AraxiaCore instance;
    return &instance;
}

void AraxiaCore::Initialize()
{
    if (_initialized)
        return;
    
    TC_LOG_INFO("araxia", "[AraxiaCore] Initializing Araxia systems hook...");
    _initialized = true;
}

void AraxiaCore::Shutdown()
{
    TC_LOG_INFO("araxia", "[AraxiaCore] Shutting down (%zu callbacks registered)...", _callbacks.size());
    
    std::lock_guard<std::mutex> lock(_callbackMutex);
    _callbacks.clear();
    _initialized = false;
}

void AraxiaCore::Update(uint32 diff)
{
    if (!_initialized)
        return;
    
    std::lock_guard<std::mutex> lock(_callbackMutex);
    
    for (auto& [name, callback] : _callbacks)
    {
        try
        {
            callback(diff);
        }
        catch (const std::exception& e)
        {
            TC_LOG_ERROR("araxia", "[AraxiaCore] Exception in callback '%s': %s", name.c_str(), e.what());
        }
    }
}

void AraxiaCore::RegisterUpdateCallback(const std::string& name, UpdateCallback callback)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    
    if (_callbacks.find(name) != _callbacks.end())
    {
        TC_LOG_WARN("araxia", "[AraxiaCore] Replacing existing callback: %s", name.c_str());
    }
    
    _callbacks[name] = std::move(callback);
    TC_LOG_INFO("araxia", "[AraxiaCore] Registered callback: %s (total: %zu)", name.c_str(), _callbacks.size());
}

void AraxiaCore::UnregisterUpdateCallback(const std::string& name)
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    
    auto it = _callbacks.find(name);
    if (it != _callbacks.end())
    {
        _callbacks.erase(it);
        TC_LOG_INFO("araxia", "[AraxiaCore] Unregistered callback: %s (remaining: %zu)", name.c_str(), _callbacks.size());
    }
}

bool AraxiaCore::HasCallback(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(_callbackMutex);
    return _callbacks.find(name) != _callbacks.end();
}

} // namespace Araxia
