/*
 * Araxia MCP Player Manager Tests
 * Unit tests for multi-session AI player manager.
 */

#include "tc_catch2.h"
#include "MCPPlayerManager.h"

using namespace Araxia;

TEST_CASE("MCPPlayerManager Session Creation", "[araxia][mcp][session]")
{
    auto* mgr = MCPPlayerManager::Instance();
    
    SECTION("Create session returns valid ID")
    {
        uint32 sessionId = mgr->CreateSession("TestOwner");
        REQUIRE(sessionId > 0);
        mgr->DestroySession(sessionId);
    }
    
    SECTION("Created session is retrievable")
    {
        uint32 sessionId = mgr->CreateSession("TestOwner");
        MCPPlayerSession* session = mgr->GetSession(sessionId);
        REQUIRE(session != nullptr);
        REQUIRE(session->sessionId == sessionId);
        REQUIRE(session->ownerName == "TestOwner");
        mgr->DestroySession(sessionId);
    }
    
    SECTION("Session has valid token")
    {
        uint32 sessionId = mgr->CreateSession("TestOwner");
        MCPPlayerSession* session = mgr->GetSession(sessionId);
        REQUIRE(!session->sessionToken.empty());
        REQUIRE(session->sessionToken.length() == 32);
        mgr->DestroySession(sessionId);
    }
    
    SECTION("Multiple sessions have unique IDs")
    {
        uint32 id1 = mgr->CreateSession("Owner1");
        uint32 id2 = mgr->CreateSession("Owner2");
        uint32 id3 = mgr->CreateSession("Owner3");
        
        REQUIRE(id1 != id2);
        REQUIRE(id2 != id3);
        REQUIRE(id1 != id3);
        
        mgr->DestroySession(id1);
        mgr->DestroySession(id2);
        mgr->DestroySession(id3);
    }
}

TEST_CASE("MCPPlayerManager Session Destruction", "[araxia][mcp][session]")
{
    auto* mgr = MCPPlayerManager::Instance();
    
    SECTION("Destroy session returns true for valid session")
    {
        uint32 sessionId = mgr->CreateSession("TestOwner");
        REQUIRE(mgr->DestroySession(sessionId) == true);
    }
    
    SECTION("Destroy session returns false for invalid session")
    {
        REQUIRE(mgr->DestroySession(999999) == false);
    }
    
    SECTION("Session not retrievable after destruction")
    {
        uint32 sessionId = mgr->CreateSession("TestOwner");
        mgr->DestroySession(sessionId);
        REQUIRE(mgr->GetSession(sessionId) == nullptr);
    }
}

TEST_CASE("MCPPlayerManager Session Listing", "[araxia][mcp][session]")
{
    auto* mgr = MCPPlayerManager::Instance();
    
    SECTION("GetActiveSessions returns created sessions")
    {
        size_t initialCount = mgr->GetSessionCount();
        
        uint32 id1 = mgr->CreateSession("Owner1");
        uint32 id2 = mgr->CreateSession("Owner2");
        
        auto sessions = mgr->GetActiveSessions();
        REQUIRE(sessions.size() == initialCount + 2);
        
        mgr->DestroySession(id1);
        mgr->DestroySession(id2);
    }
}

TEST_CASE("MCPPlayerManager Session State", "[araxia][mcp][session]")
{
    auto* mgr = MCPPlayerManager::Instance();
    
    SECTION("New session is not online")
    {
        uint32 sessionId = mgr->CreateSession("TestOwner");
        REQUIRE(mgr->IsOnline(sessionId) == false);
        REQUIRE(mgr->GetPlayer(sessionId) == nullptr);
        mgr->DestroySession(sessionId);
    }
    
    SECTION("Session timestamps are set")
    {
        uint32 sessionId = mgr->CreateSession("TestOwner");
        MCPPlayerSession* session = mgr->GetSession(sessionId);
        REQUIRE(session->createdAt > 0);
        REQUIRE(session->lastActivity > 0);
        mgr->DestroySession(sessionId);
    }
}
