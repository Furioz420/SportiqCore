// npc_belligerent_missile.cpp

#include "ScriptMgr.h"
#include "Creature.h"
#include "Player.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
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


class npc_belligerent_missile : public CreatureScript
{
public:
    npc_belligerent_missile() : CreatureScript("npc_belligerent_missile") {}

    struct npc_belligerent_missileAI : public ScriptedAI
    {
        npc_belligerent_missileAI(Creature* creature) : ScriptedAI(creature) {}

        Position returnPos;
        const Position& spawnPos = me->GetHomePosition();
        bool returning = false;
        bool finished = false;
        bool active = false;
        uint32 checkTimer = 500;
        uint32 damageSpell = 3001029;
        uint32 currentMoveId = 0;

        void SetReturnPosition(Position const& pos)
        {
            float z = me->GetMap()->GetHeight(spawnPos.GetPositionX(), spawnPos.GetPositionY(), spawnPos.GetPositionZ());
            returnPos.Relocate(spawnPos.GetPositionX(), spawnPos.GetPositionY(), z);

            LOG_DEBUG("module.affix.prideful", "[Missile] '{}' return position set to [{:.2f}, {:.2f}]", me->GetGUID().ToString().c_str(), pos.GetPositionX(), pos.GetPositionY());
        }

        void SetData(uint32 id, uint32 value) override
        {
            if (id == MISSILE_AI_ACTIVATE)
            {
                active = true;
                LOG_DEBUG("module.affix.prideful", "[Missile] '{}' activated", me->GetGUID().ToString().c_str());
            }
            else if (id == MISSILE_AI_TICKRATE)
                checkTimer = value;
            else if (id == MISSILE_AI_DAMAGE)
                damageSpell = value;
            else if (id == MISSILE_AI_RETURN_X)
                returnPos.m_positionX = BitCast<float>(value);
            else if (id == MISSILE_AI_RETURN_Y)
                returnPos.m_positionY = BitCast<float>(value);
            else if (id == MISSILE_AI_RETURN_Z)
                returnPos.m_positionZ = BitCast<float>(value);
            else
                LOG_DEBUG("module.affix.prideful", "[Missile] '{}' unknown setdata requested", value);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!active || !me->IsInWorld())
                return;

            if (checkTimer <= diff)
            {
                std::list<Unit*> units;
                Acore::AnyUnitInObjectRangeCheck check(me, 2.0f);
                Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(me, units, check);
                Cell::VisitAllObjects(me, searcher, 2.0f);

                // Now filter out the players:
                for (Unit* unit : units)
                {
                    if (Player* player = unit->ToPlayer())
                    {
                        if (player->IsAlive())
                        {
                            me->AddAura(damageSpell, player);
                            LOG_DEBUG("module.affix.prideful", "[Missile] '{}' hit '{}'", me->GetGUID().ToString().c_str(), player->GetName());
                        }
                    }
                }

                checkTimer = 500;
            }
            else
            {
                checkTimer -= diff;
            }

            if (returning)
            {
                MovementGeneratorType moveType = me->GetMotionMaster()->GetCurrentMovementGeneratorType();
                if (moveType != POINT_MOTION_TYPE || me->movespline->Finalized() || !finished)
                {
                    float dist = me->GetDistance(returnPos);
                    if (dist > 0.5f)
                    {
                        LOG_DEBUG("module.affix.prideful", "[Missile returning] '{}' stuck — reissuing return MovePoint", me->GetGUID().ToString().c_str());

                        me->Relocate(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                        me->GetMotionMaster()->Clear();
                        me->StopMoving();

                        currentMoveId = POINT_BACK + me->GetGUID().GetCounter();
                        me->GetMotionMaster()->MovePoint(currentMoveId, returnPos);
                        LOG_DEBUG("module.affix.prideful", "[Missile returning] MovePoint issued with ID {}", currentMoveId);
                        LOG_DEBUG("module.affix.prideful", "[Missile returning] returnPos: X: {:.2f}, Y: {:.2f}, Z: {:.2f}",
                            returnPos.GetPositionX(), returnPos.GetPositionY(), returnPos.GetPositionZ());
                    }
                }

                if (me->GetPositionX() == returnPos.GetPositionX() &&
                    me->GetPositionY() == returnPos.GetPositionY())
                {
                    finished = true;
                    LOG_DEBUG("module.affix.prideful", "[Missile returning] '{}' reached return position", me->GetGUID().ToString().c_str());
                    me->DespawnOrUnsummon();
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (returnPos.GetPositionX() == 0.0f &&
                returnPos.GetPositionY() == 0.0f)
            {
                LOG_DEBUG("module.affix.prideful", "[Missile] '{}' returnPos is nil", me->GetGUID().ToString().c_str());
                Position safeReturn = me->GetHomePosition();
                float fixedZ = me->GetMap()->GetHeight(safeReturn.GetPositionX(), safeReturn.GetPositionY(), safeReturn.GetPositionZ());
                safeReturn.m_positionZ = fixedZ;
                returnPos = safeReturn;
            }
            if (id == POINT_OUT && !returning)
            {
                returning = true;

                if (returnPos.GetPositionX() == 0.0f && returnPos.GetPositionY() == 0.0f)
                {
                    Position safeReturn = me->GetHomePosition();
                    float fixedZ = me->GetMap()->GetHeight(safeReturn.GetPositionX(), safeReturn.GetPositionY(), safeReturn.GetPositionZ());
                    safeReturn.m_positionZ = fixedZ;
                    returnPos = safeReturn;
                    LOG_DEBUG("module.affix.prideful", "[Missile MovementInform] '{}' returnPos fallback", me->GetGUID().ToString().c_str());
                }

                LOG_DEBUG("module.affix.prideful", "[Missile MovementInform] returnPos: X: {:.2f}, Y: {:.2f}, Z: {:.2f}",
                    returnPos.GetPositionX(), returnPos.GetPositionY(), returnPos.GetPositionZ());

                float dist = me->GetDistance(returnPos);
                MovementGeneratorType moveType = me->GetMotionMaster()->GetCurrentMovementGeneratorType();

                LOG_DEBUG("module.affix.prideful", "[Missile MovementInform] Distance to returnPos: {:.2f}, movement type: {}", dist, moveType);

                me->Relocate(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
                me->GetMotionMaster()->Clear();
                me->StopMoving();

                currentMoveId = POINT_BACK + me->GetGUID().GetCounter();
                me->GetMotionMaster()->MovePoint(currentMoveId, returnPos);

                LOG_DEBUG("module.affix.prideful", "[Missile MovementInform] '{}' reached target — returning to spawn", me->GetGUID().ToString().c_str());
            }
            else if (id == currentMoveId && finished)
            {
                float distance = me->GetDistance(returnPos);
                if (distance > 0.5f)
                {
                    LOG_WARN("module.affix.prideful", "[Missile MovementInform] '{}' triggered POINT_BACK too early (distance {:.2f})", me->GetGUID().ToString().c_str(), distance);
                    return;
                }
            }
        }

        void JustRespawned() override
        {
            returning = false;
            active = false;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_belligerent_missileAI(creature);
    }
};

void AddSC_npc_belligerent_missile()
{
    new npc_belligerent_missile();
}
