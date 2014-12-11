#include "instance_bloodmaul.h"

namespace MS
{
    namespace Instances
    {
        namespace Bloodmaul
        {
            // Entry: 74787
            class boss_SlaveWatcherCrushto : public CreatureScript
            {
            public:
                boss_SlaveWatcherCrushto()
                    : CreatureScript("boss_SlaveWatcherCrushto")
                {
                }

                enum class Spells : uint32
                {
                    EarthCrush = 153735,  // Summon.
                    EarthCrush2 = 153732, // Useless, no visual.
                    EarthCrush3 = 153679, // Visual foot kick in the rock.
                    EarthCrush4 = 167246,
                    FerociousYell = 150759,
                    RaiseTheMiners = 150801,
                    WildSlam = 150753,
                    CrushingLeap = 150746,  // Jump
                    CrushingLeap2 = 150745, // Dummy
                    CrushingLeap3 = 150751, // Effect.
                    SlayerOfTheSlayer = 163577, // Apply when boss dies.
                    Fear = 38154,
                    WeakenedWill = 150811,
                };

                enum class Texts : int32
                {
                    COMBAT_START,
                    JUST_DIED,
                    KILL_PLAYER_1,
                    KILL_PLAYER_2,
                    VICTORY
                };

                enum class Events : uint32
                {
                    EarthCrush = 1,
                    FerociousYell = 2,
                    RaiseTheMiners = 3,
                    WildSlam = 4,
                    CrushingLeap = 5
                };

                CreatureAI* GetAI(Creature* creature) const
                {
                    return new boss_AI(creature);
                }

                struct boss_AI : public BossAI
                {
                    boss_AI(Creature* creature) : BossAI(creature, uint32(BossIds::SlaveWatcherCrushto)),
                    m_LastEarthCrushStalkerPosition()
                    {
                        if (instance)
                            instance->SetBossState(uint32(BossIds::SlaveWatcherCrushto), EncounterState::TO_BE_DECIDED);
                    }

                    void Reset()
                    {
                        _Reset();
                    }

                    void JustReachedHome()
                    {
                        _JustReachedHome();

                        if (instance)
                        {
                            instance->SetBossState(uint32(BossIds::SlaveWatcherCrushto), FAIL);
                            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                        }
                    }

                    void JustDied(Unit* /*killer*/)
                    {
                        _JustDied();

                        if (instance)
                            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                    }

                    void KilledUnit(Unit* /*victim*/)
                    {
                    }

                    void EnterCombat(Unit* who)
                    {
                        _EnterCombat();

                        events.ScheduleEvent(uint32(Events::EarthCrush), 5000);
                        events.ScheduleEvent(uint32(Events::FerociousYell), 10000);
                        events.ScheduleEvent(uint32(Events::RaiseTheMiners), 16000);
                        events.ScheduleEvent(uint32(Events::CrushingLeap), 23000);
                        events.ScheduleEvent(uint32(Events::WildSlam), 27000);

                        if (instance)
                            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
                    }

                    void MovementInform(uint32 p_Type, uint32 p_Id)
                    {
                        switch (p_Type)
                        {
                        case EFFECT_MOTION_TYPE:
                            if (Player* l_Plr = sObjectAccessor->FindPlayer(m_TargetGUID))
                            {
                                me->CastSpell(l_Plr, uint32(Spells::CrushingLeap3));
                                m_TargetGUID = 0;
                                if (instance)
                                    instance->SetData64(uint32(Data::RaiseTheMinersChangeTarget), l_Plr->GetGUID());
                            }
                            break;
                        }
                    }

                    void SpellHitTarget(Unit* p_Target, SpellInfo const* p_SpellInfo)
                    {
                        // Handling Earth Crush damages.
                        if (p_SpellInfo->Id == uint32(Spells::EarthCrush3) && p_Target && p_Target->GetEntry() == uint32(MobEntries::EarthCrushStalker))
                        {
                            ScriptUtils::ApplyOnEveryPlayer(me, [&](Unit* p_Me, Player* p_Plr) {
                                // We check if players are in not behind the boss and if they are in the line.
                                if (p_Me->isInFront(p_Plr) && DistanceFromLine(*p_Me, m_LastEarthCrushStalkerPosition, *p_Plr) < 2.0f && p_Me->GetExactDist2d(p_Plr) < 40.0f)
                                    p_Me->CastSpell(p_Plr, uint32(Spells::EarthCrush3), true);
                            });
                        }

                        // Handling fear after Ferocious Yell.
                        if (p_SpellInfo->Id == uint32(Spells::FerociousYell) && p_Target && p_Target->GetAuraCount(uint32(Spells::WeakenedWill)) == 3)
                        {
                            p_Target->RemoveAura(uint32(Spells::WeakenedWill));
                            me->AddAura(uint32(Spells::Fear), p_Target);
                        }
                    }

                    void UpdateAI(const uint32 diff)
                    {
                        if (!UpdateVictim())
                            return;

                        events.Update(diff);

                        if (me->HasUnitState(UNIT_STATE_CASTING))
                            return;

                        switch (events.ExecuteEvent())
                        {
                        case uint32(Events::EarthCrush):
                            if (Player* l_Plr = ScriptUtils::SelectRandomPlayerIncludedTank(me, 40.0f))
                            {
                                Position l_Dir = *l_Plr - *me;
                                normalizeXY(l_Dir);

                                Position l_SummonPos = *me + (l_Dir * 2.0f);
                                m_LastEarthCrushStalkerPosition = l_SummonPos;

                                // We summon the rock.
                                TempSummon* l_Summon = me->SummonCreature(uint32(MobEntries::EarthCrushStalker), l_SummonPos, TempSummonType::TEMPSUMMON_TIMED_DESPAWN, 4000);
                                l_Summon->SetFacingToObject(l_Plr);
                                l_Summon->CastSpell(l_Summon, uint32(Spells::EarthCrush4), true);

                                // We launch foot kick.
                                me->SetFacingToObject(l_Plr);
                                me->CastSpell(l_Summon, uint32(Spells::EarthCrush3), false);
                            }
                            events.ScheduleEvent(uint32(Events::EarthCrush), urand(20000, 25000));
                            break;
                        case uint32(Events::FerociousYell):
                            me->CastSpell(me, uint32(Spells::FerociousYell));
                            events.ScheduleEvent(uint32(Events::FerociousYell), urand(10000, 12000));
                            break;
                        case uint32(Events::RaiseTheMiners):
                            me->CastSpell(me, uint32(Spells::RaiseTheMiners));
                            events.ScheduleEvent(uint32(Events::RaiseTheMiners), urand(25000, 29000));
                            break;
                        case uint32(Events::WildSlam):
                            me->CastSpell(me->getVictim(), uint32(Spells::WildSlam));
                            events.ScheduleEvent(uint32(Events::WildSlam), urand(20000, 27000));
                            break;
                        case uint32(Events::CrushingLeap):
                            if (Player* l_Plr = ScriptUtils::SelectFarEnoughPlayerIncludedTank(me, 8.0f))
                            {
                                m_TargetGUID = l_Plr->GetGUID();
                                me->CastSpell(l_Plr, uint32(Spells::CrushingLeap));
                            }
                            events.ScheduleEvent(uint32(Events::CrushingLeap), urand(20000, 23000));
                            break;
                        default:
                            break;
                        }

                        DoMeleeAttackIfReady();
                    }

                    Position m_LastEarthCrushStalkerPosition;
                    uint64 m_TargetGUID;
                };
            };
        }
    }
}

void AddSC_boss_SlaveWatcherCrushto()
{
    new MS::Instances::Bloodmaul::boss_SlaveWatcherCrushto();
}