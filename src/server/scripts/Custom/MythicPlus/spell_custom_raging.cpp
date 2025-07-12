#include "ScriptMgr.h"
#include "Unit.h"
#include "SpellMgr.h"
#include "SpellAuraEffects.h"

/// Applies raging CC immunity when below 30% HP
class RagingAffix_UnitScript : public UnitScript
{
public:
    RagingAffix_UnitScript() : UnitScript("RagingAffix_UnitScript") {}

    void OnDamage(Unit* attacker, Unit* victim, uint32& /*damage*/) override
    {
        if (!victim || !victim->IsCreature())
            return;

        // Mob must have Raging aura
        if (!victim->HasAura(3000000))
            return;

        // Already enraged?
        if (victim->HasAura(3001001))
            return;

        // Trigger only when at or below 30%
        if (victim->HealthAbovePct(30))
            return;

        victim->CastSpell(victim, 3001001, true); // Apply CC immunity
        victim->RemoveAura(3000000); //Lets remove Raging so it doesnt reproc

        LOG_INFO("module.affix.raging", "Raging triggered: {} (Entry: {}) is now immune to CC.",
            victim->GetName(), victim->GetEntry());
    }
};


void AddSC_spell_custom_raging()
{
    new RagingAffix_UnitScript();
}
