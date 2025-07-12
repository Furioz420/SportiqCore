// spell_custom_entangling.cpp

#include "ScriptMgr.h"
#include "Creature.h"
#include "Player.h"
#include "Group.h"
#include "Log.h"
#include "Containers.h"

class spell_custom_entangling : public UnitScript
{
public:
    spell_custom_entangling() : UnitScript("spell_custom_entangling") {}

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || !unit->IsAlive() || !unit->ToCreature())
            return;

        if (aura->GetId() != 3000010)
            return;

        Creature* creature = unit->ToCreature();

        LOG_DEBUG("module.affix.entangling", "Entangling: Aura 3000010 applied to creature '{}' (Entry: {})",
            creature->GetName(), creature->GetEntry());

        // Start spawn loop
        creature->m_Events.AddEvent(new EntanglingSpawnEvent(creature), creature->m_Events.CalculateTime(urand(10000, 15000)));
    }

private:
    class EntanglingSpawnEvent : public BasicEvent
    {
    public:
        explicit EntanglingSpawnEvent(Creature* creature) : _creature(creature) {}

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (!_creature || !_creature->IsAlive())
                return false;

            if (!_creature->HasAura(3000010))
            {
                LOG_DEBUG("module.affix.entangling", "Entangling: Creature lost aura — stopping.");
                return false;
            }

            if (!_creature->IsInCombat())
            {
                LOG_DEBUG("module.affix.entangling", "Entangling: Creature not in combat — delaying.");
                _creature->m_Events.AddEvent(new EntanglingSpawnEvent(_creature), _creature->m_Events.CalculateTime(5000));
                return true;
            }

            // Find nearby players
            std::vector<Player*> players;

            if (Map* map = _creature->GetMap())
            {
                for (auto const& ref : map->GetPlayers())
                {
                    Player* player = ref.GetSource();
                    if (!player || !player->IsAlive() || !player->IsInWorld())
                        continue;

                    float dist = _creature->GetDistance(player);
                    if (dist <= 100.0f)
                        players.push_back(player);
                }
            }

            if (players.empty())
            {
                LOG_DEBUG("module.affix.entangling", "Entangling: No nearby players found.");
            }
            else
            {
                Player* target = Acore::Containers::SelectRandomContainerElement(players);
                if (!target)
                    return true;

                // Determine group key
                Group* group = target->GetGroup();
                uint64 groupId = group ? group->GetLeaderGUID().GetRawValue() : target->GetGUID().GetRawValue();

                if (HasActivePlantForGroup(_creature->GetMap(), groupId))
                {
                    LOG_DEBUG("module.affix.entangling", "Entangling: Plant already active for this group — skipping spawn.");
                }
                else
                {
                    float x, y, z, o;
                    target->GetPosition(x, y, z, o);

                    LOG_DEBUG("module.affix.entangling", "Entangling: Spawning plant under player '{}'", target->GetName());

                    _creature->SummonCreature(
                        5001005, x, y, z, o,
                        TEMPSUMMON_TIMED_DESPAWN,
                        10000
                    );
                }
            }

            // Re-run every 10–15s
            _creature->m_Events.AddEvent(new EntanglingSpawnEvent(_creature), _creature->m_Events.CalculateTime(urand(10000, 15000)));
            return true;
        }

    private:
        Creature* _creature;

        bool HasActivePlantForGroup(Map* map, uint64 groupId)
        {
            for (auto const& ref : map->GetPlayers())
            {
                Player* player = ref.GetSource();
                if (!player || !player->IsAlive())
                    continue;

                Group* playerGroup = player->GetGroup();
                uint64 playerGroupId = playerGroup ? playerGroup->GetLeaderGUID().GetRawValue() : player->GetGUID().GetRawValue();

                if (playerGroupId != groupId)
                    continue;

                std::list<Creature*> nearbyPlants;
                player->GetCreatureListWithEntryInGrid(nearbyPlants, 5001005, 100.0f);

                for (Creature* plant : nearbyPlants)
                {
                    if (plant && plant->IsAlive())
                        return true;
                }
            }

            return false;
        }


    };
};

void AddSC_spell_custom_entangling()
{
    new spell_custom_entangling();
}
