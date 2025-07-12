// spell_custom_spiteful.cpp

#include "ScriptMgr.h"
#include "Unit.h"
#include "Creature.h"
#include "Player.h"
#include "Log.h"

class spell_custom_spiteful : public UnitScript
{
public:
    spell_custom_spiteful() : UnitScript("spell_custom_spiteful") { }

    void OnUnitDeath(Unit* victim, Unit* /*killer*/) override
    {
        if (!victim || !victim->ToCreature())
        {
            LOG_DEBUG("module.affix.spiteful", "Spiteful: Invalid or non-creature unit died.");
            return;
        }

        Creature* deadMob = victim->ToCreature();

        if (!deadMob->HasAura(3000009))
        {
            LOG_DEBUG("module.affix.spiteful", "Spiteful: '{}' died without Spiteful aura.", deadMob->GetName().c_str());
            return;
        }

        float x, y, z, o;
        deadMob->GetPosition(x, y, z, o);

        LOG_DEBUG("module.affix.spiteful", "Spiteful: '{}' (Entry: {}) died with aura 3000009. Spawning creature 5001004 at (X: {:.2f}, Y: {:.2f}, Z: {:.2f})",
            deadMob->GetName().c_str(), deadMob->GetEntry(), x, y, z);

        deadMob->SummonCreature(
            5001004,           // Spiteful Shade
            x, y, z, o,
            TEMPSUMMON_TIMED_DESPAWN,
            20000              // Despawn after 20s (or controlled by SAI)
        );
    }
};

// Registration
void AddSC_spell_custom_spiteful()
{
    new spell_custom_spiteful();
}
