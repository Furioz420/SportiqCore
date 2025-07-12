// spell_custom_bolstering.cpp

#include "ScriptMgr.h"
#include "Unit.h"
#include "Creature.h"
#include "Player.h"
#include "Log.h"
#include "Cell.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ObjectAccessor.h"
#include "Containers.h"

class spell_custom_bolstering : public UnitScript
{
public:
    spell_custom_bolstering() : UnitScript("spell_custom_bolstering") {}

    void OnUnitDeath(Unit* victim, Unit* /*killer*/) override
    {
        if (!victim || !victim->ToCreature())
        {
            LOG_DEBUG("module.affix.bolstering", "Bolstering: Victim invalid or not a creature.");
            return;
        }

        Creature* deadMob = victim->ToCreature();

        if (!deadMob->HasAura(3000005))
        {
            LOG_DEBUG("module.affix.bolstering", "Bolstering: Creature {} died but does not have aura 3000005.",
                deadMob->GetName().c_str());
            return;
        }

        LOG_DEBUG("module.affix.bolstering", "Bolstering: Creature '{}' (Entry: {}) died with aura 3000005.",
            deadMob->GetName().c_str(), deadMob->GetEntry());

        if (!deadMob->GetMap())
        {
            LOG_DEBUG("module.affix.bolstering", "Bolstering: No map found for dead mob.");
            return;
        }

        // Search nearby units in 30-yard radius
        std::list<Unit*> nearbyUnits;
        Acore::AnyUnitInObjectRangeCheck check(deadMob, 30.0f);
        Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(deadMob, nearbyUnits, check);
        Cell::VisitAllObjects(deadMob, searcher, 30.0f);

        LOG_DEBUG("module.affix.bolstering", "Bolstering: Found {} units near '{}'.", nearbyUnits.size(), deadMob->GetName().c_str());

        uint32 appliedCount = 0;

        for (Unit* unit : nearbyUnits)
        {
            Creature* nearby = unit->ToCreature();
            if (!nearby)
            {
                LOG_DEBUG("module.affix.bolstering", "Bolstering: Skipped non-creature unit.");
                continue;
            }

            if (nearby == deadMob)
            {
                LOG_DEBUG("module.affix.bolstering", "Bolstering: Skipped self.");
                continue;
            }

            if (!nearby->IsAlive())
            {
                LOG_DEBUG("module.affix.bolstering", "Bolstering: Skipped dead creature '{}'.", nearby->GetName().c_str());
                continue;
            }

            if (!nearby->IsInCombat())
            {
                LOG_DEBUG("module.affix.bolstering", "Bolstering: Skipped out-of-combat creature '{}'.", nearby->GetName().c_str());
                continue;
            }

            float distance = nearby->GetDistance(deadMob);
            if (distance > 30.0f)
            {
                LOG_DEBUG("module.affix.bolstering", "Bolstering: Skipped '{}' (out of range at {:.2f} yards).",
                    nearby->GetName().c_str(), distance);
                continue;
            }

            LOG_DEBUG("module.affix.bolstering", "Bolstering: Applying spell 3001006 to '{}' at {:.2f} yards.",
                nearby->GetName().c_str(), distance);

            nearby->CastSpell(nearby, 3001006, true);
            appliedCount++;
        }

        LOG_DEBUG("module.affix.bolstering", "Bolstering: Applied to {} units near '{}'.", appliedCount, deadMob->GetName().c_str());
    }
};

// Standard registration
void AddSC_spell_custom_bolstering()
{
    new spell_custom_bolstering();
}
