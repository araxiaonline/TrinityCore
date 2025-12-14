/*
 * Araxia Online - Flight Style Toggle Command
 * 
 * Allows players to toggle between Skyriding and Steady flight modes.
 */

#include "ScriptMgr.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "Player.h"
#include "SpellAuras.h"
#include "WorldSession.h"

using namespace Trinity::ChatCommands;

// Flight style spell IDs
constexpr uint32 SPELL_FLIGHT_STYLE_SKYRIDING = 404464;
constexpr uint32 SPELL_FLIGHT_STYLE_STEADY = 404468;

// Dragonriding spells
constexpr uint32 SPELL_SKYRIDING_BASICS = 376777;
constexpr uint32 SPELL_SURGE_FORWARD = 372608;
constexpr uint32 SPELL_TAKE_TO_THE_SKIES = 377920;

// Helper function to grant dragonriding spells
static void GrantDragonridingSpells(Player* player)
{
    if (!player->HasSpell(SPELL_SKYRIDING_BASICS))
        player->LearnSpell(SPELL_SKYRIDING_BASICS, false);
    if (!player->HasSpell(SPELL_SURGE_FORWARD))
        player->LearnSpell(SPELL_SURGE_FORWARD, false);
    if (!player->HasSpell(SPELL_TAKE_TO_THE_SKIES))
        player->LearnSpell(SPELL_TAKE_TO_THE_SKIES, false);
}

class flight_commandscript : public CommandScript
{
public:
    flight_commandscript() : CommandScript("flight_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable flightCommandTable =
        {
            { "toggle",  HandleFlightToggleCommand,  rbac::RBAC_PERM_COMMAND_HELP,  Console::No },
            { "sky",     HandleFlightSkyCommand,     rbac::RBAC_PERM_COMMAND_HELP,  Console::No },
            { "steady",  HandleFlightSteadyCommand,  rbac::RBAC_PERM_COMMAND_HELP,  Console::No },
        };
        
        static ChatCommandTable commandTable =
        {
            { "flight", flightCommandTable },
        };
        
        return commandTable;
    }

    // Toggle between Skyriding and Steady flight
    static bool HandleFlightToggleCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        if (player->HasAura(SPELL_FLIGHT_STYLE_SKYRIDING))
        {
            // Switch to Steady
            player->RemoveAura(SPELL_FLIGHT_STYLE_SKYRIDING);
            player->AddAura(SPELL_FLIGHT_STYLE_STEADY, player);
            handler->SendSysMessage("Flight style changed to |cff00ff00Steady Flight|r.");
        }
        else
        {
            // Switch to Skyriding - grant dragonriding spells
            player->RemoveAura(SPELL_FLIGHT_STYLE_STEADY);
            player->AddAura(SPELL_FLIGHT_STYLE_SKYRIDING, player);
            GrantDragonridingSpells(player);
            handler->SendSysMessage("Flight style changed to |cff00ffffSkyriding|r (Dragonriding). Dragonriding spells learned!");
        }

        return true;
    }

    // Force Skyriding mode
    static bool HandleFlightSkyCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        player->RemoveAura(SPELL_FLIGHT_STYLE_STEADY);
        player->AddAura(SPELL_FLIGHT_STYLE_SKYRIDING, player);
        GrantDragonridingSpells(player);
        handler->SendSysMessage("Flight style set to |cff00ffffSkyriding|r (Dragonriding). Dragonriding spells learned!");

        return true;
    }

    // Force Steady flight mode
    static bool HandleFlightSteadyCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
            return false;

        player->RemoveAura(SPELL_FLIGHT_STYLE_SKYRIDING);
        player->AddAura(SPELL_FLIGHT_STYLE_STEADY, player);
        handler->SendSysMessage("Flight style set to |cff00ff00Steady Flight|r.");

        return true;
    }
};

void AddSC_flight_commandscript()
{
    new flight_commandscript();
}
