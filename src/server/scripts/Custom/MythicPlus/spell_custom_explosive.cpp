// spell_custom_explosive.cpp

#include "ScriptMgr.h"
#include "Unit.h"
#include "Creature.h"
#include "Player.h"
#include "Log.h"
#include "SpellMgr.h"
#include "Random.h"

class spell_custom_explosive : public UnitScript
{
public:
    spell_custom_explosive() : UnitScript("spell_custom_explosive") { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || !unit->IsAlive() || aura->GetId() != 3000007)
            return;

        LOG_INFO("module.affix.explosive", "Explosive aura applied to {}", unit->GetName());

        unit->m_Events.AddEvent(new ExplosiveOrbSpawnEvent(unit), unit->m_Events.CalculateTime(10000));
    }

private:
    class ExplosiveOrbSpawnEvent : public BasicEvent
    {
    public:
        explicit ExplosiveOrbSpawnEvent(Unit* unit) : _unit(unit) {}

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (!_unit || !_unit->IsAlive() || !_unit->IsInCombat() || !_unit->HasAura(3000007))
            {
                LOG_DEBUG("module.affix.explosive", "Explosive: Stop spawning orbs — unit invalid or no longer eligible.");
                return false;
            }

            if (roll_chance_i(20)) // 20% chance every 10s
            {
                Position spawnPos = _unit->GetRandomNearPosition(5.0f);

                if (Creature* orb = _unit->SummonCreature(5001003, spawnPos, TEMPSUMMON_TIMED_DESPAWN, 15000))
                {
                    LOG_DEBUG("module.affix.explosive", "{} spawned Explosive Orb (5001003) at {:.2f}, {:.2f}, {:.2f}",
                        _unit->GetName(), spawnPos.GetPositionX(), spawnPos.GetPositionY(), spawnPos.GetPositionZ());

                    // Optional: Add aura to make orb immune to AoE
                    //orb->CastSpell(orb, XXXXXX, true); // e.g., AoE immunity aura
                }
            }
            else
            {
                LOG_DEBUG("module.affix.explosive", "{} did not trigger orb spawn this cycle.", _unit->GetName());
            }

            // Re-register for next tick
            _unit->m_Events.AddEvent(new ExplosiveOrbSpawnEvent(_unit), _unit->m_Events.CalculateTime(10000));
            return true;
        }

    private:
        Unit* _unit;
    };
};


// Standard registration
void AddSC_spell_custom_explosive()
{
    new spell_custom_explosive();
}
