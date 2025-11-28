# START HERE - AIO Integration Project

## What I Just Created For You

I've analyzed your AIO integration problem and created comprehensive documentation to help you (and future LLMs) fix it systematically without getting lost.

### Documentation Created

**Six documents** in `araxiaonline/araxia_docs/aio_integration/`:

1. **README.md** - Navigation guide for all documentation
2. **00_OVERVIEW.md** - High-level problem statement and context
3. **01_DISCOVERY_PLAN.md** - Systematic investigation methodology
4. **02_DISCOVERY_LOG.md** - Living document to track findings
5. **03_TECHNICAL_ARCHITECTURE.md** - Deep technical analysis
6. **04_PROPOSED_SOLUTIONS.md** - Menu of solutions with trade-offs
7. **05_IMPLEMENTATION_LOG.md** - Living document to track work
8. **QUICK_REFERENCE.md** - One-page cheat sheet

## What I Found

### The Core Problem

**AIO was designed for AzerothCore 3.3.5 (WotLK) ~15 years ago.**
**You're running TrinityCore 11.2.5 (Midnight) - the gap is massive.**

Between these versions, WoW's addon message system underwent major changes:
- Client API moved to `C_ChatInfo` namespace
- Events split into `CHAT_MSG_ADDON` and `CHAT_MSG_ADDON_LOGGED`
- Server packet structure completely redesigned
- Chat system overhauled multiple times
- Protocol opcodes changed

**Current Status:**
- ✅ Server compiles and runs with Eluna
- ✅ AIO_Server loads successfully
- ✅ AIO_Client loads without errors
- ❌ Messages don't flow in EITHER direction

### Why Previous Attempt Failed

You got lost trying to make AIO work across multiple chat channels (whisper, guild, party, etc.) without first understanding:
1. **Do addon messages even reach the server?**
2. **How does Eluna receive them?**
3. **What changed in 11.2.5?**

This is like trying to fix a car's AC when you haven't verified the engine starts.

### Critical Findings from Code Analysis

**Client→Server Flow (BROKEN):**
```
Client sends via WHISPER to self
  ↓
??? (Needs investigation)
  ↓
Should reach Eluna OnAddonMessage hook (Event 30)
```

**Server→Client Flow (BROKEN):**
```
Server calls player:SendAddonMessage(prefix, msg, 7, player)
  ↓
Eluna builds packet with LANG_ADDON
  ↓
??? (Needs investigation)
  ↓
Should fire CHAT_MSG_ADDON_LOGGED event on client
```

**Key Unknowns:**
1. Do messages reach `ChatHandler::HandleChatAddonMessage()`?
2. Is `LANG_ADDON` set correctly for 11.2.5 protocol?
3. Does `OnChat()` get called with `LANG_ADDON`?
4. Does whisper-to-self even work in 11.2.5?
5. What is channel type `7` in 11.2.5 enum?
6. Is the packet format correct for 11.2.5 client?

## My Recommended Path Forward

### Phase 1: Add Logging (1-2 hours)
**Don't write any fixes yet.** First, find where messages break.

Add logging to trace message flow:
1. **C++ in ChatHandler.cpp** - Log when addon messages arrive
2. **C++ in PlayerHooks.cpp** - Log when LANG_ADDON detected
3. **C++ in ServerHooks.cpp** - Log OnAddonMessage calls
4. **Lua on server** - Log ADDON_EVENT_ON_MESSAGE handler
5. **Lua on client** - Log CHAT_MSG_ADDON_LOGGED event

### Phase 2: Test Each Layer (1-2 hours)
Run simple tests to identify exactly where flow breaks:

**Test 1: Do messages reach server?**
```lua
/run C_ChatInfo.SendAddonMessage("TEST", "Hello", "WHISPER", UnitName("player"))
```
Check: Does ChatHandler log it?

**Test 2: Try different channels**
```lua
/run C_ChatInfo.SendAddonMessage("TEST", "Hello", "GUILD")
```
Check: Does GUILD work better than WHISPER?

**Test 3: Server→Client**
```lua
-- Server Lua
player:SendAddonMessage("TEST", "Hello", 7, player)
```
Check: Does client receive it?

### Phase 3: Fix Root Cause (2-4 hours)
Based on findings, implement targeted fix:

**If messages don't reach ChatHandler:**
→ Client API problem, need to fix client-side sending

**If LANG_ADDON not detected:**
→ Need to hook ChatHandler explicitly or fix language detection

**If OnAddonMessage doesn't fire:**
→ Need to add missing hook in C++

**If packet format wrong:**
→ Need to fix SendAddonMessage packet construction

### Phase 4: Integrate AIO (4-8 hours)
Once basic messages work:
1. Update AIO to use working channel
2. Simplify to single-channel (no multi-channel support)
3. Test handler system
4. Integrate with AraxiaTrinityAdmin

**Total Estimated Time: 8-16 hours**

## What Makes This Different

### This Time We Will:
1. ✅ **Test at each layer** before moving to next
2. ✅ **Log everything** to see exactly where it breaks
3. ✅ **Focus on one channel** until it works perfectly
4. ✅ **Not assume 3.3.5 code works** on 11.2.5
5. ✅ **Document as we go** so we don't lose progress

### We Will NOT:
1. ❌ Try to make all channels work at once
2. ❌ Fix AIO before basic messages work
3. ❌ Guess - we'll verify every assumption
4. ❌ Get lost in rabbit holes
5. ❌ Give up if first approach doesn't work

## How to Use This Documentation

### If You're Starting Now:
1. **Read:** `00_OVERVIEW.md` (10 min)
2. **Skim:** `03_TECHNICAL_ARCHITECTURE.md` (15 min)
3. **Follow:** `01_DISCOVERY_PLAN.md` Phase 1
4. **Document:** Findings in `02_DISCOVERY_LOG.md`
5. **Keep:** `QUICK_REFERENCE.md` open while working

### If You're Continuing Work:
1. **Check:** `05_IMPLEMENTATION_LOG.md` for current status
2. **Review:** `02_DISCOVERY_LOG.md` for what's known
3. **Pick:** Next step from `04_PROPOSED_SOLUTIONS.md`
4. **Document:** Your session in `05_IMPLEMENTATION_LOG.md`

### If You're Stuck:
1. **Review:** `04_PROPOSED_SOLUTIONS.md` for alternatives
2. **Check:** `02_DISCOVERY_LOG.md` for unanswered questions
3. **Read:** `03_TECHNICAL_ARCHITECTURE.md` for system understanding
4. **Ask:** Specific question with context from docs

## Quick Test to Run Right Now

Want to see the problem immediately?

**On Client (in-game):**
```lua
/run C_ChatInfo.SendAddonMessage("TEST", "Hello from client", "WHISPER", UnitName("player"))
```

**On Server (check logs):**
```bash
tail -f /opt/trinitycore/logs/Server.log | grep -i "addon\|TEST"
```

**Expected:** Nothing appears in logs ← This is the problem

## Key Success Metrics

You'll know you're on track when:

- **< 1 hour:** See addon message in server logs
- **< 4 hours:** Server Lua receives message via hook
- **< 8 hours:** Server can send, client receives
- **< 16 hours:** Full AIO handler system works
- **< 24 hours:** AraxiaTrinityAdmin working

If any step takes much longer → Stop, reassess, try different approach from solutions doc.

## The Big Picture

This is NOT about making 3.3.5 code work on 11.2.5.

This is about:
1. **Understanding** how 11.2.5 addon messages work
2. **Adapting** AIO to work within those constraints
3. **Building** a reliable foundation for content creation tools

## Final Notes

### For Future LLMs
These documents contain everything you need to understand:
- What the problem is
- Why it's hard
- What's been tried
- What to try next
- How the system works

Start with README.md, read what's relevant, update as you learn.

### For Human Developers
Don't try to hold all this in your head. Use the docs. Update them. They're here to help you not get lost like last time.

### For Both
**Remember:** The goal isn't to fix AIO as-is. The goal is to get client↔server Lua communication working reliably. If that means rewriting parts of AIO for 11.2.5, that's fine. Simple and working beats complex and broken.

---

## Ready to Start?

1. Read `00_OVERVIEW.md` for full context
2. Open `01_DISCOVERY_PLAN.md` and begin Phase 1
3. Keep `QUICK_REFERENCE.md` handy
4. Update `05_IMPLEMENTATION_LOG.md` as you work

**Good luck!** 🚀

This is a solvable problem. We just need to approach it methodically.
