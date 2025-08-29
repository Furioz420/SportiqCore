#include "SpellMgr.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Unit.h"
#include "Player.h"
#include "Log.h"
#include "MoveSplineInit.h"


enum DHSpells
{
    SPELL_DEMONHUNTER_LEECH_HEAL = 98779,
    SPELL_THE_HUNT_HEAL = 100067,
    SPELL_SANGUINE_CORRUPTION_PASSIVE = 100109,
    SPELL_SANGUINE_CORRUPTION_HEAL = 100110,
    SPELL_SANGUINE_CORRUPTION_DAMAGE = 100111,
};

enum DHMasterySpells
{
    SPELL_DEMONIC_PRESENCE = 100082, // ID do novo feitiço
};

enum RagefireSpells
{
    SPELL_IMMOLATION_AURA = 100002,
    SPELL_IMMOLATION_PERIODIC = 100003,
    SPELL_RAGEFIRE_PROC = 100071,
    SPELL_RAGEFIRE_EXPLOSION = 100072,
    SPELL_RAGEFIRE_RANK_1 = 100068,
    SPELL_RAGEFIRE_RANK_2 = 100069,
    SPELL_RAGEFIRE_RANK_3 = 100070,
    SPELL_DASH_OF_CHAOS = 100076,
};

enum FirebornRenewalSpells
{
    SPELL_FIREBORN_RENEWAL_RANK_1 = 100114,
    SPELL_FIREBORN_RENEWAL_RANK_2 = 100115,
    SPELL_FIREBORN_RENEWAL_RANK_3 = 100116,
    SPELL_FIREBORN_RENEWAL_RANK_4 = 100117,
    SPELL_FIREBORN_RENEWAL_RANK_5 = 100118,
    SPELL_FIREBORN_RENEWAL_HEAL = 100119
};

enum SpiritBombSpells
{
    SPELL_SPIRIT_BOMB_ABSORB = 100093, // Feitiço de absorção
    SPELL_SPIRIT_BOMB_EXPLOSION = 100094  // Explosão e cura ao expirar
};

class spell_dh_fel_rush : public SpellScriptLoader
{
public:
    spell_dh_fel_rush() : SpellScriptLoader("spell_dh_fel_rush") {}

private:
    struct SavedPosition
    {
        float x, y, z, orientation;
    };

    static SavedPosition m_SavedPosition;

public:
    class spell_dh_fel_rush_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dh_fel_rush_SpellScript);

        void HandleAfterCast()
        {
            Player* caster = GetCaster()->ToPlayer();
            if (!caster || !caster->IsAlive())
                return;

            // Salvar posição inicial caso tenha o talento DASH OF CHAOS
            if (caster->HasSpell(SPELL_DASH_OF_CHAOS))
            {
                m_SavedPosition.x = caster->GetPositionX();
                m_SavedPosition.y = caster->GetPositionY();
                m_SavedPosition.z = caster->GetPositionZ();
                m_SavedPosition.orientation = caster->GetOrientation();
                LOG_INFO("custom.spell", "Fel Rush: Saved initial position for teleport.");
            }

            float distance = 22.3f;
            float speed = 49.5f;  // Ajustado para não ultrapassar o limite de 50.0f
            float orientation = caster->GetOrientation();

            // Calcular a posição inicial do dash
            float destX = caster->GetPositionX() + std::cos(orientation) * distance;
            float destY = caster->GetPositionY() + std::sin(orientation) * distance;
            float destZ = caster->GetPositionZ();

            // Verificar colisões antes de definir a posição final
            float groundZ = caster->GetMap()->GetHeight(destX, destY, destZ, true);
            if (groundZ == INVALID_HEIGHT)
            {
                groundZ = caster->GetPositionZ(); // Fallback se a altura for inválida
            }

            // Prevenir quedas e atravessar o chão
            if (groundZ > caster->GetPositionZ())
            {
                destZ = groundZ;  // Ajustar para a altura do terreno
            }

            // Prevenir atravessar montanhas: se a posição final for muito alta, cancelar o dash
            if (groundZ - caster->GetPositionZ() > 5.0f) // Ajuste conforme necessário
            {
                LOG_INFO("custom.spell", "Fel Rush: Destination too high, canceling dash.");
                return;
            }

            // Prevenir atravessar paredes
            if (!caster->IsWithinLOS(destX, destY, destZ))
            {
                LOG_INFO("custom.spell", "Fel Rush: Collision detected, adjusting position.");
                return;
            }

            // Iniciar o movimento do dash
            caster->GetMotionMaster()->MoveFelRush(destX, destY, destZ, speed, EVENT_CHARGE, true);
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_dh_fel_rush_SpellScript::HandleAfterCast);
        }
    };

    class spell_dh_fel_rush_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_dh_fel_rush_AuraScript);

        void OnEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->IsPlayer())
                return;

            Player* player = caster->ToPlayer();

            // Teleportar de volta se tiver o talento DASH OF CHAOS e estiver em combate
            if (player->HasSpell(SPELL_DASH_OF_CHAOS) && player->IsInCombat())
            {
                player->NearTeleportTo(
                    m_SavedPosition.x,
                    m_SavedPosition.y,
                    m_SavedPosition.z,
                    m_SavedPosition.orientation,
                    true);
                player->SendPlaySpellVisual(9884);
            }
        }

        void Register() override
        {
            AfterEffectRemove += AuraEffectRemoveFn(spell_dh_fel_rush_AuraScript::OnEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_dh_fel_rush_SpellScript();
    }

    AuraScript* GetAuraScript() const override
    {
        return new spell_dh_fel_rush_AuraScript();
    }
};

spell_dh_fel_rush::SavedPosition spell_dh_fel_rush::m_SavedPosition = { 0.0f, 0.0f, 0.0f, 0.0f };



class spell_dh_leech : public AuraScript
{
    PrepareAuraScript(spell_dh_leech);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_DEMONHUNTER_LEECH_HEAL });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();

        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
            return;

        Unit* caster = eventInfo.GetActor();
        int32 selfHeal = CalculatePct(static_cast<int32>(damageInfo->GetDamage()), aurEff->GetAmount());

        // Cast the healing spell on self with base points = selfHeal
        caster->CastCustomSpell(SPELL_DEMONHUNTER_LEECH_HEAL, SPELLVALUE_BASE_POINT0, selfHeal, caster, true);
    }


    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_dh_leech::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_dh_the_hunt_dot : public AuraScript
{
    PrepareAuraScript(spell_dh_the_hunt_dot);

    void OnPeriodic(AuraEffect const* aurEff)
    {
        Unit* caster = GetCaster();
        Unit* target = GetTarget();

        if (!caster || !target || !target->IsAlive())
            return;

        int32 damage = aurEff->GetAmount();

        if (damage <= 0)
            return;

        int32 healAmount = CalculatePct(damage, 20);

        caster->CastCustomSpell(caster, 100067, &healAmount, nullptr, nullptr, true);

    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_dh_the_hunt_dot::OnPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
    }
};

class spell_ragefire_immolation : public SpellScriptLoader
{
public:
    spell_ragefire_immolation() : SpellScriptLoader("spell_ragefire_immolation") {}

    static uint32 storedDamage;

    class spell_ragefire_immolation_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_ragefire_immolation_SpellScript);

        void HandleOnHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();

            if (!caster || !target || !caster->IsPlayer())
            {
                LOG_ERROR("custom.spell", "Ragefire: Invalid caster or target in HandleOnHit.");
                return;
            }

            Player* player = caster->ToPlayer();
            int32 damage = GetHitDamage();

            LOG_INFO("custom.spell", "Ragefire: Damage from SPELL_IMMOLATION_PERIODIC = %d.", damage);

            static uint32 percentage;

            if (player->HasSpell(SPELL_RAGEFIRE_RANK_1))
                percentage = 30.0f;
            else if (player->HasSpell(SPELL_RAGEFIRE_RANK_2))
                percentage = 50.0f;
            else if (player->HasSpell(SPELL_RAGEFIRE_RANK_3))
                percentage = 60.0f;

            int32 capturedDamage = CalculatePct(damage, percentage);
            spell_ragefire_immolation::storedDamage += capturedDamage;

            LOG_INFO("custom.spell", "Ragefire: Captured damage = %d, Total stored damage = %u.", capturedDamage, spell_ragefire_immolation::storedDamage);
        }

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->IsPlayer())
            {
                LOG_ERROR("custom.spell", "Ragefire: Caster is null or not a player in HandleAfterCast.");
                return;
            }

            Player* player = caster->ToPlayer();

            if (!player->HasSpell(SPELL_RAGEFIRE_RANK_1) && !player->HasSpell(SPELL_RAGEFIRE_RANK_2) && !player->HasSpell(SPELL_RAGEFIRE_RANK_3))
                return;


            if (!player->HasAura(SPELL_RAGEFIRE_PROC))
            {
                player->CastSpell(player, SPELL_RAGEFIRE_PROC, true);
                LOG_INFO("custom.spell", "Ragefire: Applied aura SPELL_RAGEFIRE_PROC (100071) on cast.");
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_ragefire_immolation_SpellScript::HandleOnHit, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
            AfterCast += SpellCastFn(spell_ragefire_immolation_SpellScript::HandleAfterCast);
        }
    };

    class spell_ragefire_immolation_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_ragefire_immolation_AuraScript);

        void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->IsPlayer())
            {
                LOG_ERROR("custom.spell", "Ragefire: Caster is null or not a player in HandleEffectRemove.");
                return;
            }

            Player* player = caster->ToPlayer();

            if (!player->HasSpell(SPELL_RAGEFIRE_RANK_1) && !player->HasSpell(SPELL_RAGEFIRE_RANK_2) && !player->HasSpell(SPELL_RAGEFIRE_RANK_3))
                return;

            if (player->HasAura(SPELL_RAGEFIRE_PROC))
            {
                player->RemoveAura(SPELL_RAGEFIRE_PROC);
                LOG_INFO("custom.spell", "Ragefire: Removed aura SPELL_RAGEFIRE_PROC (100071).");
            }

            static uint32 multiplier;

            if (player->HasSpell(SPELL_RAGEFIRE_RANK_1))
                multiplier = 1.5f;
            else if (player->HasSpell(SPELL_RAGEFIRE_RANK_2))
                multiplier = 2.0f;
            else if (player->HasSpell(SPELL_RAGEFIRE_RANK_3))
                multiplier = 2.5f;


            int32 finalDamage = spell_ragefire_immolation::storedDamage * multiplier;

            player->CastCustomSpell(player, SPELL_RAGEFIRE_EXPLOSION, &finalDamage, nullptr, nullptr, true);
            spell_ragefire_immolation::storedDamage = 0;
        }

        void Register() override
        {
            AfterEffectRemove += AuraEffectRemoveFn(spell_ragefire_immolation_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_ragefire_immolation_SpellScript();
    }

    AuraScript* GetAuraScript() const override
    {
        return new spell_ragefire_immolation_AuraScript();
    }
};

uint32 spell_ragefire_immolation::storedDamage = 0;

class spell_dh_demonic_presence : public AuraScript
{
    PrepareAuraScript(spell_dh_demonic_presence);

    static constexpr uint32 AUTO_CAST_SPELL_ID = 100087;

    void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        if (caster && caster->IsPlayer())
        {
            Player* player = caster->ToPlayer();
            int32 agility = player->GetStat(STAT_AGILITY);

            // Calcula o tempo de tick baseado na agilidade
            float baseCooldown = 5.0f;                      // Tempo base: 5 segundos
            float reductionPerAgilityPercent = 0.1f;        // Redução de 0.1s por 1% de agilidade
            float agilityPercent = agility * 0.01f;         // Percentual de agilidade

            uint32 newTickTime = static_cast<uint32>(
                std::max(500.0f, (baseCooldown - (agilityPercent * reductionPerAgilityPercent)) * 1000.0f)
                ); // Tempo mínimo de 500 ms

            // Ajusta o intervalo do efeito periódico
            const_cast<AuraEffect*>(aurEff)->SetPeriodicTimer(newTickTime);

            LOG_INFO("custom.spell", "Demonic Presence: Agility = %d, TickTime = %u ms", agility, newTickTime);
        }
    }

    void OnPeriodic(AuraEffect const* aurEff)
    {
        Unit* caster = GetCaster();
        if (caster && caster->IsPlayer())
        {
            caster->CastSpell(caster, AUTO_CAST_SPELL_ID, true);
            LOG_INFO("custom.spell", "Auto-cast triggered: Spell ID %u", AUTO_CAST_SPELL_ID);
        }
    }

    void OnCalculate(AuraEffect const* aurEff, int32& amount, bool& /*canBeRecalculated*/)
    {
        Unit* caster = GetCaster();
        if (!caster || !caster->IsPlayer())
            return;

        Player* player = caster->ToPlayer();
        int32 agility = player->GetStat(STAT_AGILITY);

        amount = static_cast<int32>(agility * 0.09f * 0.005f);
        LOG_INFO("custom.spell", "Demonic Presence: Agility = %d, Bonus Amount = %d", agility, amount);
    }

    void Register() override
    {
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dh_demonic_presence::OnCalculate, EFFECT_0, SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT);
        DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dh_demonic_presence::OnCalculate, EFFECT_1, SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT);
        AfterEffectApply += AuraEffectApplyFn(spell_dh_demonic_presence::OnApply, EFFECT_2, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_dh_demonic_presence::OnPeriodic, EFFECT_2, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

class spell_spirit_bomb : public SpellScriptLoader
{
public:
    spell_spirit_bomb() : SpellScriptLoader("spell_spirit_bomb") {}

    static uint32 storedDamage; // Armazena o dano absorvido

    class spell_spirit_bomb_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_spirit_bomb_AuraScript);

        void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->IsPlayer())
                return;

            Player* player = caster->ToPlayer();
            amount = CalculatePct(player->GetMaxHealth(), 50); // Absorve 50% da vida máxima
            canBeRecalculated = true;
        }

        void Absorb(AuraEffect* /*aurEff*/, DamageInfo& dmgInfo, uint32& absorbAmount)
        {
            absorbAmount = dmgInfo.GetDamage(); // Absorve todo o dano possível
            spell_spirit_bomb::storedDamage += absorbAmount; // Acumula o dano absorvido
        }

        void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->IsPlayer())
                return;

            Player* player = caster->ToPlayer();

            if (spell_spirit_bomb::storedDamage > 0)
            {
                int32 finalDamage = spell_spirit_bomb::storedDamage;
                spell_spirit_bomb::storedDamage = 0; // Reseta o dano armazenado

                // Explode causando dano aos inimigos próximos e curando aliados
                player->CastCustomSpell(player, 100094, &finalDamage, &finalDamage, nullptr, true);
            }
        }

        void Register() override
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_spirit_bomb_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            OnEffectAbsorb += AuraEffectAbsorbFn(spell_spirit_bomb_AuraScript::Absorb, EFFECT_0);
            AfterEffectRemove += AuraEffectRemoveFn(spell_spirit_bomb_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_spirit_bomb_AuraScript();
    }
};

// Variável estática para armazenar o dano absorvido
uint32 spell_spirit_bomb::storedDamage = 0;

class spell_fireball_embraceness : public SpellScriptLoader
{
public:
    spell_fireball_embraceness() : SpellScriptLoader("spell_fireball_embraceness") {}

    class spell_fireball_embraceness_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_fireball_embraceness_SpellScript);

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->IsPlayer())
                return;

            Player* player = caster->ToPlayer();
            const uint32 PROC_BUFF = 100097;
            const uint32 REQUIRED_SPELL = 100098; // Feitiço necessário para ganhar os stacks
            const uint32 STACK_AMOUNT = 3;
            const float PROC_CHANCE = 33.0f; // Ajuste a chance conforme necessário (15% de chance)

            // Verifica se o jogador tem o feitiço 100098 aprendido
            if (!player->HasSpell(REQUIRED_SPELL))
                return;

            // Verifica se o buff já está no jogador
            Aura* buff = player->GetAura(PROC_BUFF);
            if (!buff && roll_chance_f(PROC_CHANCE))
            {
                // Aplica o buff com 3 stacks
                player->CastCustomSpell(player, PROC_BUFF, nullptr, nullptr, nullptr, true);
                player->SetAuraStack(PROC_BUFF, player, STACK_AMOUNT);
                LOG_INFO("custom.spell", "Granted 3 stacks of spell %u.", PROC_BUFF);
            }
            else if (buff && buff->GetStackAmount() > 0)
            {
                // Remove 1 stack do buff
                uint32 currentStacks = buff->GetStackAmount();
                if (currentStacks > 1)
                {
                    buff->SetStackAmount(currentStacks - 1);
                    LOG_INFO("custom.spell", "Removed 1 stack of spell %u. Remaining: %u.", PROC_BUFF, currentStacks - 1);
                }
                else
                {
                    player->RemoveAura(PROC_BUFF);
                    LOG_INFO("custom.spell", "Removed last stack of spell %u.", PROC_BUFF);
                }
            }
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_fireball_embraceness_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_fireball_embraceness_SpellScript();
    }
};

class spell_sanguine_corruption : public AuraScript
{
    PrepareAuraScript(spell_sanguine_corruption);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_SANGUINE_CORRUPTION_HEAL });
    }

    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        Unit* caster = eventInfo.GetActor();

        if (!caster || !caster->IsPlayer())
            return;

        Player* player = caster->ToPlayer();

        // Verifica se o jogador tem o feitiço passivo 100109 (Sanguine Corruption Passive)
        if (!player->HasSpell(SPELL_SANGUINE_CORRUPTION_PASSIVE))
            return;

        DamageInfo* damageInfo = eventInfo.GetDamageInfo();
        if (!damageInfo || !damageInfo->GetDamage())
            return;

        // Calcula 40% do dano causado como cura
        int32 selfHeal = CalculatePct(damageInfo->GetDamage(), 40.0f);
        int32 selfDamage = CalculatePct(damageInfo->GetDamage(), 30.0f);

        // Aplica a cura em área
        player->CastCustomSpell(player, SPELL_SANGUINE_CORRUPTION_DAMAGE, &selfDamage, nullptr, nullptr, true);
        player->CastCustomSpell(player, SPELL_SANGUINE_CORRUPTION_HEAL, nullptr, &selfHeal, nullptr, true);
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_sanguine_corruption::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

class spell_fireborn_renewal : public AuraScript
{
    PrepareAuraScript(spell_fireborn_renewal);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_FIREBORN_RENEWAL_HEAL });
    }

    // ??? Captura dano de feitiços diretos e triggerados
    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        PreventDefaultAction();
        Unit* caster = eventInfo.GetActor();
        if (!caster || !caster->IsPlayer())
            return;

        Player* player = caster->ToPlayer();
        DamageInfo* damageInfo = eventInfo.GetDamageInfo();

        if (!damageInfo || !damageInfo->GetDamage())
            return;

        int32 healPct = GetHealPercentage(player);
        if (healPct == 0)
            return;

        int32 healAmount = CalculatePct(damageInfo->GetDamage(), healPct);
        player->CastCustomSpell(player, SPELL_FIREBORN_RENEWAL_HEAL, &healAmount, nullptr, nullptr, true);
    }

    // ?? Captura dano periódico (exemplo: Auras que aplicam feitiços triggerados)
    void HandlePeriodic(AuraEffect const* aurEff)
    {
        Unit* caster = GetCaster();
        if (!caster || !caster->IsPlayer())
            return;

        Player* player = caster->ToPlayer();
        int32 damage = aurEff->GetAmount();
        if (damage <= 0)
            return;

        int32 healPct = GetHealPercentage(player);
        if (healPct == 0)
            return;

        int32 healAmount = CalculatePct(damage, healPct);
        player->CastCustomSpell(player, SPELL_FIREBORN_RENEWAL_HEAL, &healAmount, nullptr, nullptr, true);
    }

    // ?? Obtém a porcentagem de cura de acordo com o rank
    int32 GetHealPercentage(Player* player)
    {
        if (player->HasSpell(SPELL_FIREBORN_RENEWAL_RANK_5)) return 12;
        if (player->HasSpell(SPELL_FIREBORN_RENEWAL_RANK_4)) return 10;
        if (player->HasSpell(SPELL_FIREBORN_RENEWAL_RANK_3)) return 8;
        if (player->HasSpell(SPELL_FIREBORN_RENEWAL_RANK_2)) return 6;
        if (player->HasSpell(SPELL_FIREBORN_RENEWAL_RANK_1)) return 4;
        return 0;
    }

    void Register() override
    {
        // ?? Captura dano direto e feitiços ativados
        OnEffectProc += AuraEffectProcFn(spell_fireborn_renewal::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);

        // ?? Captura dano periódico de feitiços triggerados
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_fireborn_renewal::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
    }
};

class spell_soul_consume : public SpellScriptLoader
{
public:
    spell_soul_consume() : SpellScriptLoader("spell_soul_consume") {}

    class spell_soul_consume_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_soul_consume_SpellScript);

        SpellCastResult CheckCast()
        {
            Unit* caster = GetCaster();
            if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                return SPELL_FAILED_DONT_REPORT;

            Player* player = caster->ToPlayer();

            if (!player->HasAura(100121) || player->GetAura(100121)->GetStackAmount() == 0)
                return SPELL_FAILED_CASTER_AURASTATE;

            return SPELL_CAST_OK;
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_soul_consume_SpellScript::CheckCast);
        }
    };

    class spell_soul_consume_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_soul_consume_AuraScript);

        void HandlePeriodicTick(AuraEffect const* /*aurEff*/)
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->IsPlayer())
                return;

            Player* player = caster->ToPlayer();
            Aura* soulStacks = player->GetAura(100121);

            if (!soulStacks || soulStacks->GetStackAmount() <= 0)
            {
                player->InterruptSpell(CURRENT_CHANNELED_SPELL);
                player->RemoveAura(100121);
            }
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_soul_consume_AuraScript::HandlePeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_soul_consume_SpellScript();
    }

    AuraScript* GetAuraScript() const override
    {
        return new spell_soul_consume_AuraScript();
    }
};

class spell_soul_consume_heal : public SpellScriptLoader
{
public:
    spell_soul_consume_heal() : SpellScriptLoader("spell_soul_consume_heal") {}

    class spell_soul_consume_heal_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_soul_consume_heal_SpellScript);

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->IsPlayer())
                return;

            Player* player = caster->ToPlayer();
            Aura* soulAura = player->GetAura(100121);

            if (!soulAura)
                return;

            int32 stacks = soulAura->GetStackAmount();

            if (stacks > 1)
            {
                soulAura->ModStackAmount(-1);
                LOG_INFO("custom.spell", "Stack removed. There are %d stacks left.", soulAura->GetStackAmount());
            }
            else
            {
                // Último stack, remove a aura e interrompe a canalização
                player->RemoveAura(100121);
                LOG_INFO("custom.spell", "Soul Consume: Last stack removed. Aura 100121 removed.");

                if (player->HasAura(100123)) // Se ainda estiver canalizando
                {
                    player->InterruptSpell(CURRENT_CHANNELED_SPELL);
                    LOG_INFO("custom.spell", "Soul Consume: Interrupted channel (100123).");
                }
            }
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_soul_consume_heal_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_soul_consume_heal_SpellScript();
    }
};

class spell_soul_barrier : public SpellScriptLoader
{
public:
    spell_soul_barrier() : SpellScriptLoader("spell_soul_barrier") {}

    class spell_soul_barrier_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_soul_barrier_SpellScript);

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->IsPlayer())
                return;

            Player* player = caster->ToPlayer();
            Aura* soulAura = player->GetAura(100121); // Soul Fragments

            // Calcula a absorção base (5% da vida máxima)
            int32 absorbAmount = CalculatePct(player->GetMaxHealth(), 5);
            int32 stackBonus = 0;

            if (soulAura)
            {
                int32 stacks = soulAura->GetStackAmount();
                stackBonus = stacks * CalculatePct(player->GetMaxHealth(), 1); // 1% por stack

                // Remove todas as stacks ao lançar Soul Barrier
                player->RemoveAura(100121);

            }

            int32 totalAbsorb = absorbAmount + stackBonus;

            // Aplica o escudo de absorção
            player->CastCustomSpell(player, 100132, &totalAbsorb, nullptr, nullptr, true); // 100130 = Spell de absorção

        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_soul_barrier_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_soul_barrier_SpellScript();
    }
};

class spell_bulk_extraction : public SpellScriptLoader
{
public:
    spell_bulk_extraction() : SpellScriptLoader("spell_bulk_extraction") {}

    class spell_bulk_extraction_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_bulk_extraction_SpellScript);

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->IsPlayer())
                return;

            Player* player = caster->ToPlayer();

            // Verifica se o jogador já tem a aura ativa
            Aura* soulAura = player->GetAura(100121);
            if (soulAura)
            {
                int32 currentStacks = soulAura->GetStackAmount();
                int32 newStacks = std::min(currentStacks + 5, 30); // Limita o máximo de stacks
                soulAura->SetStackAmount(newStacks);
            }
            else
            {
                // Aplica a aura com 5 stacks caso o jogador ainda não tenha
                player->CastSpell(player, 100121, true);
                player->SetAuraStack(100121, player, 5);
            }
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_bulk_extraction_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_bulk_extraction_SpellScript();
    }
};

class spell_soulmonger : public SpellScriptLoader
{
public:
    spell_soulmonger() : SpellScriptLoader("spell_soulmonger") {}

    class spell_soulmonger_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_soulmonger_SpellScript);

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->IsPlayer())
                return;

            Player* player = caster->ToPlayer();

            // ?? Verifica se o jogador tem Soulmonger aprendido (100138)
            if (!player->HasSpell(100138))
                return;

            // ?? Garantimos que o feitiço processado seja o 100122 (Soul Heal)
            if (GetSpellInfo()->Id != 100122)
                return;

            // ?? Se o jogador estiver com 100% de vida
            if (player->GetHealth() == player->GetMaxHealth())
            {
                int32 maxAbsorb = CalculatePct(player->GetMaxHealth(), 30); // ?? Máximo permitido: 30% da vida
                Aura* soulShield = player->GetAura(100139);

                if (!soulShield)
                {
                    // Aplica o escudo inicial de 10% da vida total
                    int32 absorbAmount = CalculatePct(player->GetMaxHealth(), 10);
                    if (absorbAmount > maxAbsorb)
                        absorbAmount = maxAbsorb;

                    player->CastCustomSpell(player, 100139, &absorbAmount, nullptr, nullptr, true);
                    LOG_INFO("custom.spell", "Soulmonger: Created initial %d absorption shield.", absorbAmount);
                }
                else
                {
                    // Obtém o efeito de absorção já existente
                    AuraEffect* absorbEffect = soulShield->GetEffect(EFFECT_0);
                    if (!absorbEffect)
                        return;

                    // Adiciona +0.5% da vida total ao escudo atual
                    int32 extraAbsorb = CalculatePct(player->GetMaxHealth(), 0.5);
                    int32 newAbsorbTotal = absorbEffect->GetAmount() + extraAbsorb;

                    // ?? Garante que o escudo não ultrapasse 30% da vida total
                    if (newAbsorbTotal > maxAbsorb)
                        newAbsorbTotal = maxAbsorb;

                    absorbEffect->SetAmount(newAbsorbTotal);
                    LOG_INFO("custom.spell", "Soulmonger: Shield increased by +%d absorption. Total:", extraAbsorb, newAbsorbTotal);
                }
            }
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_soulmonger_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_soulmonger_SpellScript();
    }
};

class spell_fel_conversion : public SpellScriptLoader
{
public:
    spell_fel_conversion() : SpellScriptLoader("spell_fel_conversion") {}

    class spell_fel_conversion_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_fel_conversion_SpellScript);

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            Unit* target = GetExplTargetUnit(); // Obtém o alvo do feitiço

            if (!caster || !target || !caster->IsPlayer() || !target->IsPlayer())
                return;

            Player* playerCaster = caster->ToPlayer();
            Player* playerTarget = target->ToPlayer();

            // Obtém a vida atual e a vida máxima dos jogadores
            float casterHealthPct = (float)playerCaster->GetHealth() / playerCaster->GetMaxHealth();
            float targetHealthPct = (float)playerTarget->GetHealth() / playerTarget->GetMaxHealth();

            // Calcula a nova vida com base na porcentagem da vida máxima de cada um
            uint32 newCasterHealth = uint32(targetHealthPct * playerCaster->GetMaxHealth());
            uint32 newTargetHealth = uint32(casterHealthPct * playerTarget->GetMaxHealth());

            // Aplica a nova vida
            playerCaster->SetHealth(newCasterHealth);
            playerTarget->SetHealth(newTargetHealth);

            // Logs para debug
            LOG_INFO("custom.spell", "Fel Conversion: %s now has %u HP (%0.2f%%) | %s now has %u HP (%0.2f%%).",
                playerCaster->GetName().c_str(), newCasterHealth, targetHealthPct * 100,
                playerTarget->GetName().c_str(), newTargetHealth, casterHealthPct * 100);
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_fel_conversion_SpellScript::HandleAfterCast);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_fel_conversion_SpellScript();
    }
};

class spell_eye_of_sargeras : public AuraScript
{
    PrepareAuraScript(spell_eye_of_sargeras);

    static constexpr uint32 STACK_AURA = 100147; // Buff de stacks
    static constexpr uint32 FINAL_SPELL = 100148; // Explosão ao atingir 20 stacks
    static constexpr uint8 MAX_STACKS = 20; // Máximo de stacks antes da ativação
    static constexpr uint32 PERIODIC_TIME = 1000; // Tempo para remover stacks (1s)

    // ? Verifica sempre que o jogador ganha stacks
    void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
    {
        Unit* caster = eventInfo.GetActor();
        if (!caster || !caster->IsPlayer())
            return;

        Player* player = caster->ToPlayer();
        Aura* aura = player->GetAura(STACK_AURA);

        if (!aura)
            return;

        uint8 stacks = aura->GetStackAmount();
        LOG_INFO("custom.spell", "[Eye of Sargeras] %s received a stack. Current total: %u", player->GetName().c_str(), stacks);

        if (stacks >= MAX_STACKS)
        {
            LOG_INFO("custom.spell", "[Eye of Sargeras] %s atingiu 20 stacks. Removing aura and launching 100148!", player->GetName().c_str());

            player->RemoveAura(STACK_AURA);
            player->CastSpell(player, FINAL_SPELL, true);
        }
    }

    // ? Configura o efeito periódico para remover stacks fora de combate
    void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
    {
        Unit* caster = GetCaster();
        if (!caster || !caster->IsPlayer())
            return;

        Aura* aura = caster->GetAura(STACK_AURA);
        if (aura)
        {
            aura->GetEffect(EFFECT_1)->SetPeriodicTimer(PERIODIC_TIME);
            LOG_INFO("custom.spell", "[Eye of Sargeras] Timer of periodic removing stacks configured (%u ms).", PERIODIC_TIME);
        }
    }

    // ? Remove stacks fora de combate a cada 1 segundo
    void OnPeriodic(AuraEffect const* aurEff)
    {
        Unit* caster = GetCaster();
        if (!caster || !caster->IsPlayer())
            return;

        Player* player = caster->ToPlayer();
        Aura* aura = player->GetAura(STACK_AURA);

        if (!aura)
            return;

        if (!player->IsInCombat()) // Se estiver fora de combate, reduz 1 stack por segundo
        {
            uint8 stacks = aura->GetStackAmount();
            if (stacks > 1)
            {
                aura->SetStackAmount(stacks - 1);
                LOG_INFO("custom.spell", "[Eye of Sargeras] %s lost 1 stack. Remaining: %u", player->GetName().c_str(), stacks - 1);
            }
            else
            {
                player->RemoveAura(STACK_AURA);
                LOG_INFO("custom.spell", "[Eye of Sargeras] %s lost last stack and aura was removed.", player->GetName().c_str());
            }
        }
    }

    void Register() override
    {
        OnEffectProc += AuraEffectProcFn(spell_eye_of_sargeras::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        AfterEffectApply += AuraEffectApplyFn(spell_eye_of_sargeras::OnApply, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_eye_of_sargeras::OnPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
    }
};


void AddSC_demonhunter_spell_scripts()
{
    RegisterSpellScript(spell_dh_leech);
    RegisterSpellScript(spell_dh_the_hunt_dot);
    RegisterSpellScript(spell_dh_demonic_presence);
    RegisterSpellScript(spell_sanguine_corruption);
    RegisterSpellScript(spell_fireborn_renewal);
    RegisterSpellScript(spell_eye_of_sargeras);
    //RegisterSpellScript(spell_fel_conversion);
    //RegisterSpellScript(spell_soul_consume);
    //RegisterSpellScript(spell_soul_consume_heal);
    RegisterSpellScript(spell_dh_fel_rush);
    new spell_ragefire_immolation();
    new spell_dh_fel_rush();
    new spell_spirit_bomb();
    new spell_fireball_embraceness();
    new spell_soul_consume();
    new spell_soul_consume_heal();
    new spell_soul_barrier();
    new spell_bulk_extraction();
    new spell_soulmonger();
    new spell_fel_conversion();
    // new spell_eye_of_sargeras();
}
