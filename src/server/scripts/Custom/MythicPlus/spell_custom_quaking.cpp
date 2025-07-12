// spell_custom_quaking.cpp

#include "ScriptMgr.h"
#include "Player.h"
#include "Log.h"
#include "SpellMgr.h"

class spell_custom_quaking : public UnitScript
{
public:
    spell_custom_quaking() : UnitScript("spell_custom_quaking") {}

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || !unit->IsAlive() || !unit->IsPlayer())
            return;

        if (aura->GetId() != 3000008)
            return;

        Player* player = unit->ToPlayer();
        LOG_DEBUG("module.affix.quaking", "Quaking: Aura 3000008 applied to player '{}'", player->GetName());

        player->m_Events.AddEvent(new QuakingCycleEvent(player), player->m_Events.CalculateTime(20000));
    }

    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode /*mode*/) override
    {
        if (!unit || !unit->IsPlayer() || !aurApp)
            return;

        Aura const* aura = aurApp->GetBase();
        if (!aura || aura->GetId() != 3000008)
            return;

        Player* player = unit->ToPlayer();
        LOG_DEBUG("module.affix.quaking", "Quaking: Aura 3000008 removed from '{}', cancelling quaking loop.", player->GetName());

        // Events auto-clean with player, but log helps with debug
    }

private:
    class QuakingCycleEvent : public BasicEvent
    {
    public:
        explicit QuakingCycleEvent(Player* player) : _player(player) {}

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (!_player || !_player->IsAlive() || !_player->HasAura(3000008))
            {
                LOG_DEBUG("module.affix.quaking", "Quaking: Player invalid or lost aura. Canceling loop.");
                return false;
            }

            LOG_DEBUG("module.affix.quaking", "Quaking: Applying warning aura 53455 to '{}'", _player->GetName());
            _player->CastSpell(_player, 53455, true);

            // 2.5s later, do the actual proximity check + aura
            _player->m_Events.AddEvent(new QuakingProximityCheck(_player), _player->m_Events.CalculateTime(2500));

            // Reschedule next cycle in 20s
            _player->m_Events.AddEvent(new QuakingCycleEvent(_player), _player->m_Events.CalculateTime(20000));
            return true;
        }

    private:
        Player* _player;
    };

    class QuakingProximityCheck : public BasicEvent
    {
    public:
        explicit QuakingProximityCheck(Player* player) : _player(player) {}

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (!_player || !_player->IsAlive() || !_player->HasAura(3000008))
            {
                LOG_DEBUG("module.affix.quaking", "Quaking: Proximity check skipped — player invalid or aura gone.");
                return false;
            }

            LOG_DEBUG("module.affix.quaking", "Quaking: Checking proximity for '{}'", _player->GetName());

            bool triggered = false;
            if (Map* map = _player->GetMap())
            {
                for (auto const& ref : map->GetPlayers())
                {
                    Player* other = ref.GetSource();
                    if (!other || other == _player || !other->IsAlive())
                        continue;

                    float dist = _player->GetDistance(other);
                    if (dist <= 5.0f)
                    {
                        LOG_DEBUG("module.affix.quaking", "Quaking: '{}' is within 5yd of '{}'. Applying aura 3001010.",
                            other->GetName(), _player->GetName());

                        other->AddAura(3001010, other);
                        triggered = true;
                    }
                }
            }

            if (!triggered)
            {
                LOG_DEBUG("module.affix.quaking", "Quaking: No players near '{}'. No aura applied.", _player->GetName());
            }

            // Always remove the 53455 aura to signify end of cast window
            if (_player->HasAura(53455))
            {
                _player->RemoveAura(53455);
                LOG_DEBUG("module.affix.quaking", "Quaking: Removed warning aura 53455 from '{}'", _player->GetName());
            }

            return true;
        }

    private:
        Player* _player;
    };
};

// Standard registration
void AddSC_spell_custom_quaking()
{
    new spell_custom_quaking();
}
