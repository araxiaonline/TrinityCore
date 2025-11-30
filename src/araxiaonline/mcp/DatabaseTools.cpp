/*
 * Araxia MCP Server - Database Tools
 * 
 * Direct database access for debugging and administration.
 */

#include "AraxiaMCPServer.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include <sstream>

namespace Araxia
{

// Helper to convert QueryResult to JSON
json QueryResultToJson(QueryResult result)
{
    if (!result)
        return {{"rows", json::array()}, {"rowCount", 0}, {"fieldCount", 0}};
    
    json rows = json::array();
    uint32 fieldCount = result->GetFieldCount();
    
    // Get field names
    std::vector<std::string> fieldNames;
    for (uint32 i = 0; i < fieldCount; ++i)
    {
        QueryResultFieldMetadata const& meta = result->GetFieldMetadata(i);
        fieldNames.push_back(meta.Alias);
    }
    
    do
    {
        Field* fields = result->Fetch();
        json row = json::object();
        
        for (uint32 i = 0; i < fieldCount; ++i)
        {
            if (fields[i].IsNull())
            {
                row[fieldNames[i]] = nullptr;
            }
            else
            {
                // Try to preserve types
                switch (fields[i].GetType())
                {
                    case DatabaseFieldTypes::Int8:
                    case DatabaseFieldTypes::Int16:
                    case DatabaseFieldTypes::Int32:
                        row[fieldNames[i]] = fields[i].GetInt32();
                        break;
                    case DatabaseFieldTypes::Int64:
                        row[fieldNames[i]] = fields[i].GetInt64();
                        break;
                    case DatabaseFieldTypes::UInt8:
                    case DatabaseFieldTypes::UInt16:
                    case DatabaseFieldTypes::UInt32:
                        row[fieldNames[i]] = fields[i].GetUInt32();
                        break;
                    case DatabaseFieldTypes::UInt64:
                        row[fieldNames[i]] = fields[i].GetUInt64();
                        break;
                    case DatabaseFieldTypes::Float:
                        row[fieldNames[i]] = fields[i].GetFloat();
                        break;
                    case DatabaseFieldTypes::Double:
                        row[fieldNames[i]] = fields[i].GetDouble();
                        break;
                    default:
                        row[fieldNames[i]] = fields[i].GetString();
                        break;
                }
            }
        }
        rows.push_back(row);
    }
    while (result->NextRow());
    
    return {
        {"rows", rows},
        {"rowCount", rows.size()},
        {"fieldCount", fieldCount},
        {"fields", fieldNames}
    };
}

void RegisterDatabaseTools()
{
    // db_query - Execute SELECT query on any database
    sMCPServer->RegisterTool(
        "db_query",
        "Execute a SELECT query on the specified database (world, characters, auth). Returns rows as JSON.",
        {
            {"type", "object"},
            {"properties", {
                {"database", {
                    {"type", "string"},
                    {"description", "Database to query: world, characters, or auth"},
                    {"enum", {"world", "characters", "auth"}}
                }},
                {"query", {
                    {"type", "string"},
                    {"description", "SQL SELECT query to execute"}
                }}
            }},
            {"required", {"database", "query"}}
        },
        [](const json& params) -> json {
            std::string database = params.value("database", "world");
            std::string query = params.value("query", "");
            
            // Security: Only allow SELECT
            std::string upperQuery = query;
            std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
            
            // Trim whitespace
            size_t start = upperQuery.find_first_not_of(" \t\n\r");
            if (start != std::string::npos)
                upperQuery = upperQuery.substr(start);
            
            if (upperQuery.substr(0, 6) != "SELECT" && 
                upperQuery.substr(0, 4) != "SHOW" && 
                upperQuery.substr(0, 8) != "DESCRIBE" &&
                upperQuery.substr(0, 7) != "EXPLAIN")
            {
                return {
                    {"success", false},
                    {"error", "Only SELECT, SHOW, DESCRIBE, and EXPLAIN queries are allowed. Use db_execute for modifications."}
                };
            }
            
            TC_LOG_DEBUG("araxia.mcp", "[MCP] db_query on %s: %s", database.c_str(), query.c_str());
            
            QueryResult result;
            if (database == "world")
                result = WorldDatabase.Query(query.c_str());
            else if (database == "characters")
                result = CharacterDatabase.Query(query.c_str());
            else if (database == "auth")
                result = LoginDatabase.Query(query.c_str());
            else
                return {{"success", false}, {"error", "Unknown database: " + database}};
            
            json data = QueryResultToJson(result);
            data["success"] = true;
            data["database"] = database;
            data["query"] = query;
            
            return data;
        }
    );
    
    // db_execute - Execute INSERT/UPDATE/DELETE (with logging)
    sMCPServer->RegisterTool(
        "db_execute",
        "Execute an INSERT, UPDATE, or DELETE query. Changes are logged for audit.",
        {
            {"type", "object"},
            {"properties", {
                {"database", {
                    {"type", "string"},
                    {"description", "Database to modify: world, characters, or auth"},
                    {"enum", {"world", "characters", "auth"}}
                }},
                {"query", {
                    {"type", "string"},
                    {"description", "SQL query to execute (INSERT, UPDATE, DELETE)"}
                }},
                {"reason", {
                    {"type", "string"},
                    {"description", "Reason for the change (for audit log)"}
                }}
            }},
            {"required", {"database", "query", "reason"}}
        },
        [](const json& params) -> json {
            std::string database = params.value("database", "world");
            std::string query = params.value("query", "");
            std::string reason = params.value("reason", "No reason provided");
            
            // Security: Block dangerous operations
            std::string upperQuery = query;
            std::transform(upperQuery.begin(), upperQuery.end(), upperQuery.begin(), ::toupper);
            
            if (upperQuery.find("DROP ") != std::string::npos ||
                upperQuery.find("TRUNCATE ") != std::string::npos ||
                upperQuery.find("ALTER ") != std::string::npos ||
                upperQuery.find("CREATE ") != std::string::npos)
            {
                return {
                    {"success", false},
                    {"error", "DDL operations (DROP, TRUNCATE, ALTER, CREATE) are not allowed via MCP."}
                };
            }
            
            TC_LOG_INFO("araxia.mcp", "[MCP] db_execute on %s (reason: %s): %s", 
                        database.c_str(), reason.c_str(), query.c_str());
            
            try
            {
                if (database == "world")
                    WorldDatabase.DirectExecute(query.c_str());
                else if (database == "characters")
                    CharacterDatabase.DirectExecute(query.c_str());
                else if (database == "auth")
                    LoginDatabase.DirectExecute(query.c_str());
                else
                    return {{"success", false}, {"error", "Unknown database: " + database}};
                
                return {
                    {"success", true},
                    {"database", database},
                    {"query", query},
                    {"reason", reason},
                    {"message", "Query executed successfully"}
                };
            }
            catch (const std::exception& e)
            {
                return {
                    {"success", false},
                    {"error", e.what()},
                    {"database", database},
                    {"query", query}
                };
            }
        }
    );
    
    // db_tables - List tables in a database
    sMCPServer->RegisterTool(
        "db_tables",
        "List all tables in the specified database.",
        {
            {"type", "object"},
            {"properties", {
                {"database", {
                    {"type", "string"},
                    {"description", "Database to list: world, characters, or auth"},
                    {"enum", {"world", "characters", "auth"}}
                }}
            }},
            {"required", {"database"}}
        },
        [](const json& params) -> json {
            std::string database = params.value("database", "world");
            
            QueryResult result;
            if (database == "world")
                result = WorldDatabase.Query("SHOW TABLES");
            else if (database == "characters")
                result = CharacterDatabase.Query("SHOW TABLES");
            else if (database == "auth")
                result = LoginDatabase.Query("SHOW TABLES");
            else
                return {{"success", false}, {"error", "Unknown database: " + database}};
            
            json tables = json::array();
            if (result)
            {
                do
                {
                    Field* fields = result->Fetch();
                    tables.push_back(fields[0].GetString());
                }
                while (result->NextRow());
            }
            
            return {
                {"success", true},
                {"database", database},
                {"tables", tables},
                {"count", tables.size()}
            };
        }
    );
    
    // db_describe - Get table schema
    sMCPServer->RegisterTool(
        "db_describe",
        "Get the schema/structure of a specific table.",
        {
            {"type", "object"},
            {"properties", {
                {"database", {
                    {"type", "string"},
                    {"description", "Database containing the table"},
                    {"enum", {"world", "characters", "auth"}}
                }},
                {"table", {
                    {"type", "string"},
                    {"description", "Name of the table to describe"}
                }}
            }},
            {"required", {"database", "table"}}
        },
        [](const json& params) -> json {
            std::string database = params.value("database", "world");
            std::string table = params.value("table", "");
            
            // Sanitize table name (alphanumeric and underscore only)
            for (char c : table)
            {
                if (!std::isalnum(c) && c != '_')
                    return {{"success", false}, {"error", "Invalid table name"}};
            }
            
            std::string query = "DESCRIBE " + table;
            
            QueryResult result;
            if (database == "world")
                result = WorldDatabase.Query(query.c_str());
            else if (database == "characters")
                result = CharacterDatabase.Query(query.c_str());
            else if (database == "auth")
                result = LoginDatabase.Query(query.c_str());
            else
                return {{"success", false}, {"error", "Unknown database: " + database}};
            
            json data = QueryResultToJson(result);
            data["success"] = true;
            data["database"] = database;
            data["table"] = table;
            
            return data;
        }
    );
    
    TC_LOG_INFO("araxia.mcp", "[MCP] Database tools registered (db_query, db_execute, db_tables, db_describe)");
}

} // namespace Araxia
