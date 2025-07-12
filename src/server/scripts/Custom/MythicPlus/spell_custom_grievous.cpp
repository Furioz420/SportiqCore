#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Unit.h"

// This UnitScript handles the healing logic for any unit with the DOT aura
class GrievousWound_UnitScript : public UnitScript
{
public:
    GrievousWound_UnitScript() : UnitScript("GrievousWound_UnitScript") { }

    void OnHeal(Unit* target, Unit* /*healer*/, uint32& /*healing*/) override
    {
        if (!target || !target->HasAura(3001000))
            return;

        if (target->GetHealthPct() > 50.0f)
        {
            Aura* aura = target->GetAura(3001000);
            if (!aura)
                return;

            uint8 stacks = aura->GetStackAmount();
            if (stacks <= 1)
                target->RemoveAura(3001000);
            else
                aura->SetStackAmount(stacks - 1);
        }
    }
};

void AddSC_spell_custom_grievous()
{
    new GrievousWound_UnitScript();
}
