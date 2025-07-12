// spell_custom_shielding.cpp

#include "ScriptMgr.h"
#include "Creature.h"
#include "Log.h"
#include "Cell.h"
#include "CellImpl.h"
#include "ObjectAccessor.h"
#include "GridNotifiersImpl.h"
#include <unordered_set>

class spell_custom_shielding : public UnitScript
{
public:
    spell_custom_shielding() : UnitScript("spell_custom_shielding") {}

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || !unit->ToCreature() || aura->GetId() != 3000012)
            return;

        Creature* creature = unit->ToCreature();

        LOG_DEBUG("module.affix.shielding", "Aura 3000012 applied to '{}'", creature->GetName());

    }

    void OnUnitEnterCombat(Unit* unit, Unit* /*target*/) override
    {
        if (!unit || !unit->ToCreature())
            return;

        Creature* creature = unit->ToCreature();

        if (!creature->HasAura(3000012))
            return;

        LOG_DEBUG("module.affix.shielding", "Creature '{}' entered combat with aura 3000012 — scheduling orb spawn loop", creature->GetName());

        creature->m_Events.AddEvent(new ShieldingOrbSpawnEvent(creature), creature->m_Events.CalculateTime(urand(15000, 25000)));
    }

private:
    class ShieldingOrbSpawnEvent : public BasicEvent
    {
    public:
        explicit ShieldingOrbSpawnEvent(Creature* creature) : _creature(creature) {}

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (!_creature || !_creature->IsAlive())
                return false;

            if (!_creature->HasAura(3000012))
            {
                LOG_DEBUG("module.affix.shielding", "'{}' no longer has aura 3000012 — stopping orb spawns", _creature->GetName());
                return false;
            }

            if (!_creature->IsInCombat())
            {
                LOG_DEBUG("module.affix.shielding", "'{}' is not in combat — skipping this orb spawn", _creature->GetName());
                _creature->m_Events.AddEvent(new ShieldingOrbSpawnEvent(_creature), _creature->m_Events.CalculateTime(5000));
                return true; // Reschedule and wait for combat
            }

            uint32 count = 0;
            std::list<Creature*> orbs;
            _creature->GetCreatureListWithEntryInGrid(orbs, 5001006, 200.0f);

            for (Creature* orb : orbs)
            {
                if (orb && orb->IsAlive())
                    ++count;
            }

            if (count < 2)
            {
                float x, y, z, o;
                _creature->GetPosition(x, y, z, o);

                _creature->SummonCreature(
                    5001006, x, y, z, o,
                    TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,
                    120000
                );

                LOG_DEBUG("module.affix.shielding", "Spawned shielding orb — total now: {}", count + 1);
            }
            else
            {
                LOG_DEBUG("module.affix.shielding", "Shielding orb skipped — {} already active", count);
            }

            // Reschedule next spawn
            _creature->m_Events.AddEvent(new ShieldingOrbSpawnEvent(_creature), _creature->m_Events.CalculateTime(urand(15000, 25000)));
            return true;
        }

    private:
        Creature* _creature;
    };
};

void AddSC_spell_custom_shielding()
{
    new spell_custom_shielding();
}
