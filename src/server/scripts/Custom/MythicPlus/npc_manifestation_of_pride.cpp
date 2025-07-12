// npc_manifestation_of_pride.cpp

#include "ScriptMgr.h"
#include "Creature.h"
#include "Player.h"
#include "Log.h"
#include "Containers.h"
#include <list>

class npc_manifestation_of_pride : public CreatureScript
{
public:
    npc_manifestation_of_pride() : CreatureScript("npc_manifestation_of_pride") {}

    struct npc_manifestation_of_prideAI : public ScriptedAI
    {
        npc_manifestation_of_prideAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 burstingTimer = 2000;
        uint32 boastTimer = 7000;

        void Reset() override
        {
            burstingTimer = 2000;
            boastTimer = 7000;

            if (!me->HasAura(3001024))
                me->AddAura(3001024, me);

            LOG_DEBUG("module.affix.prideful", "[Prideful] '{}' reset", me->GetName());
        }

        void JustEngagedWith(Unit* who) override
        {
            if (!me->HasAura(3001024))
                me->AddAura(3001024, me);

            LOG_DEBUG("module.affix.prideful", "[Prideful] '{}' engaged in combat with '{}'", me->GetName(), who->GetName());
        }

        void JustReachedHome() override
        {

            if (!me->HasAura(3001024))
                me->AddAura(3001024, me);

            LOG_DEBUG("module.affix.prideful", "[Prideful] '{}' wiped — cleared stacking aura", me->GetName());
        }

        void UpdateAI(uint32 diff) override
        {

            if (!me->HasAura(3001024))
                me->AddAura(3001024, me);

            if (!UpdateVictim())
                return;

            if (burstingTimer <= diff)
            {
                me->CastSpell(me, 3001025, false); // Bursting With Pride
                me->AddAura(3001026, me) ;// Add aura to self aswell for scaling

                std::list<Unit*> units;
                Acore::AnyUnitInObjectRangeCheck check(me, 8.0f);
                Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(me, units, check);
                Cell::VisitAllObjects(me, searcher, 8.0f);

                for (Unit* unit : units)
                {
                    if (Player* player = unit->ToPlayer())
                    {
                        me->AddAura(3001026, player);
                        LOG_DEBUG("module.affix.prideful", "[Prideful] Applied Bursting Aura (3001026) to '{}'", player->GetName());
                    }
                }

                LOG_DEBUG("module.affix.prideful", "[Prideful] '{}' cast Bursting With Pride + stacking aura", me->GetName());

                burstingTimer = 2000;
            }
            else
            {
                burstingTimer -= diff;
            }

            if (boastTimer <= diff)
            {
                me->CastSpell(me, 3001027, false); // Visual to show we will apply 3001028

                if (Unit* target = SelectRandomPlayer())
                {
                    me->AddAura(3001028, target); // Applies 3001028
                    LOG_DEBUG("module.affix.prideful", "[Prideful] '{}' cast Belligerent Boast on '{}'", me->GetName(), target->GetName());
                }

                boastTimer = 7000;
            }
            else
            {
                boastTimer -= diff;
            }

            DoMeleeAttackIfReady();
        }

        Unit* SelectRandomPlayer()
        {
            std::list<Unit*> nearby;
            Acore::AnyUnitInObjectRangeCheck check(me, 100.0f);
            Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(me, nearby, check);
            Cell::VisitAllObjects(me, searcher, 100.0f);

            std::vector<Player*> players;
            for (Unit* unit : nearby)
            {
                if (Player* player = unit->ToPlayer())
                    players.push_back(player);
            }

            if (players.empty())
                return nullptr;

            return Acore::Containers::SelectRandomContainerElement(players);
        }
        void JustDied(Unit* /*killer*/) override
        {
            LOG_INFO("module.affix.prideful", "[Prideful] '{}' died — applying 3001030 to players", me->GetName());

            std::set<Player*> uniquePlayers;

            // Nearby players fallback
            if (Map* map = me->GetMap())
            {
                for (auto const& ref : map->GetPlayers())
                {
                    Player* player = ref.GetSource();
                    if (!player || !player->IsAlive())
                        continue;

                    if (player->IsInCombat() && player->IsWithinDistInMap(me, 100.0f))
                        uniquePlayers.insert(player);
                }
            }

            // Apply the buff
            for (Player* player : uniquePlayers)
            {
                me->AddAura(3001030, player);
                me->AddAura(3001031, player);
                LOG_INFO("module.affix.prideful", "[Prideful] Applied buff 3001030 to '{}'", player->GetName());
            }
        }

    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_manifestation_of_prideAI(creature);
    }
};

void AddSC_npc_manifestation_of_pride()
{
    new npc_manifestation_of_pride();
}
