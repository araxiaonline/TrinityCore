--[[
Smallfolk - A Lua serialization library
https://github.com/gvx/Smallfolk
License: ISC
]]

local Smallfolk = {}

local floor = math.floor
local pairs = pairs
local type = type
local concat = table.concat
local tostring = tostring
local tonumber = tonumber
local s_byte = string.byte
local s_char = string.char
local s_sub = string.sub
local s_gsub = string.gsub
local s_match = string.match

local function is_array(t)
    local n = #t
    for k in pairs(t) do
        if type(k) ~= "number" or k < 1 or k > n or floor(k) ~= k then
            return false
        end
    end
    return true
end

local function escape_string(s)
    return s_gsub(s, '[%c"\\]', function(c)
        local n = s_byte(c)
        if c == '"' then return '\\"'
        elseif c == '\\' then return '\\\\'
        elseif c == '\n' then return '\\n'
        elseif c == '\r' then return '\\r'
        elseif c == '\t' then return '\\t'
        else return '\\' .. n
        end
    end)
end

local function dump_value(value, tables)
    local t = type(value)
    if t == "number" then
        if value ~= value then
            return "nan"
        elseif value == 1/0 then
            return "inf"
        elseif value == -1/0 then
            return "-inf"
        else
            return tostring(value)
        end
    elseif t == "string" then
        return '"' .. escape_string(value) .. '"'
    elseif t == "boolean" then
        return value and "true" or "false"
    elseif t == "nil" then
        return "nil"
    elseif t == "table" then
        if tables[value] then
            return "@" .. tables[value]
        end
        local n = (tables.n or 0) + 1
        tables[value] = n
        tables.n = n
        
        local parts = {}
        if is_array(value) then
            for i = 1, #value do
                parts[i] = dump_value(value[i], tables)
            end
            return "[" .. concat(parts, ",") .. "]"
        else
            local i = 1
            for k, v in pairs(value) do
                parts[i] = dump_value(k, tables) .. ":" .. dump_value(v, tables)
                i = i + 1
            end
            return "{" .. concat(parts, ",") .. "}"
        end
    else
        error("cannot serialize type " .. t)
    end
end

function Smallfolk.dumps(value)
    return dump_value(value, {})
end

local function unescape_string(s)
    return s_gsub(s, '\\(.)', function(c)
        if c == 'n' then return '\n'
        elseif c == 'r' then return '\r'
        elseif c == 't' then return '\t'
        elseif c == '"' then return '"'
        elseif c == '\\' then return '\\'
        else
            local n = tonumber(c)
            if n then return s_char(n) end
            return c
        end
    end)
end

local function parse_value(str, pos, tables)
    local c = s_sub(str, pos, pos)
    
    if c == '"' then
        local end_pos = pos + 1
        while true do
            local ch = s_sub(str, end_pos, end_pos)
            if ch == '"' then break
            elseif ch == '\\' then end_pos = end_pos + 1
            elseif ch == '' then error("unterminated string")
            end
            end_pos = end_pos + 1
        end
        return unescape_string(s_sub(str, pos + 1, end_pos - 1)), end_pos + 1
    elseif c == '[' then
        local arr = {}
        local n = #tables + 1
        tables[n] = arr
        pos = pos + 1
        if s_sub(str, pos, pos) == ']' then
            return arr, pos + 1
        end
        local i = 1
        while true do
            local val
            val, pos = parse_value(str, pos, tables)
            arr[i] = val
            i = i + 1
            c = s_sub(str, pos, pos)
            if c == ']' then return arr, pos + 1
            elseif c == ',' then pos = pos + 1
            else error("expected ',' or ']'")
            end
        end
    elseif c == '{' then
        local obj = {}
        local n = #tables + 1
        tables[n] = obj
        pos = pos + 1
        if s_sub(str, pos, pos) == '}' then
            return obj, pos + 1
        end
        while true do
            local key, val
            key, pos = parse_value(str, pos, tables)
            if s_sub(str, pos, pos) ~= ':' then error("expected ':'") end
            val, pos = parse_value(str, pos + 1, tables)
            obj[key] = val
            c = s_sub(str, pos, pos)
            if c == '}' then return obj, pos + 1
            elseif c == ',' then pos = pos + 1
            else error("expected ',' or '}'")
            end
        end
    elseif c == '@' then
        local end_pos = pos + 1
        while s_match(s_sub(str, end_pos, end_pos), '%d') do
            end_pos = end_pos + 1
        end
        local ref = tonumber(s_sub(str, pos + 1, end_pos - 1))
        return tables[ref], end_pos
    elseif s_sub(str, pos, pos + 3) == "true" then
        return true, pos + 4
    elseif s_sub(str, pos, pos + 4) == "false" then
        return false, pos + 5
    elseif s_sub(str, pos, pos + 2) == "nil" then
        return nil, pos + 3
    elseif s_sub(str, pos, pos + 2) == "nan" then
        return 0/0, pos + 3
    elseif s_sub(str, pos, pos + 2) == "inf" then
        return 1/0, pos + 3
    elseif s_sub(str, pos, pos + 3) == "-inf" then
        return -1/0, pos + 4
    else
        local end_pos = pos
        if s_sub(str, end_pos, end_pos) == '-' then
            end_pos = end_pos + 1
        end
        while s_match(s_sub(str, end_pos, end_pos), '[%d%.eE%+%-]') do
            end_pos = end_pos + 1
        end
        local num_str = s_sub(str, pos, end_pos - 1)
        local num = tonumber(num_str)
        if num then
            return num, end_pos
        else
            error("unexpected character at position " .. pos .. ": " .. c)
        end
    end
end

function Smallfolk.loads(str)
    if str == nil or str == "" then
        return nil
    end
    local val, _ = parse_value(str, 1, {})
    return val
end

return Smallfolk
