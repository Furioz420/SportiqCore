// spell_belligerent_missile_launcher.cpp
#include <cstring> // for std::memcpy
#include <type_traits>

#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "Creature.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "MotionMaster.h"
#include "Position.h"
#include "Log.h"

enum MissileMovement
{
    POINT_OUT = 1,
    POINT_BACK = 2
};

enum MissileAIData
{
    MISSILE_AI_RETURN_X = 100,
    MISSILE_AI_RETURN_Y = 101,
    MISSILE_AI_RETURN_Z = 102,
    MISSILE_AI_ACTIVATE = 1,
    MISSILE_AI_TICKRATE = 2,
    MISSILE_AI_DAMAGE = 3,
};

template<typename To, typename From>
typename std::enable_if<sizeof(To) == sizeof(From), To>::type
BitCast(From const& src)
{
    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}


class spell_belligerent_missile_launcher : public SpellScriptLoader
{
public:
    spell_belligerent_missile_launcher() : SpellScriptLoader("spell_belligerent_missile_launcher") {}

    class aura_belligerent_missile_launcher_AuraScript : public AuraScript
    {
        PrepareAuraScript(aura_belligerent_missile_launcher_AuraScript);

        std::vector<std::pair<Creature*, Position>> missiles;

        void OnApply(AuraEffect const*, AuraEffectHandleModes)
        {
            LOG_DEBUG("module.affix.prideful", "[Missile Aura] OnApply triggered for '{}'", GetTarget()->GetName());

            Player* target = GetTarget()->ToPlayer();
            if (!target)
                return;

            const Position& base = target->GetPosition();
            float z = base.GetPositionZ();
            float o = base.GetOrientation();

            struct Dir { float dx, dy, dz; };
            std::vector<Dir> dirs = {
                {  0.0f,  3.0f,  1.0f },
                {  3.0f,  0.0f,  1.0f },
                {  0.0f, -3.0f,  1.0f },
                { -3.0f,  0.0f,  1.0f },
            };

            for (auto const& dir : dirs)
            {
                Position spawnPos(base.GetPositionX() + dir.dx, base.GetPositionY() + dir.dy, z + dir.dz, o);
                Position targetPos(spawnPos.GetPositionX() + dir.dx * 3.00f, spawnPos.GetPositionY() + dir.dy * 3.00f, z);

                float angle = atan2f(targetPos.GetPositionY() - spawnPos.GetPositionY(),
                    targetPos.GetPositionX() - spawnPos.GetPositionX());

                Position spawnPosAngle(base.GetPositionX() + dir.dx, base.GetPositionY() + dir.dy, z + dir.dz, angle);

                if (Creature* missile = target->SummonCreature(5001008, spawnPosAngle, TEMPSUMMON_MANUAL_DESPAWN, 0))
                {
                    missile->AddUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY);
                    missile->SetHover(true);
                    LOG_DEBUG("module.affix.prideful", "[Missile] Spawned at [{:.2f}, {:.2f}] for '{}'", spawnPosAngle.GetPositionX(), spawnPosAngle.GetPositionY(), target->GetName());

                    missiles.emplace_back(missile, targetPos);
                }
            }
        }

        void OnRemove(AuraEffect const*, AuraEffectHandleModes)
        {
            for (auto& [missile, targetPos] : missiles)
            {
                if (!missile || !missile->IsAlive())
                    continue;

                const Position& missilebase = missile->GetPosition();

                missile->GetMotionMaster()->Clear();
                missile->GetMotionMaster()->MovePoint(POINT_OUT, targetPos);

                missile->AI()->SetData(MISSILE_AI_ACTIVATE, 1);
                missile->AI()->SetData(MISSILE_AI_TICKRATE, 500);
                missile->AI()->SetData(MISSILE_AI_DAMAGE, 3001029);
                missile->AI()->SetData(MISSILE_AI_RETURN_X, BitCast<uint32>(missilebase.GetPositionX()));
                missile->AI()->SetData(MISSILE_AI_RETURN_Y, BitCast<uint32>(missilebase.GetPositionY()));
                missile->AI()->SetData(MISSILE_AI_RETURN_Z, BitCast<uint32>(missilebase.GetPositionZ()));

                LOG_DEBUG("module.affix.prideful", "[Missile] Triggered move to [{:.2f}, {:.2f}]", targetPos.GetPositionX(), targetPos.GetPositionY());
                LOG_DEBUG("module.affix.prideful", "[Missile] Should return to [{:.2f}, {:.2f}]", missilebase.GetPositionX(), missilebase.GetPositionY());
            }
        }

        void Register() override
        {
            OnEffectApply += AuraEffectApplyFn(aura_belligerent_missile_launcher_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectRemove += AuraEffectRemoveFn(aura_belligerent_missile_launcher_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new aura_belligerent_missile_launcher_AuraScript();
    }
};

void AddSC_spell_belligerent_missile_launcher()
{
    new spell_belligerent_missile_launcher();
}
