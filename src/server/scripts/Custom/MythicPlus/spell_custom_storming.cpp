#include "ScriptMgr.h"
#include "Creature.h"
#include "SpellMgr.h"
#include "ObjectAccessor.h"
#include "MotionMaster.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "TypeContainerVisitor.h"
#include "Position.h"
#include "Spell.h"
#include "Unit.h"

enum StormingAffix
{
    SPELL_STORMING_AURA = 3000002, // aura on mobs
    SPELL_STORMING_DAMAGE = 3001002, // damage spell used by whirlwind
    NPC_STORMING_WHIRLWIND = 5001000 // the summoned creature
};

// --------------------------------------------
// Whirlwind AI - expanding spiral movement + AoE damage
// --------------------------------------------

class npc_storming_whirlwind : public CreatureScript
{
public:
    npc_storming_whirlwind() : CreatureScript("npc_storming_whirlwind") {}

    struct npc_storming_whirlwindAI : public ScriptedAI
    {
        npc_storming_whirlwindAI(Creature* creature) : ScriptedAI(creature) {}

        uint32 damageTimer = 1000;
        uint32 moveTimer = 2000;
        float angle = 0.0f;
        float radius = 3.0f;
        Position origin;

        void Reset() override
        {
            damageTimer = 1000;
            moveTimer = 2000;
            angle = frand(0.0f, 2 * M_PI);
            radius = 3.0f;
            origin = me->GetHomePosition(); // spawn origin for spiral
        }

        void UpdateAI(uint32 diff) override
        {
            // Spiral-like circular motion
            if (moveTimer <= diff)
            {
                angle += M_PI / 12.0f; // ~15 degrees per tick, smooth

                float orbitRadius = 3.0f; // fixed distance from origin
                float x = origin.GetPositionX() + std::cos(angle) * orbitRadius;
                float y = origin.GetPositionY() + std::sin(angle) * orbitRadius;
                float z = origin.GetPositionZ();

                me->GetMotionMaster()->MovePoint(0, x, y, z);

                moveTimer = 100; // move every 250ms for smoother motion
            }
            else
            {
                moveTimer -= diff;
            }

        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_storming_whirlwindAI(creature);
    }
};

// --------------------------------------------
// UnitScript that has the storming aura and summons whirlwinds
// --------------------------------------------
class storming_affix_unit_script : public UnitScript
{
public:
    storming_affix_unit_script() : UnitScript("storming_affix_unit_script") {}

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || !unit->IsAlive())
            return;

        if (aura->GetId() != 3000002)
            return;

        LOG_DEBUG("module.affix.storming", "Storming aura applied to {}", unit->GetName());

        unit->m_Events.AddEvent(new StormingSpawnEvent(unit), unit->m_Events.CalculateTime(10000));
    }

    // Optional
    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode /*mode*/) override
    {
        if (!unit || !aurApp)
            return;

        Aura const* aura = aurApp->GetBase();
        if (!aura || aura->GetId() != 3000002)
            return;

        LOG_DEBUG("module.affix.storming", "Storming aura removed from {}", unit->GetName());
    }

private:
    class StormingSpawnEvent : public BasicEvent
    {
    public:
        explicit StormingSpawnEvent(Unit* unit) : _unit(unit) {}

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (!_unit || !_unit->IsAlive() || !_unit->IsInCombat() || !_unit->HasAura(3000002))
                return false;

            LOG_DEBUG("module.affix.storming", "{} GetRandomNearPosition", _unit->GetName());
            Position pos = _unit->GetRandomNearPosition(5.0f);

            if (Creature* whirlwind = _unit->SummonCreature(5001000, pos, TEMPSUMMON_TIMED_DESPAWN, 10000))
            {
                whirlwind->CastSpell(whirlwind, 3001002, true);
                LOG_DEBUG("module.affix.storming", "{} spawned whirlwind", _unit->GetName());
            }

            _unit->m_Events.AddEvent(new StormingSpawnEvent(_unit), _unit->m_Events.CalculateTime(10000));
            return true;
        }

    private:
        Unit* _unit;
    };
};

// --------------------------------------------
// Script loader
// --------------------------------------------

void AddSC_spell_custom_storming()
{
    new npc_storming_whirlwind();       // Whirlwind behavior
    new storming_affix_unit_script();        // All mobs with aura 3000002
}
