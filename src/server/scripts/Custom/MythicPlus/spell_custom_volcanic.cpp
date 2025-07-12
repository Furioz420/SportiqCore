// volcanic_affix_unit_script.cpp

#include "ScriptMgr.h"
#include "Unit.h"
#include "Player.h"
#include "Creature.h"
#include "Log.h"
#include "Containers.h"

class volcanic_affix_unit_script : public UnitScript
{
public:
    volcanic_affix_unit_script() : UnitScript("volcanic_affix_unit_script") {}

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || !unit->IsAlive() || aura->GetId() != 3000004)
            return;

        LOG_DEBUG("module.affix.volcanic", "Volcanic aura applied to {}", unit->GetName());
        unit->m_Events.AddEvent(new VolcanicSpawnEvent(unit), unit->m_Events.CalculateTime(urand(10000, 20000)));
    }

    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode /*mode*/) override
    {
        if (!unit || !aurApp)
            return;

        Aura const* aura = aurApp->GetBase();
        if (!aura || aura->GetId() != 3000004)
            return;

        LOG_DEBUG("module.affix.volcanic", "Volcanic aura removed from {}", unit->GetName());
        // No need to manually cancel events here unless you want to force it;
        // event will stop executing when conditions fail (no aura / not alive)
    }

private:
    class VolcanicSpawnEvent : public BasicEvent
    {
    public:
        explicit VolcanicSpawnEvent(Unit* unit) : _unit(unit) {}

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (!_unit || !_unit->IsAlive() || !_unit->IsInCombat() || !_unit->HasAura(3000004))
                return false;

            Player* target = SelectDistantPlayer(_unit);
            if (target)
            {
                float x, y, z, o;
                target->GetPosition(x, y, z, o);

                LOG_DEBUG("module.affix.volcanic", "{} spawning volcano under {}", _unit->GetName(), target->GetName());

                _unit->SummonCreature(
                    5001001, x, y, z, o,
                    TEMPSUMMON_TIMED_DESPAWN,
                    6000
                );
            }
            else
            {
                LOG_DEBUG("module.affix.volcanic", "{} found no valid distant player", _unit->GetName());
            }

            // Reschedule next volcano
            _unit->m_Events.AddEvent(new VolcanicSpawnEvent(_unit), _unit->m_Events.CalculateTime(urand(10000, 20000)));
            return true;
        }

    private:
        Unit* _unit;

        Player* SelectDistantPlayer(Unit* creature)
        {
            std::vector<Player*> distantPlayers;

            if (Map* map = creature->GetMap())
            {
                for (auto const& playerRef : map->GetPlayers())
                {
                    Player* player = playerRef.GetSource();
                    if (!player || !player->IsAlive())
                        continue;

                    if (player->IsGameMaster() || !player->IsInWorld())
                        continue;

                    float dist = creature->GetDistance(player);
                    if (dist >= 10.0f)
                    {
                        distantPlayers.push_back(player);
                        LOG_DEBUG("module.affix.volcanic", "{} found distant player {} at {:.2f} yards",
                            creature->GetName(), player->GetName(), dist);
                    }
                }
            }

            if (!distantPlayers.empty())
            {
                Player* chosen = Acore::Containers::SelectRandomContainerElement(distantPlayers);
                LOG_DEBUG("module.affix.volcanic", "{} selected {} for volcano", creature->GetName(), chosen->GetName());
                return chosen;
            }

            return nullptr;
        }
    };
};

// Standard registration function
void AddSC_spell_custom_volcanic()
{
    new volcanic_affix_unit_script();
}
