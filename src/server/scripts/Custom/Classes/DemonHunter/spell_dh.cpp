#include "SpellMgr.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Unit.h"
#include "Player.h"
#include "Log.h"
#include "MoveSplineInit.h"



class spell_dh_fel_rush : public SpellScriptLoader
{
public:
    spell_dh_fel_rush() : SpellScriptLoader("spell_dh_fel_rush") {}

public:
    class spell_dh_fel_rush_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dh_fel_rush_SpellScript);

        void HandleAfterCast()
        {
            Player* caster = GetCaster()->ToPlayer();
            if (!caster || !caster->IsAlive())
                return;

            float savedPositionX;
            float savedPositionY;
            float savedPositionZ;
            float savedPositionO;

            if (caster->HasSpell(100076)) // SPELL_DASH_OF_CHAOS (talent)
            {
                savedPositionX = caster->GetPositionX();
                savedPositionY = caster->GetPositionY();
                savedPositionZ = caster->GetPositionZ();
                savedPositionO = caster->GetOrientation();
                LOG_INFO("custom.spell", "Fel Rush: Saved initial position for teleport.");
            }

            float distance = 22.3f; // Far distance
            float speed = 60.0f;    // Movement speed
            float orientation = caster->GetOrientation();

            float destX = caster->GetPositionX() + std::cos(orientation) * distance;
            float destY = caster->GetPositionY() + std::sin(orientation) * distance;
            float destZ = caster->GetPositionZ();

            float groundZ = caster->GetMap()->GetHeight(destX, destY, destZ, true);

            if (groundZ == INVALID_HEIGHT)
            {
                groundZ = caster->GetPositionZ();
            }

            float adjustedDestZ = destZ;

            if (groundZ != INVALID_HEIGHT && groundZ > caster->GetPositionZ())
            {
                adjustedDestZ = groundZ;
            }

            caster->GetMotionMaster()->MoveFelRush(destX, destY, adjustedDestZ, speed, EVENT_CHARGE, true);
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_dh_fel_rush_SpellScript::HandleAfterCast);
        }
    };


    SpellScript* GetSpellScript() const override
    {
        return new spell_dh_fel_rush_SpellScript();
    }

    /* // Uncomment if you need an AuraScript for this spell
    * dindt come with one from browler
    AuraScript* GetAuraScript() const override
    {
        return new spell_dh_fel_rush_AuraScript();
    }
    */
};

void AddSC_demonhunter_spell_scripts()
{
    new spell_dh_fel_rush();
}
