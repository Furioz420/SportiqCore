#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

class spell_bursting_with_pride : public SpellScript
{
    PrepareSpellScript(spell_bursting_with_pride);

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        Unit* target = GetHitUnit();
        if (!caster || !target)
            return;

        int32 base = GetEffectValue();

        uint32 stacks = 0;
        if (Aura* aura = caster->GetAura(3001026))
            stacks = aura->GetStackAmount();

        float modifier = 1.0f + 0.4f * stacks;
        int32 scaled = int32(base * modifier);

        SetHitDamage(scaled);
        LOG_DEBUG("module.affix.prideful", "[Bursting] caster '{}', stacks {}, base {}, scaled {}", caster->GetName(), stacks, base, scaled);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_bursting_with_pride::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }
};

class spell_bursting_with_pride_loader : public SpellScriptLoader
{
public:
    spell_bursting_with_pride_loader() : SpellScriptLoader("spell_bursting_with_pride") {}

    SpellScript* GetSpellScript() const override
    {
        return new spell_bursting_with_pride();
    }
};

void AddSC_spell_bursting_with_pride()
{
    new spell_bursting_with_pride_loader();
}

