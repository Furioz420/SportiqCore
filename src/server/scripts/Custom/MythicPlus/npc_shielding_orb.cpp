// npc_shielding_orb.cpp

#include "ScriptMgr.h"
#include "Creature.h"
#include "Log.h"
#include <unordered_map>

class npc_shielding_orb : public CreatureScript
{
public:
    npc_shielding_orb() : CreatureScript("npc_shielding_orb") {}

    struct npc_shielding_orbAI : public ScriptedAI
    {
        npc_shielding_orbAI(Creature* creature) : ScriptedAI(creature) {}

        std::unordered_map<ObjectGuid, uint32> recentlyShielded;
        uint32 checkTimer = 1000;

        // Custom unit filter: only shield living creatures not owned by players
        struct ShieldingTargetCheck
        {
            Creature* orb;

            ShieldingTargetCheck(Creature* source) : orb(source) {}

            bool operator()(Unit* unit)
            {
                if (!unit || !unit->IsAlive() || unit == orb)
                    return false;

                Creature* creature = unit->ToCreature();
                if (!creature)
                    return false;

                // Skip pets, guardians, totems, mind-controlled units
                if (creature->GetCharmerOrOwnerGUID().IsPlayer())
                {
                    LOG_DEBUG("module.affix.shielding", "Skipping '{}' — owned/controlled by player", creature->GetName());
                    return false;
                }

                return true;
            }
        };

        void UpdateAI(uint32 diff) override
        {
            // Countdown for shield cooldowns
            for (auto it = recentlyShielded.begin(); it != recentlyShielded.end(); )
            {
                if (it->second <= diff)
                    it = recentlyShielded.erase(it);
                else
                {
                    it->second -= diff;
                    ++it;
                }
            }

            if (me->HasUnitState(UNIT_STATE_CASTING))
            {
                return;
            }

            if (checkTimer > diff)
            {
                checkTimer -= diff;
                return;
            }

            checkTimer = 1000;

            std::list<Unit*> nearby;
            ShieldingTargetCheck check(me);
            Acore::UnitListSearcher<ShieldingTargetCheck> searcher(me, nearby, check);
            Cell::VisitAllObjects(me, searcher, 40.0f);

            LOG_DEBUG("module.affix.shielding", "Orb '{}' scanning {} units for shielding", me->GetGUID().ToString().c_str(), nearby.size());

            for (Unit* target : nearby)
            {
                if (!target->IsAlive())
                    continue;

                ObjectGuid guid = target->GetGUID();

                if (recentlyShielded.count(guid))
                {
                    LOG_DEBUG("module.affix.shielding", "Skipping '{}' — recently shielded ({}ms remaining)", target->GetName(), recentlyShielded[guid]);
                    continue;
                }

                if (target->HasAura(3001023))
                {
                    LOG_DEBUG("module.affix.shielding", "Skipping '{}' — still has aura 3001023", target->GetName());
                    continue;
                }

                me->CastSpell(target, 3001023, false); // false = Normal cast with cast bar
                recentlyShielded[guid] = 12000;

                LOG_DEBUG("module.affix.shielding", "Started casting 3001023 on '{}'", target->GetName());
                break;
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_shielding_orbAI(creature);
    }
};

void AddSC_npc_shielding_orb()
{
    new npc_shielding_orb();
}
