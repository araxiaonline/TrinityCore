#!/bin/bash
# MCP Player Login Test Script
# Tests the full MCP player login flow and shutdown behavior

MCP_HOST="${MCP_HOST:-localhost}"
MCP_PORT="${MCP_PORT:-8765}"
MCP_URL="http://${MCP_HOST}:${MCP_PORT}/mcp"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
GRAY='\033[0;90m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Helper function to make MCP calls
mcp_call() {
    local method="$1"
    local tool_name="$2"
    local args="$3"
    
    local payload="{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"${method}\",\"params\":{\"name\":\"${tool_name}\",\"arguments\":${args}}}"
    
    curl -s -X POST "$MCP_URL" \
        -H "Content-Type: application/json" \
        -d "$payload"
}

# Pretty print MCP response - extracts and formats the inner JSON
format_response() {
    python3 -c "
import sys, json
try:
    data = json.load(sys.stdin)
    if 'result' in data and 'content' in data['result']:
        text = data['result']['content'][0]['text']
        inner = json.loads(text)
        print(json.dumps(inner, indent=2))
    else:
        print(json.dumps(data, indent=2))
except Exception as e:
    print(f'Parse error: {e}')
" 2>/dev/null
}

# Extract a value from the inner JSON response
extract_value() {
    local key="$1"
    python3 -c "
import sys, json
try:
    data = json.load(sys.stdin)
    text = data['result']['content'][0]['text']
    inner = json.loads(text)
    print(inner.get('$key', ''))
except:
    print('')
" 2>/dev/null
}

echo -e "${BOLD}${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${BLUE}║     MCP Player Login Test Script       ║${NC}"
echo -e "${BOLD}${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""
echo -e "  ${GRAY}Server:${NC} ${CYAN}${MCP_URL}${NC}"
echo ""

# Step 1: Create session
echo -e "${BOLD}${BLUE}▶ Step 1:${NC} Creating MCP session..."
RESPONSE=$(mcp_call "tools/call" "mcp_session_create" '{"owner_name":"TestScript"}')
SESSION_ID=$(echo "$RESPONSE" | extract_value "session_id")

if [ -z "$SESSION_ID" ]; then
    echo -e "  ${RED}✗ Failed to create session. Is the server running?${NC}"
    exit 1
fi

echo -e "  ${GREEN}✓${NC} Session created: ${BOLD}${SESSION_ID}${NC}"
echo ""

# Step 2: Login player
CHARACTER_NAME="${1:-Scarletseer}"
echo -e "${BOLD}${BLUE}▶ Step 2:${NC} Logging in character: ${YELLOW}${CHARACTER_NAME}${NC}..."
RESPONSE=$(mcp_call "tools/call" "mcp_player_login" "{\"session_id\":${SESSION_ID},\"character_name\":\"${CHARACTER_NAME}\"}")
SUCCESS=$(echo "$RESPONSE" | extract_value "success")
if [ "$SUCCESS" = "True" ] || [ "$SUCCESS" = "true" ]; then
    echo -e "  ${GREEN}✓${NC} Login initiated"
else
    echo -e "  ${RED}✗${NC} Login failed"
    echo "$RESPONSE" | format_response
fi
echo ""

# Step 3: Wait a moment for async login
echo -e "${BOLD}${BLUE}▶ Step 3:${NC} Waiting for login to complete..."
sleep 1
echo -e "  ${GREEN}✓${NC} Done"
echo ""

# Step 4: Check player status
echo -e "${BOLD}${BLUE}▶ Step 4:${NC} Checking player status..."
RESPONSE=$(mcp_call "tools/call" "mcp_player_status" "{\"session_id\":${SESSION_ID}}")

# Parse and display player info nicely
python3 -c "
import sys, json
try:
    data = json.load(sys.stdin)
    text = data['result']['content'][0]['text']
    info = json.loads(text)
    
    if info.get('in_world'):
        print('  \033[0;32m✓\033[0m Player is in world!')
        print()
        char = info.get('character', {})
        pos = info.get('position', {})
        
        print('  \033[1m┌─ Character ─────────────────────────┐\033[0m')
        print(f'  │  Name:   \033[1;33m{char.get(\"name\", \"?\")}\033[0m')
        print(f'  │  Level:  {char.get(\"level\", \"?\")}')
        print(f'  │  Race:   {char.get(\"race\", \"?\")}  Class: {char.get(\"class\", \"?\")}')
        print(f'  │  Health: {char.get(\"health\", \"?\")}/{char.get(\"max_health\", \"?\")}')
        print(f'  │  Alive:  {\"Yes\" if char.get(\"alive\") else \"No\"}')
        print('  \033[1m└──────────────────────────────────────┘\033[0m')
        print()
        print('  \033[1m┌─ Position ──────────────────────────┐\033[0m')
        print(f'  │  Map:    {pos.get(\"map\", \"?\")}')
        print(f'  │  Zone:   {pos.get(\"zone\", \"?\")}  Area: {pos.get(\"area\", \"?\")}')
        print(f'  │  X:      {pos.get(\"x\", 0):.2f}')
        print(f'  │  Y:      {pos.get(\"y\", 0):.2f}')
        print(f'  │  Z:      {pos.get(\"z\", 0):.2f}')
        print('  \033[1m└──────────────────────────────────────┘\033[0m')
    else:
        print('  \033[0;31m✗\033[0m Player not in world yet')
except Exception as e:
    print(f'  Error parsing response: {e}')
" <<< "$RESPONSE"
echo ""

# Step 5: List all sessions
echo -e "${BOLD}${BLUE}▶ Step 5:${NC} Listing active sessions..."
RESPONSE=$(mcp_call "tools/call" "mcp_session_list" "{}")

python3 -c "
import sys, json
try:
    data = json.load(sys.stdin)
    text = data['result']['content'][0]['text']
    info = json.loads(text)
    
    sessions = info.get('sessions', [])
    print(f'  Found {len(sessions)} session(s):')
    for s in sessions:
        status = '\033[0;32m●\033[0m' if s.get('online') else '\033[0;90m○\033[0m'
        char = s.get('character') or '(no character)'
        print(f'    {status} Session {s.get(\"session_id\")}: {char} ({s.get(\"owner_name\")})')
except Exception as e:
    print(f'  Error: {e}')
" <<< "$RESPONSE"
echo ""

# Interactive mode
echo -e "${BOLD}${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${BLUE}║          Player Ready!                 ║${NC}"
echo -e "${BOLD}${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""
echo -e "  ${BOLD}Session ID:${NC} ${YELLOW}${SESSION_ID}${NC}"
echo -e "  ${BOLD}Character:${NC}  ${YELLOW}${CHARACTER_NAME}${NC}"
echo ""
echo -e "  ${BOLD}${CYAN}To test shutdown crash fix:${NC}"
echo -e "    1. Keep this player logged in"
echo -e "    2. In the server console, run: ${YELLOW}server shutdown 1${NC}"
echo -e "    3. Server should shut down ${GREEN}cleanly${NC} without crash"
echo ""
echo -e "${BLUE}──────────────────────────────────────────${NC}"

# Optional: Wait for user input to cleanup
read -p "Press Enter to logout and destroy session (or Ctrl+C to keep active)..."

# Cleanup
echo ""
echo -e "${BOLD}${BLUE}▶ Cleanup${NC}"
echo -n "  Logging out... "
mcp_call "tools/call" "mcp_player_logout" "{\"session_id\":${SESSION_ID}}" > /dev/null 2>&1
echo -e "${GREEN}✓${NC}"

echo -n "  Destroying session... "
mcp_call "tools/call" "mcp_session_destroy" "{\"session_id\":${SESSION_ID}}" > /dev/null 2>&1
echo -e "${GREEN}✓${NC}"

echo ""
echo -e "${GREEN}${BOLD}✓ Test complete!${NC}"
