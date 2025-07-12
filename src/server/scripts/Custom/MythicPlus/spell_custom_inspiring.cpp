// spell_custom_inspiring.cpp

#include "ScriptMgr.h"
#include "Creature.h"
#include "Player.h"
#include "Log.h"
#include "Cell.h"
#include "CellImpl.h"
#include "ObjectAccessor.h"
#include "GridNotifiersImpl.h"
#include "Unit.h"
#include <unordered_map>
#include <unordered_set>

class spell_custom_inspiring : public UnitScript
{
public:
    spell_custom_inspiring() : UnitScript("spell_custom_inspiring") {}

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || !unit->ToCreature() || aura->GetId() != 3000011)
            return;

        Creature* creature = unit->ToCreature();
        LOG_DEBUG("module.affix.inspiring", "Affix aura 3000011 applied to '{}'", creature->GetName());

        if (!roll_chance_i(50))
        {
            LOG_DEBUG("module.affix.inspiring", "Random roll failed — no Inspiring Presence applied.");
            return;
        }

        if (HasNearbyPresence(creature))
        {
            LOG_DEBUG("module.affix.inspiring", "Nearby Inspiring Presence found — skipping.");
            return;
        }

        LOG_DEBUG("module.affix.inspiring", "No nearby Inspiring Presence — applying aura 3001021 to '{}'", creature->GetName());
        creature->CastSpell(creature, 3001021, true);
    }

    void OnUnitUpdate(Unit* unit, uint32 diff) override
    {
        if (!unit || !unit->ToCreature() || !unit->HasAura(3001021))
            return;

        Creature* source = unit->ToCreature();
        ObjectGuid sourceGuid = source->GetGUID();

        auto& timer = _inspiringTimers[sourceGuid];
        auto& inspired = _inspiredTargets[sourceGuid];

        if (timer > diff)
        {
            timer -= diff;
            return;
        }

        timer = 1000;

        std::list<Unit*> nearbyUnits;
        Acore::AnyUnitInObjectRangeCheck check(source, 15.0f);
        Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(source, nearbyUnits, check);
        Cell::VisitAllObjects(source, searcher, 15.0f);

        LOG_DEBUG("module.affix.inspiring", "Update for '{}': Found {} units in 15yd range",
            source->GetName(), nearbyUnits.size());

        std::unordered_set<ObjectGuid> currentCycle;

        for (Unit* u : nearbyUnits)
        {
            if (!u->IsAlive() || !u->ToCreature())
                continue;

            float dist = source->GetDistance(u);
            Creature* c = u->ToCreature();
            ObjectGuid guid = c->GetGUID();

            if (u->GetGUID() == source->GetGUID())
            {
                LOG_DEBUG("module.affix.inspiring", "Skipped self '{}' at {:.2f} yd", c->GetName(), dist);
                continue;
            }

            LOG_DEBUG("module.affix.inspiring", "Checking '{}': distance = {:.2f} yd", c->GetName(), dist);

            currentCycle.insert(guid);

            if (!c->HasAura(3001022))
            {
                source->AddAura(3001022, c);
                LOG_DEBUG("module.affix.inspiring", "Applied aura 3001022 to '{}' at {:.2f} yd", c->GetName(), dist);
            }

            inspired.insert(guid);
        }

        // Cleanup creatures no longer in range
        for (auto it = inspired.begin(); it != inspired.end(); )
        {
            if (currentCycle.find(*it) == currentCycle.end())
            {
                if (Creature* oldTarget = ObjectAccessor::GetCreature(*source, *it))
                {
                    if (oldTarget->HasAura(3001022))
                    {
                        oldTarget->RemoveAura(3001022);
                        LOG_DEBUG("module.affix.inspiring", "Removed aura 3001022 from '{}' (left range)", oldTarget->GetName());
                    }
                }
                it = inspired.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode /*mode*/) override
    {
        if (!unit || !unit->ToCreature())
            return;

        Aura* aura = aurApp->GetBase();
        if (!aura || aura->GetId() != 3001021)
            return;

        Creature* source = unit->ToCreature();
        ObjectGuid sourceGuid = source->GetGUID();

        LOG_DEBUG("module.affix.inspiring", "Inspiring Presence removed from '{}'", source->GetName());

        if (_inspiredTargets.count(sourceGuid))
        {
            for (auto const& guid : _inspiredTargets[sourceGuid])
            {
                if (Creature* target = ObjectAccessor::GetCreature(*source, guid))
                {
                    if (target->HasAura(3001022))
                    {
                        target->RemoveAura(3001022);
                        LOG_DEBUG("module.affix.inspiring", "Cleanup: Removed aura 3001022 from '{}'", target->GetName());
                    }
                }
            }

            _inspiredTargets.erase(sourceGuid);
        }

        _inspiringTimers.erase(sourceGuid);
    }

private:
    std::unordered_map<ObjectGuid, uint32> _inspiringTimers;
    std::unordered_map<ObjectGuid, std::unordered_set<ObjectGuid>> _inspiredTargets;

    bool HasNearbyPresence(Creature* creature)
    {
        std::list<Unit*> nearbyUnits;
        Acore::AnyUnitInObjectRangeCheck check(creature, 15.0f);
        Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(creature, nearbyUnits, check);
        Cell::VisitAllObjects(creature, searcher, 15.0f);

        for (Unit* u : nearbyUnits)
        {
            if (u == creature || !u->IsAlive() || !u->ToCreature())
                continue;

            if (u->HasAura(3001021))
            {
                LOG_DEBUG("module.affix.inspiring", "Nearby '{}' already has Inspiring Presence", u->GetName());
                return true;
            }
        }

        return false;
    }
};

void AddSC_spell_custom_inspiring()
{
    new spell_custom_inspiring();
}
