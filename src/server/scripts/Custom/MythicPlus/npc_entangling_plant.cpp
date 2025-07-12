// npc_entangling_plant.cpp

#include "ScriptMgr.h"
#include "Creature.h"
#include "Player.h"
#include "SpellAuras.h"
#include "Log.h"
#include <unordered_map>

class npc_entangling_plant : public CreatureScript
{
public:
    npc_entangling_plant() : CreatureScript("npc_entangling_plant") {}

    struct npc_entangling_plantAI : public ScriptedAI
    {
        npc_entangling_plantAI(Creature* creature) : ScriptedAI(creature) {}

        std::unordered_map<ObjectGuid, uint32> entangledPlayers;    // GUID → remaining time (ms)
        std::unordered_map<ObjectGuid, uint32> ropeSpells;          // GUID → rope spell ID

        uint32 checkTimer = 250;

        void Reset() override
        {
            me->CastSpell(me, 3001013, true); // Plant FX aura
            LOG_DEBUG("module.affix.entangling", "Plant {} spawned at X: {:.2f}, Y: {:.2f}, Z: {:.2f}",
                me->GetGUID().ToString().c_str(), me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
        }

        void UpdateAI(uint32 diff) override
        {
            if (checkTimer <= diff)
            {
                LOG_DEBUG("module.affix.entangling", "Plant {} running entangling check", me->GetGUID().ToString().c_str());
                ProcessEntangling(diff);
                checkTimer = 250;
            }
            else
            {
                checkTimer -= diff;
            }
        }

        void ProcessEntangling(uint32 diff)
        {
            if (!me->GetMap())
            {
                LOG_DEBUG("module.affix.entangling", "Plant {} has no valid map", me->GetGUID().ToString().c_str());
                return;
            }

            for (auto const& ref : me->GetMap()->GetPlayers())
            {
                Player* player = ref.GetSource();
                if (!player || !player->IsAlive())
                    continue;

                ObjectGuid guid = player->GetGUID();
                float distance = me->GetDistance(player);

                LOG_DEBUG("module.affix.entangling", "Checking player '{}' at {:.2f} yd", player->GetName(), distance);

                if (distance <= 10.0f)
                {
                    if (!player->HasAura(3001014))
                    {
                        uint32 ropeSpell = GetOrAssignRopeSpell(guid);
                        LOG_DEBUG("module.affix.entangling", "Applying rope spell {} and slow to '{}'", ropeSpell, player->GetName());

                        me->CastSpell(player, ropeSpell, true);       // Rope aura (on plant)
                        me->AddAura(3001014, player);                 // Slow aura

                        entangledPlayers[guid] = 500;
                    }
                    else
                    {
                        LOG_DEBUG("module.affix.entangling", "'{}' is still entangled, {}ms remaining",
                            player->GetName(), entangledPlayers[guid]);

                        if (entangledPlayers[guid] > diff)
                        {
                            entangledPlayers[guid] -= diff;
                        }
                        else
                        {
                            LOG_DEBUG("module.affix.entangling", "'{}' failed to move in time — applying stun", player->GetName());
                            player->AddAura(3001020, player);          // Stun
                            CleanupEntangledPlayer(player);
                        }
                    }
                }
                else
                {
                    if (entangledPlayers.count(guid))
                    {
                        LOG_DEBUG("module.affix.entangling", "'{}' moved out of range — breaking vine", player->GetName());
                        CleanupEntangledPlayer(player);
                    }
                }
            }
        }

        uint32 GetOrAssignRopeSpell(ObjectGuid guid)
        {
            if (ropeSpells.count(guid))
                return ropeSpells[guid];

            static const uint32 ropeIDs[] = { 3001015, 3001016, 3001017, 3001018, 3001019 };
            uint32 assignedIndex = ropeSpells.size() % std::size(ropeIDs);
            uint32 ropeSpell = ropeIDs[assignedIndex];

            ropeSpells[guid] = ropeSpell;

            LOG_DEBUG("module.affix.entangling", "Assigned rope spell {} to GUID {}", ropeSpell, guid.ToString().c_str());

            return ropeSpell;
        }

        void CleanupEntangledPlayer(Player* player)
        {
            ObjectGuid guid = player->GetGUID();

            if (ropeSpells.count(guid))
            {
                uint32 ropeSpellId = ropeSpells[guid];
                if (me->HasAura(ropeSpellId))
                {
                    me->RemoveAura(ropeSpellId);
                    LOG_DEBUG("module.affix.entangling", "Removed rope aura {} from plant (for '{}')", ropeSpellId, player->GetName());
                }
                ropeSpells.erase(guid);
            }

            if (player->HasAura(3001014))
            {
                player->RemoveAura(3001014);
                LOG_DEBUG("module.affix.entangling", "Removed slow aura from '{}'", player->GetName());
            }

            entangledPlayers.erase(guid);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_entangling_plantAI(creature);
    }
};

void AddSC_npc_entangling_plant()
{
    new npc_entangling_plant();
}
