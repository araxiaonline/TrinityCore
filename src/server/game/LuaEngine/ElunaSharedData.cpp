/*
 * Copyright (C) 2024 Araxia Online <https://araxiaonline.com>
 * Custom Eluna extension for cross-state data sharing
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ElunaSharedData.h"

ElunaSharedData* ElunaSharedData::instance()
{
    static ElunaSharedData instance;
    return &instance;
}

void ElunaSharedData::Set(const std::string& key, const std::string& serializedValue)
{
    std::unique_lock lock(mutex_);
    data_[key] = serializedValue;
}

bool ElunaSharedData::Get(const std::string& key, std::string& outValue) const
{
    std::shared_lock lock(mutex_);
    auto it = data_.find(key);
    if (it == data_.end())
        return false;
    outValue = it->second;
    return true;
}

bool ElunaSharedData::Has(const std::string& key) const
{
    std::shared_lock lock(mutex_);
    return data_.find(key) != data_.end();
}

void ElunaSharedData::Clear(const std::string& key)
{
    std::unique_lock lock(mutex_);
    data_.erase(key);
}

void ElunaSharedData::ClearAll()
{
    std::unique_lock lock(mutex_);
    data_.clear();
}

std::vector<std::string> ElunaSharedData::GetKeys() const
{
    std::shared_lock lock(mutex_);
    std::vector<std::string> keys;
    keys.reserve(data_.size());
    for (const auto& pair : data_)
        keys.push_back(pair.first);
    return keys;
}

size_t ElunaSharedData::Size() const
{
    std::shared_lock lock(mutex_);
    return data_.size();
}
