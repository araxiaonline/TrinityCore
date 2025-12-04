# AIO Integration Documentation

This folder contains comprehensive documentation for integrating AIO (Addon I/O) client↔server communication on TrinityCore 11.2.5.

## Problem Statement

We need bidirectional Lua communication between WoW 11.2.5 client addons and TrinityCore server Lua scripts. The existing AIO implementation was designed for AzerothCore 3.3.5, which has significant protocol and API differences from 11.2.5.

**Current Status:** Message passing is NOT working in either direction.

## Documentation Structure

### 📋 [00_OVERVIEW.md](00_OVERVIEW.md)
**Read this first.** High-level overview of the problem, current status, and project goals.

**Contents:**
- What's working vs. what's broken
- Key differences between 3.3.5 and 11.2.5
- Why previous attempts failed
- Success criteria and principles

### 🔍 [01_DISCOVERY_PLAN.md](01_DISCOVERY_PLAN.md)
Systematic plan for discovering the root cause of communication failure.

**Contents:**
- Phase 1: Message flow analysis (client→server and server→client)
- Phase 2: Root cause identification hypotheses
- Phase 3: Solution design approach
- Phase 4: Implementation and testing plan
- Logging strategy and test commands

### 📝 [02_DISCOVERY_LOG.md](02_DISCOVERY_LOG.md)
**Living document.** Tracks findings as we investigate the issue.

**Contents:**
- Initial code review findings
- Analysis of client-side implementation
- Analysis of server-side implementation
- Analysis of Eluna hooks
- Key questions and unknowns
- Test cases to run

**Update this document as you discover new information.**

### 🏗️ [03_TECHNICAL_ARCHITECTURE.md](03_TECHNICAL_ARCHITECTURE.md)
Deep dive into how AIO works (or should work) technically.

**Contents:**
- System architecture diagrams
- Message flow details (both directions)
- Data structures and message formats
- Handler system design
- Protocol differences between 3.3.5 and 11.2.5
- Security considerations

**Use this to understand how the pieces fit together.**

### 💡 [04_PROPOSED_SOLUTIONS.md](04_PROPOSED_SOLUTIONS.md)
Menu of potential solutions with trade-offs and recommendations.

**Contents:**
- Phase 1: Establish basic communication (test different channels)
- Phase 2: Hook into Eluna (fix LANG_ADDON detection)
- Phase 3: Server→Client messages (fix packet format)
- Phase 4: AIO protocol adaptation (make it work on 11.2.5)
- Phase 5: Production hardening
- Recommended solution path with time estimates
- Contingency plans if approaches fail

**Use this to decide what to try next.**

### 🔨 [05_IMPLEMENTATION_LOG.md](05_IMPLEMENTATION_LOG.md)
**Living document.** Tracks actual implementation work, code changes, and results.

**Contents:**
- Session-by-session work log
- Actions taken, results, findings
- Code changes made
- Status tracking (milestones, issues, technical debt)
- Quick reference (test commands, file locations, debugging tips)

**Update this document as you implement and test solutions.**

## Quick Start Guide

### For LLM Context Loading

If you're an LLM being asked to help with AIO integration:

1. **Start with:** `00_OVERVIEW.md` - Get the big picture
2. **Then read:** `02_DISCOVERY_LOG.md` - See what's been investigated
3. **Check status:** `05_IMPLEMENTATION_LOG.md` - See current progress
4. **Reference:** `03_TECHNICAL_ARCHITECTURE.md` - When you need technical details
5. **Plan next steps:** `04_PROPOSED_SOLUTIONS.md` - Choose an approach

### For Human Developers

1. Read `00_OVERVIEW.md` to understand the problem
2. Review `01_DISCOVERY_PLAN.md` to understand the methodology
3. Follow the plan, updating `02_DISCOVERY_LOG.md` with findings
4. When you know the root cause, choose a solution from `04_PROPOSED_SOLUTIONS.md`
5. Implement and document your work in `05_IMPLEMENTATION_LOG.md`

## Current Status

**Phase:** Discovery (Phase 1 from plan)
**Next Action:** Add logging to trace message flow
**Blocker:** None - ready to start

See `05_IMPLEMENTATION_LOG.md` for detailed status.

## Project Goals

### Primary Goals
1. ✅ Client can send messages to server
2. ✅ Server Lua receives and processes messages
3. ✅ Server can send messages to client
4. ✅ Client receives and processes messages
5. ✅ AIO handler system works bidirectionally
6. ✅ AraxiaTrinityAdmin addon can interact with server

### Non-Goals (For Now)
- Supporting all chat channels (focus on one reliable channel)
- Backwards compatibility with 3.3.5
- Dynamic addon code loading (use static addons)
- Multiple AIO namespaces

## Key Principles

1. **Simple First** - Get one thing working perfectly
2. **Test Each Layer** - Verify at C++, Eluna, and Lua levels
3. **Log Everything** - Can't fix what you can't see
4. **Don't Assume** - 11.2.5 ≠ 3.3.5, verify everything
5. **Be Willing to Rewrite** - Don't force old code to fit new system

## Critical Context

### Why This Is Hard

AIO was built for AzerothCore 3.3.5 (WotLK). We're running TrinityCore 11.2.5 (Midnight). Between these versions:

- **Client API changed** (SendAddonMessage moved to C_ChatInfo namespace)
- **Events changed** (CHAT_MSG_ADDON split into _ADDON and _ADDON_LOGGED)
- **Server packet structure changed** (WorldPacket → WorldPackets::Chat::Chat)
- **Protocol changed** (different opcodes and packet formats)
- **Chat system overhauled** (multiple times across expansions)

Previous attempt failed because it tried to make 3.3.5 code work across all channels without understanding how messages flow in 11.2.5.

### Repository Structure

This is a multi-repository workspace:

- **Client:** `q:\Araxia Online\World of Warcraft Araxia Trinity 11.2.5.83634\_retail_\Interface\AddOns\AIO_Client\`
- **Server Lua:** `q:\github.com\araxiaonline\TrinityServerBits\lua_scripts\AIO_Server\`
- **Server C++:** `q:\github.com\araxiaonline\TrinityCore\src\server\game\`
- **Logs:** `q:\github.com\araxiaonline\TrinityServerBits\logs\`

Changes may need to span all three locations.

## How to Use This Documentation

### When Starting Work
1. Read relevant docs for context
2. Add your goal to `05_IMPLEMENTATION_LOG.md`
3. Work on the problem
4. Document findings in real-time

### When Discovering Something
1. Update `02_DISCOVERY_LOG.md` with findings
2. If it changes the approach, update `04_PROPOSED_SOLUTIONS.md`
3. If it's architectural, update `03_TECHNICAL_ARCHITECTURE.md`

### When Implementing
1. Log your session in `05_IMPLEMENTATION_LOG.md`
2. Include: goal, actions, results, code changes, next steps
3. Update status tracking (milestones, issues, blockers)

### When Stuck
1. Review `04_PROPOSED_SOLUTIONS.md` for alternative approaches
2. Check `02_DISCOVERY_LOG.md` for questions to answer
3. Look at `03_TECHNICAL_ARCHITECTURE.md` for system understanding
4. Ask for help with specific question + context from docs

### When Complete
1. Mark milestones complete in `05_IMPLEMENTATION_LOG.md`
2. Write summary of solution in `00_OVERVIEW.md`
3. Update `04_PROPOSED_SOLUTIONS.md` with what actually worked
4. Create examples and usage docs for future developers

## Success Metrics

We'll know this is done when:

- ✅ Any developer can send client→server message and it works
- ✅ Any developer can send server→client message and it works
- ✅ AraxiaTrinityAdmin addon successfully calls server functions
- ✅ System is documented well enough to maintain and extend
- ✅ Another LLM can load this documentation and continue work

## Contributing to This Documentation

These docs are living documents. Please:

- **Keep them up to date** as you learn new things
- **Be specific** - include code snippets, file paths, line numbers
- **Be honest** - document failures as well as successes
- **Be helpful** - write for someone reading this 6 months from now
- **Be concise** - but don't sacrifice clarity for brevity

## Questions?

If you're an LLM and need clarification:
1. Check if answer is in another doc file
2. Ask specific question with context about what you've already read
3. Reference the document and section you're confused about

If you're a human:
1. Read the docs first (especially Overview and Discovery Log)
2. Check Implementation Log for current status
3. Try the suggested approaches from Proposed Solutions
4. Document your experience in Implementation Log

---

**Last Updated:** Nov 27, 2025
**Current Phase:** Discovery / Planning
**Next Milestone:** Establish basic client→server communication
