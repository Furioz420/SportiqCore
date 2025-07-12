// spell_custom_sanguine.cpp

#include "ScriptMgr.h"
#include "Unit.h"
#include "Creature.h"
#include "Player.h"
#include "Log.h"

class spell_custom_sanguine : public UnitScript
{
public:
    spell_custom_sanguine() : UnitScript("spell_custom_sanguine") { }

    void OnUnitDeath(Unit* victim, Unit* /*killer*/) override
    {
        if (!victim || !victim->ToCreature())
        {
            LOG_DEBUG("module.affix.sanguine", "Sanguine: Invalid or non-creature unit died.");
            return;
        }

        Creature* deadMob = victim->ToCreature();

        if (!deadMob->HasAura(3000006))
        {
            LOG_DEBUG("module.affix.sanguine", "Sanguine: '{}' died without Sanguine aura.", deadMob->GetName().c_str());
            return;
        }

        Position pos;
        float x, y, z, o;
        deadMob->GetPosition(x, y, z, o);

        LOG_DEBUG("module.affix.sanguine", "Sanguine: '{}' (Entry: {}) died with aura 3000006. Spawning puddle at (X: {:.2f}, Y: {:.2f}, Z: {:.2f})",
            deadMob->GetName().c_str(), deadMob->GetEntry(), x, y, z);

        deadMob->SummonCreature(
            5001002,
            x, y, z, o,
            TEMPSUMMON_TIMED_DESPAWN,
            20000 // 20s
        );
    }
};

void AddSC_spell_custom_sanguine()
{
    new spell_custom_sanguine();
}
