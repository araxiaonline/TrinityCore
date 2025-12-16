#!/usr/bin/env python3
"""
MCP Player Integration Tests

Tests the MCP player lifecycle without requiring manual interaction.
Designed to help debug login/logout/shutdown issues.

Usage:
    python3 test_mcp_integration.py [--host HOST] [--port PORT] [--character NAME]
"""

import argparse
import json
import requests
import time
import sys

class MCPClient:
    def __init__(self, host="localhost", port=8765):
        self.url = f"http://{host}:{port}/mcp"
        self.session_id = None
    
    def call(self, tool_name: str, args: dict) -> dict:
        """Make an MCP tool call and return the parsed result."""
        payload = {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "tools/call",
            "params": {
                "name": tool_name,
                "arguments": args
            }
        }
        
        try:
            response = requests.post(self.url, json=payload, timeout=5)
            data = response.json()
            
            if "result" in data and "content" in data["result"]:
                text = data["result"]["content"][0]["text"]
                return json.loads(text)
            elif "error" in data:
                return {"success": False, "error": data["error"]}
            else:
                return {"success": False, "error": "Unknown response format", "raw": data}
        except requests.exceptions.Timeout:
            return {"success": False, "error": "Request timed out (5s)"}
        except requests.exceptions.ConnectionError as e:
            return {"success": False, "error": f"Connection failed: {e}"}
        except Exception as e:
            return {"success": False, "error": str(e)}
    
    def create_session(self, owner_name: str = "IntegrationTest") -> dict:
        """Create a new MCP session."""
        result = self.call("mcp_session_create", {"owner_name": owner_name})
        if result.get("success"):
            self.session_id = result.get("session_id")
        return result
    
    def destroy_session(self) -> dict:
        """Destroy the current session."""
        if not self.session_id:
            return {"success": False, "error": "No session"}
        result = self.call("mcp_session_destroy", {"session_id": self.session_id})
        if result.get("success"):
            self.session_id = None
        return result
    
    def login(self, character_name: str) -> dict:
        """Login a character."""
        if not self.session_id:
            return {"success": False, "error": "No session"}
        return self.call("mcp_player_login", {
            "session_id": self.session_id,
            "character_name": character_name
        })
    
    def logout(self) -> dict:
        """Logout the current character."""
        if not self.session_id:
            return {"success": False, "error": "No session"}
        return self.call("mcp_player_logout", {"session_id": self.session_id})
    
    def status(self) -> dict:
        """Get player status."""
        if not self.session_id:
            return {"success": False, "error": "No session"}
        return self.call("mcp_player_status", {"session_id": self.session_id})
    
    def list_sessions(self) -> dict:
        """List all sessions."""
        return self.call("mcp_session_list", {})


def print_result(name: str, result: dict, verbose: bool = True):
    """Print a test result."""
    success = result.get("success", False)
    icon = "✓" if success else "✗"
    color = "\033[92m" if success else "\033[91m"
    reset = "\033[0m"
    
    print(f"  {color}{icon}{reset} {name}")
    if verbose and not success:
        print(f"      Error: {result.get('error', 'Unknown')}")
    return success


def test_session_lifecycle(client: MCPClient, verbose: bool = True):
    """Test basic session create/destroy."""
    print("\n[Test] Session Lifecycle")
    
    # Create session
    result = client.create_session("LifecycleTest")
    if not print_result("Create session", result, verbose):
        return False
    
    session_id = result.get("session_id")
    print(f"      Session ID: {session_id}")
    
    # List sessions
    result = client.list_sessions()
    print_result("List sessions", result, verbose)
    
    # Destroy session
    result = client.destroy_session()
    if not print_result("Destroy session", result, verbose):
        return False
    
    return True


def test_login_status(client: MCPClient, character_name: str, verbose: bool = True):
    """Test login and status check."""
    print("\n[Test] Login and Status")
    
    # Create session
    result = client.create_session("LoginTest")
    if not print_result("Create session", result, verbose):
        return False
    
    # Login
    result = client.login(character_name)
    if not print_result(f"Login '{character_name}'", result, verbose):
        client.destroy_session()
        return False
    
    # Wait for async login
    print("      Waiting for login to complete...")
    time.sleep(2)
    
    # Check status
    result = client.status()
    if not print_result("Get status", result, verbose):
        client.destroy_session()
        return False
    
    in_world = result.get("in_world", False)
    print(f"      In world: {in_world}")
    
    if in_world:
        char = result.get("character", {})
        pos = result.get("position", {})
        print(f"      Character: {char.get('name')} (Level {char.get('level')})")
        print(f"      Position: ({pos.get('x', 0):.1f}, {pos.get('y', 0):.1f}, {pos.get('z', 0):.1f}) Map {pos.get('map')}")
    
    # Cleanup - destroy session (which should handle logout)
    result = client.destroy_session()
    print_result("Destroy session", result, verbose)
    
    return in_world


def test_logout(client: MCPClient, character_name: str, verbose: bool = True):
    """Test explicit logout (the problematic operation)."""
    print("\n[Test] Explicit Logout")
    
    # Create session
    result = client.create_session("LogoutTest")
    if not print_result("Create session", result, verbose):
        return False
    
    # Login
    result = client.login(character_name)
    if not print_result(f"Login '{character_name}'", result, verbose):
        client.destroy_session()
        return False
    
    # Wait for async login
    print("      Waiting for login to complete...")
    time.sleep(2)
    
    # Verify in world
    result = client.status()
    if not result.get("in_world"):
        print_result("Verify in world", {"success": False, "error": "Not in world"}, verbose)
        client.destroy_session()
        return False
    print_result("Verify in world", {"success": True}, verbose)
    
    # Logout (this is the problematic call)
    print("      Calling logout (may hang if buggy)...")
    result = client.logout()
    if not print_result("Logout", result, verbose):
        # Even if logout fails, try to cleanup
        client.destroy_session()
        return False
    
    # Wait for async logout
    print("      Waiting for logout to complete...")
    time.sleep(2)
    
    # Verify logged out
    result = client.status()
    in_world = result.get("in_world", True)
    print_result("Verify logged out", {"success": not in_world}, verbose)
    
    # Cleanup
    result = client.destroy_session()
    print_result("Destroy session", result, verbose)
    
    return not in_world


def main():
    parser = argparse.ArgumentParser(description="MCP Player Integration Tests")
    parser.add_argument("--host", default="localhost", help="MCP server host")
    parser.add_argument("--port", type=int, default=8765, help="MCP server port")
    parser.add_argument("--character", default="Scarletseer", help="Character name to test with")
    parser.add_argument("--test", choices=["all", "session", "login", "logout"], default="all",
                        help="Which test to run")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    args = parser.parse_args()
    
    client = MCPClient(args.host, args.port)
    
    print(f"MCP Integration Tests")
    print(f"Server: {client.url}")
    print(f"Character: {args.character}")
    print("=" * 50)
    
    results = {}
    
    if args.test in ["all", "session"]:
        results["session"] = test_session_lifecycle(client, args.verbose)
    
    if args.test in ["all", "login"]:
        results["login"] = test_login_status(client, args.character, args.verbose)
    
    if args.test in ["all", "logout"]:
        results["logout"] = test_logout(client, args.character, args.verbose)
    
    # Summary
    print("\n" + "=" * 50)
    print("Summary:")
    passed = sum(1 for v in results.values() if v)
    total = len(results)
    
    for name, success in results.items():
        icon = "✓" if success else "✗"
        color = "\033[92m" if success else "\033[91m"
        reset = "\033[0m"
        print(f"  {color}{icon}{reset} {name}")
    
    print(f"\nPassed: {passed}/{total}")
    
    return 0 if passed == total else 1


if __name__ == "__main__":
    sys.exit(main())
