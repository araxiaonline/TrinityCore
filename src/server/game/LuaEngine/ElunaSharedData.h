/*
 * Copyright (C) 2024 Araxia Online <https://araxiaonline.com>
 * Custom Eluna extension for cross-state data sharing
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef ELUNA_SHARED_DATA_H
#define ELUNA_SHARED_DATA_H

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <vector>

/**
 * @brief Thread-safe shared data registry for cross-Eluna-state communication.
 *
 * Eluna creates isolated Lua environments for the world state and each map instance.
 * This singleton provides a C++-backed storage that all Lua states can access,
 * enabling features like multi-part message reassembly across state boundaries.
 *
 * Data is stored as serialized strings (using lmarshal) to avoid Lua state conflicts.
 */
class ElunaSharedData
{
public:
    /**
     * @brief Get the singleton instance.
     * @return Pointer to the shared data instance.
     */
    static ElunaSharedData* instance();

    /**
     * @brief Store a serialized value with the given key.
     * @param key Unique identifier for the data.
     * @param serializedValue The value serialized via lmarshal.
     */
    void Set(const std::string& key, const std::string& serializedValue);

    /**
     * @brief Retrieve a serialized value by key.
     * @param key Unique identifier for the data.
     * @param outValue Output parameter for the serialized value.
     * @return true if the key exists, false otherwise.
     */
    bool Get(const std::string& key, std::string& outValue) const;

    /**
     * @brief Check if a key exists in the registry.
     * @param key Unique identifier to check.
     * @return true if the key exists, false otherwise.
     */
    bool Has(const std::string& key) const;

    /**
     * @brief Remove a key from the registry.
     * @param key Unique identifier to remove.
     */
    void Clear(const std::string& key);

    /**
     * @brief Remove all keys from the registry.
     */
    void ClearAll();

    /**
     * @brief Get all keys currently in the registry.
     * @return Vector of all key names.
     */
    std::vector<std::string> GetKeys() const;

    /**
     * @brief Get the number of entries in the registry.
     * @return Number of key-value pairs stored.
     */
    size_t Size() const;

private:
    ElunaSharedData() = default;
    ~ElunaSharedData() = default;

    // Non-copyable
    ElunaSharedData(const ElunaSharedData&) = delete;
    ElunaSharedData& operator=(const ElunaSharedData&) = delete;

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::string> data_;
};

#define sElunaSharedData ElunaSharedData::instance()

#endif // ELUNA_SHARED_DATA_H
