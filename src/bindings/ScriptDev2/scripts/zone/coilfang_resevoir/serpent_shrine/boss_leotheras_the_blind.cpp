/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Boss_Leotheras_The_Blind
SD%Complete: 90
SDComment: DoYell Disable, i need help why the delete doplaysound, need more cleanup
SDCategory: Coilfang Resevoir, Serpent Shrine Cavern
EndScriptData */

#include "precompiled.h"
#include "def_serpent_shrine.h"
#include "Player.h"

enum
{
    // --- Spells used by Leotheras The Blind
    SPELL_WHIRLWIND         = 37640,
    SPELL_CHAOS_BLAST       = 37674,
    SPELL_BERSERK			= 26662,
    SPELL_INSIDIOUS_WHISPER = 37676,

    // --- Spells used in banish phase ---
    BANISH_BEAM             = 38909,
    AURA_BANISH			    = 37833,

    // --- Spells used by Greyheart Spellbinders
    SPELL_EARTHSHOCK		= 39076,
    SPELL_MINDBLAST			= 37531,

    // --- Spells used by Inner Demons and creature ID
    INNER_DEMON_ID          = 21857,
    AURA_DEMONIC_ALIGNMENT  = 37713,
    SPELL_SHADOWBOLT        = 39309,
    SPELL_SOUL_LINK         = 38007,
    SPELL_CONSUMING_MADNESS = 29546,//37749, //not supported by core yet

    //Talk
    SAY_AGGRO               = -1548009,
    SAY_SWITCH_TO_DEMON     = -1548010,
    SAY_INNER_DEMONS        = -1548011,
    SAY_DEMON_SLAY1         = -1548012,
    SAY_DEMON_SLAY2         = -1548013,
    SAY_DEMON_SLAY3         = -1548014,
    SAY_NIGHTELF_SLAY1      = -1548015,
    SAY_NIGHTELF_SLAY2      = -1548016,
    SAY_NIGHTELF_SLAY3      = -1548017,
    SAY_FINAL_FORM          = -1548018,
    SAY_FREE                = -1548019,
    SAY_DEATH               = -1548020,

    //Sound
    SOUND_AGGRO             = 11312,
    SOUND_SWITCH_TO_DEMON   = 11304,
    SOUND_INNER_DEMONS      = 11305,
    SOUND_DEMON_SLAY1       = 11306,
    SOUND_DEMON_SLAY2       = 11307,
    SOUND_DEMON_SLAY3       = 11308,
    SOUND_NIGHTELF_SLAY1    = 11314,
    SOUND_NIGHTELF_SLAY2    = 11315,
    SOUND_NIGHTELF_SLAY3    = 11316,
    SOUND_FINAL_FORM        = 11313,
    SOUND_FREE              = 11309,
    SOUND_DEATH             = 11317,

    //Sumon
    MODEL_DEMON             = 20125,
    MODEL_NIGHTELF          = 20514,
    DEMON_FORM              = 21875,
    MOB_SPELLBINDER      = 21806
 };

class MANGOS_DLL_DECL InsidiousAura : public Aura
{
public:
	InsidiousAura (SpellEntry *spell, uint32 eff, int32 *bp, Unit *target, Unit *caster) : Aura(spell, eff, bp, target, caster, NULL)
	{}
};

struct MANGOS_DLL_DECL mob_inner_demonAI : public ScriptedAI
{
    mob_inner_demonAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
		victimGUID = 0;
        Reset();
    }

    uint32 ShadowBolt_Timer;
	
	uint32 Link_Timer;
	uint64 victimGUID;

    void Reset()
    {
        ShadowBolt_Timer = 10000;		
        Link_Timer = 1000;
    }
	void JustDied(Unit *victim)
    {
        Unit* pUnit = Unit::GetUnit((*m_creature),victimGUID);
        if (pUnit && pUnit->HasAura(SPELL_INSIDIOUS_WHISPER,0))
        pUnit->RemoveAurasDueToSpell(SPELL_INSIDIOUS_WHISPER);
    }

    void DamageTaken(Unit *done_by, uint32 &damage)
    {
        if(done_by->GetGUID() != victimGUID && done_by->GetGUID() != m_creature->GetGUID())
        {
        damage = 0;
        m_creature->getThreatManager().modifyThreatPercent(done_by, -100);
        }
    }

     void Aggro(Unit *who)
    {
        if (!victimGUID) return;
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;

		if (m_creature->getVictim()->GetGUID() != victimGUID)
		{
			Unit* owner = Unit::GetUnit((*m_creature),victimGUID);
				if (owner)
					AttackStart(owner);
		}
		if(Link_Timer < diff)
		{
			DoCast(m_creature->getVictim(), SPELL_SOUL_LINK, true);
			Link_Timer = 1000;
		}else Link_Timer -= diff;


		if(!m_creature->HasAura(AURA_DEMONIC_ALIGNMENT, 0))
			DoCast(m_creature, AURA_DEMONIC_ALIGNMENT,true);

		if(ShadowBolt_Timer < diff)
		{
			DoCast(m_creature->getVictim(), SPELL_SHADOWBOLT, false);
			ShadowBolt_Timer = 10000;
		}else ShadowBolt_Timer -= diff; 

        DoMeleeAttackIfReady();
    }
};

//Original Leotheras the Blind AI
struct MANGOS_DLL_DECL boss_leotheras_the_blindAI : public ScriptedAI
{
    boss_leotheras_the_blindAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (pCreature->GetInstanceData()) ? ((ScriptedInstance*)pCreature->GetInstanceData()) : NULL;
        Demon = 0;
        m_creature->GetPosition(x,y,z);
        channelers = 0;
        for(uint8 i = 0; i < 3; i++)//clear guids
            SpellBinderGUID[i] = 0;

		
        //damage & attack speed to dual wield	2.6s mainhand and 1.5s offhand
        const CreatureInfo *cinfo = m_creature->GetCreatureInfo();
        m_creature->SetAttackTime(BASE_ATTACK, 2600);
        
        m_creature->SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, cinfo->mindmg*0.5);
        m_creature->SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, cinfo->maxdmg*0.5);
		
        m_creature->SetFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE, cinfo->mindmg*0.5);
        m_creature->SetFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE, cinfo->maxdmg*0.5);
        m_creature->SetAttackTime(OFF_ATTACK, 1500);

        // needs to reset offhand attack timer
        m_creature->setAttackTimer(BASE_ATTACK, 0);
        m_creature->setAttackTimer(OFF_ATTACK, 0);

        Reset();
    }

    ScriptedInstance *m_pInstance;

    uint32 m_uiWhirlwind_Timer;
    uint32 m_uiChaosBlast_Timer;
    uint32 m_uiSwitchToDemon_Timer;
    uint32 m_uiSwitchToHuman_Timer;
    uint32 m_uiBerserk_Timer;
    uint32 m_uiInnerDemons_Timer;
	
    bool DealDamage;
    bool NeedThreatReset;
    bool DemonForm;
    bool IsFinalForm;
    bool EnrageUsed;

    uint8  channelers;
    uint64 InnderDemon[5];
    uint32 InnderDemon_Count;
    uint64 SpellBinderGUID[3];
    float x,y,z;
    uint32 BanishTimer;
    uint64 Demon;

    void Reset()
    {
        m_uiWhirlwind_Timer = 15000;
        m_uiChaosBlast_Timer = 1000;
        m_uiSwitchToDemon_Timer = 45000;
        m_uiSwitchToHuman_Timer = 60000;
        m_uiBerserk_Timer = 600000;
        m_uiInnerDemons_Timer = 30000;
        DealDamage = true;

        DemonForm = false;
        IsFinalForm = false;
        NeedThreatReset = false;
        EnrageUsed = false;
        InnderDemon_Count = 0;

        m_creature->SetSpeed( MOVE_RUN, 1.3f, true);

        m_creature->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
        m_creature->ApplySpellImmune(1, IMMUNITY_EFFECT,SPELL_EFFECT_ATTACK_ME, true);

        m_creature->SetUInt32Value(UNIT_FIELD_DISPLAYID, MODEL_NIGHTELF);
        m_creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY  , 0);
        m_creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY+1, 0);


        if (m_pInstance)
           {
               m_pInstance->SetData(DATA_LEOTHERASTHEBLINDEVENT, NOT_STARTED); 
/*               //Respawn of the 3 Spellbinder
               uint64 GreyheartSpellbinder[3];
               GreyheartSpellbinder[0] = m_pInstance->GetData64( DATA_SPELLBINDER_1 );
               GreyheartSpellbinder[1] = m_pInstance->GetData64( DATA_SPELLBINDER_2 );
               GreyheartSpellbinder[2] = m_pInstance->GetData64( DATA_SPELLBINDER_3 );
               for( int i=0; i<3; i++ )
                  {
                  // Greyheart Spellbinder is dead, we must respawn it
                  if(GreyheartSpellbinder[i])
                      {
                      Creature* pSpellbinder = ((Creature*)Unit::GetUnit((*m_creature), GreyheartSpellbinder[i]));
                      if(pSpellbinder)
                         {
                           if(!pSpellbinder->isAlive())
                              pSpellbinder->Respawn();
                           pSpellbinder->AI()->EnterEvadeMode();
                           pSpellbinder->GetMotionMaster()->MoveTargetedHome();
                         }
                      }
                  }*/
           }
        m_creature->SetCorpseDelay(1000*60*60);
        CheckChannelers();
        BanishTimer = 1000;
    }

    void CheckChannelers(bool DoEvade = true)
    {
        for(uint8 i = 0; i < 3; i++)
        {
            Creature *add = (Creature*)Unit::GetUnit(*m_creature,SpellBinderGUID[i]);
            if (add && add->isAlive())
            {
                add->setDeathState(DEAD);
                add->RemoveCorpse();
            }else{
                if(add && add->isDead())
                    add->RemoveCorpse();
            }
            float nx = x;
            float ny = y;
            float o = 2.4f;
            if (i == 0) {nx += 20;o=3.0f;}
            if (i == 1) ny -= 20;
            if (i == 2) {nx += 18;ny -= 18;o=2.0f;}
            Creature* binder = m_creature->SummonCreature(MOB_SPELLBINDER,nx,ny,z,o,TEMPSUMMON_DEAD_DESPAWN,0);
            if (binder)
                SpellBinderGUID[i] = binder->GetGUID();

        }
    }
	void MoveInLineOfSight(Unit *who)
	{
		if(m_creature->HasAura(AURA_BANISH, 0))
			return;

		if( !m_creature->getVictim() && who->isTargetableForAttack() && ( m_creature->IsHostileTo( who )) && who->isInAccessablePlaceFor(m_creature) )
		{
			if (m_creature->GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE)
				return;

			float attackRadius = m_creature->GetAttackDistance(who);
			if(m_creature->IsWithinDistInMap(who, attackRadius))
			{
				// Check first that object is in an angle in front of this one before LoS check
				if( m_creature->HasInArc(M_PI/2.0f, who) && m_creature->IsWithinLOSInMap(who) )
				{
                    AttackStart(who);
					who->RemoveSpellsCausingAura(SPELL_AURA_MOD_STEALTH);
					if (!m_creature->getVictim())
					{
						Aggro(who);
					}
				}
			}
		}
	}

    void StartEvent()
    {
        DoScriptText(SAY_AGGRO, m_creature);

        if (m_pInstance)
            m_pInstance->SetData(DATA_LEOTHERASTHEBLINDEVENT, 1);
    }

void CheckBanish()
 {
        uint8 channelers = 0;
        for(uint8 i = 0; i < 3; i++)
        {
            Unit *add = Unit::GetUnit(*m_creature,SpellBinderGUID[i]);
            if (add && add->isAlive())
                channelers++;
        }
		// channelers == 0 remove banish aura
		if(channelers == 0 && m_creature->HasAura(AURA_BANISH, 0))
		{	
			// removing banish aura
			m_creature->RemoveAurasDueToSpell(AURA_BANISH);

			// Leotheras is getting immune again 
            m_creature->ApplySpellImmune(AURA_BANISH, IMMUNITY_MECHANIC, MECHANIC_BANISH, true);

			// changing model to bloodelf 
            m_creature->SetUInt32Value(UNIT_FIELD_DISPLAYID, MODEL_NIGHTELF);

			// and reseting equipment
			m_creature->LoadEquipment(m_creature->GetEquipmentId());

			if (m_pInstance && m_pInstance->GetData64(DATA_LEOTHERAS_EVENT_STARTER))
			{
				Unit *victim = NULL;
				victim = Unit::GetUnit(*m_creature, m_pInstance->GetData64(DATA_LEOTHERAS_EVENT_STARTER));
				if(victim)
					m_creature->getThreatManager().addThreat(victim, 1);
			}
			
		}
		else if (channelers != 0 && !m_creature->HasAura(AURA_BANISH, 0))
		{
			// channelers != 0 apply banish aura
			// removing Leotheras banish immune to apply AURA_BANISH
            m_creature->ApplySpellImmune(AURA_BANISH, IMMUNITY_MECHANIC, MECHANIC_BANISH, false);
			DoCast(m_creature, AURA_BANISH);

			// changing model
            m_creature->SetUInt32Value(UNIT_FIELD_DISPLAYID, MODEL_DEMON);
			
			// and removing weapons
            m_creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY  , 0);
            m_creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY+1, 0);
		}

	}

	void DoMeleeAttackIfReady()
    {
		if (!m_creature->getVictim())
            return;
        if (m_creature->IsWithinDistInMap(m_creature->getVictim(), ATTACK_DISTANCE) && !m_creature->IsNonMeleeSpellCasted(false))
        {
            //Check for base attack
            if( m_creature->isAttackReady() && m_creature->getVictim())
            {
                m_creature->AttackerStateUpdate(m_creature->getVictim());
                m_creature->resetAttackTimer();
            }
            //Check for offhand attack
            if( m_creature->isAttackReady(OFF_ATTACK) && m_creature->getVictim())
            {
                m_creature->AttackerStateUpdate(m_creature->getVictim(), OFF_ATTACK);
                m_creature->resetAttackTimer(OFF_ATTACK);
            }
        }
    }

	//Despawn all Inner Demon summoned
	void DespawnDemon()
	{
        for(int i=0; i<5; i++)
		{
            if (InnderDemon[i])
			{
                    //delete creature
                    Unit* pUnit = Unit::GetUnit((*m_creature), InnderDemon[i]);
                    if (pUnit && pUnit->isAlive())
					{
                        pUnit->DealDamage(pUnit, pUnit->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
					}
					InnderDemon[i] = 0;
			}
		}

		InnderDemon_Count = 0;
	}

	void CastConsumingMadness()	//remove this once SPELL_INSIDIOUS_WHISPER is supported by core
	{
        for (int i=0; i<5; i++)
		{
            if (InnderDemon[i] > 0 )
			{
                    Unit* pUnit = Unit::GetUnit((*m_creature), InnderDemon[i]);
                    if (pUnit && pUnit->isAlive())
					{
						Unit* pUnit_target = Unit::GetUnit((*pUnit), ((mob_inner_demonAI *)((Creature *)pUnit)->AI())->victimGUID);
						if( pUnit_target && pUnit_target->isAlive())
						{
							pUnit->CastSpell(pUnit_target, SPELL_CONSUMING_MADNESS, true);
							m_creature->getThreatManager().modifyThreatPercent(pUnit_target, -100);
						}
					}
			}
		}
	}

    void KilledUnit(Unit *victim)
    {
        if (victim->GetTypeId() != TYPEID_PLAYER)
            return;

        if (DemonForm)
            switch(rand()%3)
            {
                case 0:
                    //DoYell(SAY_DEMON_SLAY1, LANG_UNIVERSAL, NULL);
                    DoPlaySoundToSet(m_creature, SOUND_DEMON_SLAY1);
                    break;
                case 1:
                    //DoYell(SAY_DEMON_SLAY2, LANG_UNIVERSAL, NULL);
                    DoPlaySoundToSet(m_creature, SOUND_DEMON_SLAY2);
                    break;
                case 2:
                    //DoYell(SAY_DEMON_SLAY3, LANG_UNIVERSAL, NULL);
                    DoPlaySoundToSet(m_creature, SOUND_DEMON_SLAY3);
                    break;
            }
            else
                switch(rand()%3)
                {
                    case 0:
                        //DoYell(SAY_NIGHTELF_SLAY1, LANG_UNIVERSAL, NULL);
                        DoPlaySoundToSet(m_creature, SOUND_NIGHTELF_SLAY1);
                        break;
                    case 1:
                        //DoYell(SAY_NIGHTELF_SLAY2, LANG_UNIVERSAL, NULL);
                        DoPlaySoundToSet(m_creature, SOUND_NIGHTELF_SLAY2);
                        break;
                    case 2:
                        //DoYell(SAY_NIGHTELF_SLAY3, LANG_UNIVERSAL, NULL);
                        DoPlaySoundToSet(m_creature, SOUND_NIGHTELF_SLAY3);
                        break;
                }
    }

    void JustDied (Unit *victim)
    {
        //DoYell(SAY_DEATH, LANG_UNIVERSAL, NULL);
        DoPlaySoundToSet(m_creature, SOUND_DEATH);

        //despawn copy
        if (Demon)
        {
            Unit *pUnit = NULL;
            pUnit = Unit::GetUnit((*m_creature), Demon);

            if (pUnit)
                pUnit->DealDamage(pUnit, pUnit->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
			
		}

        if (m_pInstance)
            m_pInstance->SetData(DATA_LEOTHERASTHEBLINDEVENT, 0);
    }

    void Aggro(Unit *who)
    {

		if (m_creature->HasAura(AURA_BANISH, 0))
			return;

		m_creature->LoadEquipment(m_creature->GetEquipmentId());
		StartEvent();
    }

    void UpdateAI (const uint32 diff)
    {
        //Return since we have no target
        if (m_creature->HasAura(AURA_BANISH, 0) || !m_creature->SelectHostilTarget() || !m_creature->getVictim())
		{
			CheckBanish();
            return;
		}

		if (m_creature->HasAura(SPELL_WHIRLWIND, 0))
			if (m_uiWhirlwind_Timer < diff)
			{
				Unit *newTarget = SelectUnit(SELECT_TARGET_RANDOM, 0);
				if (newTarget)
				{
					DoResetThreat();				
					m_creature->GetMotionMaster()->Clear();
					m_creature->GetMotionMaster()->MovePoint(0,newTarget->GetPositionX(),newTarget->GetPositionY(),newTarget->GetPositionZ());
				}
				m_uiWhirlwind_Timer = 2000;
			}else m_uiWhirlwind_Timer -= diff;

		// reseting after changing forms and after ending whirlwind
		if (NeedThreatReset && !m_creature->HasAura(SPELL_WHIRLWIND, 0))
		{	
			// and reseting attack timer to 0
			m_creature->setAttackTimer(BASE_ATTACK, 0);
			m_creature->setAttackTimer(OFF_ATTACK, 0);

			// when changing forms seting timers (or when ending whirlwind - to avoid adding new variable i use Whirlwind_Timer to countdown 2s while whirlwinding)
			if (DemonForm)
				m_uiInnerDemons_Timer = 30000;
			else
				m_uiWhirlwind_Timer =  15000;
			
			NeedThreatReset = false;
			DoResetThreat();
			m_creature->GetMotionMaster()->Clear();
			m_creature->GetMotionMaster()->MoveChase(m_creature->getVictim());
		}

        //Enrage_Timer ( 10 min )
        if (m_uiBerserk_Timer < diff && !EnrageUsed)
        {
            DoCast(m_creature, SPELL_BERSERK);
			EnrageUsed = true;
        }else m_uiBerserk_Timer -= diff;

        if (!DemonForm)
        {
            //Whirlwind_Timer
			if (!m_creature->HasAura(SPELL_WHIRLWIND, 0))
			{
				if (m_uiWhirlwind_Timer < diff)
				{
					DoCast(m_creature, SPELL_WHIRLWIND);
					// while whirlwinding this variable is used to countdown target's change
					m_uiWhirlwind_Timer = 2000;
					NeedThreatReset = true;
				}else m_uiWhirlwind_Timer -= diff;
			}

            //Switch_Timer
            if (!IsFinalForm)
                if (m_uiSwitchToDemon_Timer < diff)
                {
                    //switch to demon form
					m_creature->RemoveAurasDueToSpell(SPELL_WHIRLWIND,0);
                    m_creature->SetUInt32Value(UNIT_FIELD_DISPLAYID, MODEL_DEMON);
                    //DoYell(SAY_SWITCH_TO_DEMON, LANG_UNIVERSAL, NULL);
                    DoPlaySoundToSet(m_creature, SOUND_SWITCH_TO_DEMON);

					m_creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY  , 0);
				    m_creature->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY+1, 0);
					
					DemonForm = true;
					NeedThreatReset = true;

                    m_uiSwitchToDemon_Timer = 45000;
                }else m_uiSwitchToDemon_Timer -= diff;
            DoMeleeAttackIfReady();
        }
        else
        {
            //ChaosBlast_Timer
			if (!m_creature->getVictim())
				return;
			if (m_creature->GetDistance(m_creature->getVictim()) < 30)
				m_creature->StopMoving();

            if (m_uiChaosBlast_Timer < diff)
            {
				// will cast only when in range of spell 
				if (m_creature->GetDistance(m_creature->getVictim()) < 30)
				{
					//m_creature->CastSpell(m_creature->getVictim(), SPELL_CHAOS_BLAST, true);
					int damage = 100;
					m_creature->CastCustomSpell(m_creature->getVictim(), SPELL_CHAOS_BLAST, &damage, NULL, NULL, false, NULL, NULL, m_creature->GetGUID());  

					m_uiChaosBlast_Timer = 3000;
				}
            }else m_uiChaosBlast_Timer -= diff;

			//summon Inner Demon
			if (m_uiInnerDemons_Timer < diff)
			{
				std::list<HostilReference *>& ThreatList = m_creature->getThreatManager().getThreatList();
				std::vector<Unit *> TargetList; 

				for (std::list<HostilReference *>::iterator itr = ThreatList.begin(); itr != ThreatList.end(); ++itr)
				{
					Unit *tempTarget = Unit::GetUnit(*m_creature, (*itr)->getUnitGuid());
					if (tempTarget && tempTarget->GetTypeId() == TYPEID_PLAYER && tempTarget->GetGUID() != m_creature->getVictim()->GetGUID() && TargetList.size()<5)
						TargetList.push_back( tempTarget );
				}	
				SpellEntry *spell = (SpellEntry *)GetSpellStore()->LookupEntry(SPELL_INSIDIOUS_WHISPER);
				for (std::vector<Unit *>::iterator itr = TargetList.begin(); itr != TargetList.end(); ++itr)
				{
					if ((*itr) && (*itr)->isAlive() )
					{
						Creature * demon = (Creature *)m_creature->SummonCreature(INNER_DEMON_ID, (*itr)->GetPositionX()+10, (*itr)->GetPositionY()+10, (*itr)->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
						if (demon)
						{
							((ScriptedAI *)demon->AI())->AttackStart( (*itr) );
							((mob_inner_demonAI *)demon->AI())->victimGUID = (*itr)->GetGUID();
														
							for (int i=0; i<3; i++)
							{
								if (!spell->Effect[i])
									continue;
								(*itr)->AddAura(new InsidiousAura(spell, i, NULL, (*itr), (*itr)));
							}
							if (InnderDemon_Count > 4) InnderDemon_Count = 0;

							//Safe storing of creatures
							InnderDemon [InnderDemon_Count] = demon->GetGUID();

							//Update demon count
							InnderDemon_Count++;
						}
					}
				}
				DoPlaySoundToSet(m_creature, SOUND_INNER_DEMONS);
				//DoYell(SAY_INNER_DEMONS, LANG_UNIVERSAL, NULL);
				m_uiInnerDemons_Timer = 999999;
			}else m_uiInnerDemons_Timer -= diff;

            //Switch_Timer
            if (m_uiSwitchToHuman_Timer < diff)
            {
                //switch to nightelf form
                m_creature->SetUInt32Value(UNIT_FIELD_DISPLAYID, MODEL_NIGHTELF);
				m_creature->LoadEquipment(m_creature->GetEquipmentId());

				CastConsumingMadness();
				DespawnDemon();

                DemonForm = false;
				NeedThreatReset = true;

                m_uiSwitchToHuman_Timer = 60000;
            }else m_uiSwitchToHuman_Timer -= diff;
        }

        if (!IsFinalForm && (m_creature->GetHealth()*100 / m_creature->GetMaxHealth()) < 15)
        {
			//at this point he divides himself in two parts
            Creature *Copy = NULL;
            Copy = DoSpawnCreature(DEMON_FORM, 0, 0, 0, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 6000);
			if (Copy)
            {
                Demon = Copy->GetGUID();
				if (m_creature->getVictim())
					Copy->AI()->AttackStart(m_creature->getVictim());
            }
            //set nightelf final form
            IsFinalForm = true;
            DemonForm = false;
            //DoYell(SAY_FINAL_FORM, LANG_UNIVERSAL, NULL);
            DoPlaySoundToSet(m_creature, SOUND_FINAL_FORM);
			m_creature->SetUInt32Value(UNIT_FIELD_DISPLAYID, MODEL_NIGHTELF);
			m_creature->LoadEquipment(m_creature->GetEquipmentId());
        }
    }
};

//Leotheras the Blind Demon Form AI
struct MANGOS_DLL_DECL boss_leotheras_the_blind_demonformAI : public ScriptedAI
{
    boss_leotheras_the_blind_demonformAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        Reset();
    }

    uint32 ChaosBlast_Timer;
	bool DealDamage;

    void Reset()
    {
        ChaosBlast_Timer = 1000;
		DealDamage = true;
    }

    void StartEvent()
    {
        //DoYell(SAY_FREE, LANG_UNIVERSAL, NULL);
        DoPlaySoundToSet(m_creature, SOUND_FREE);
    }

    void KilledUnit(Unit *victim)
    {
        if (victim->GetTypeId() != TYPEID_PLAYER)
            return;

        switch(rand()%3)
        {
            case 0:
                //DoYell(SAY_DEMON_SLAY1, LANG_UNIVERSAL, NULL);
                DoPlaySoundToSet(m_creature, SOUND_DEMON_SLAY1);
                break;
            case 1:
                //DoYell(SAY_DEMON_SLAY2, LANG_UNIVERSAL, NULL);
                DoPlaySoundToSet(m_creature, SOUND_DEMON_SLAY2);
                break;
            case 2:
                //DoYell(SAY_DEMON_SLAY3, LANG_UNIVERSAL, NULL);
                DoPlaySoundToSet(m_creature, SOUND_DEMON_SLAY3);
                break;
        }
    }

    void JustDied(Unit *victim)
    {
        //invisibility (blizzlike, at the end of the fight he doesn't die, he disappears)
        m_creature->CastSpell(m_creature, 8149, true);
    }

    void Aggro(Unit *who)
    {
		StartEvent();
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim() )
            return;

         //ChaosBlast_Timer
		if (m_creature->GetDistance(m_creature->getVictim()) < 30)
			 m_creature->StopMoving();

        if (ChaosBlast_Timer < diff)
        {
			// will cast only when in range od spell 
			if (m_creature->GetDistance(m_creature->getVictim()) < 30)
			{
				//m_creature->CastSpell(m_creature->getVictim(),SPELL_CHAOS_BLAST,true);
				int damage = 100;
				m_creature->CastCustomSpell(m_creature->getVictim(), SPELL_CHAOS_BLAST, &damage, NULL, NULL, false, NULL, NULL, m_creature->GetGUID());  

				ChaosBlast_Timer = 3000;
			}
        }else ChaosBlast_Timer -= diff;
		//Do NOT deal any melee damage to the target.
    }
};

struct MANGOS_DLL_DECL mob_greyheart_spellbinderAI : public ScriptedAI
{
	mob_greyheart_spellbinderAI(Creature* pCreature) : ScriptedAI(pCreature)
	{
		m_pInstance = ((ScriptedInstance *)pCreature->GetInstanceData());
		leotherasGUID = 0;
		AddedBanish = false;
		Reset();
	}

	ScriptedInstance *m_pInstance;

	uint64 leotherasGUID;

	uint32 Mindblast_Timer;
	uint32 Earthshock_Timer;

	bool AddedBanish;

	void Reset()
	{
		Mindblast_Timer  = 3000 + rand()%5000;
		Earthshock_Timer = 5000 + rand()%5000;
		if (m_pInstance)
		   {
		   m_pInstance->SetData64(DATA_LEOTHERAS_EVENT_STARTER, 0);
		   Creature *leotheras = (Creature *)Unit::GetUnit(*m_creature, leotherasGUID);
		   if(leotheras && leotheras->isAlive())
		      ((boss_leotheras_the_blindAI*)leotheras->AI())->CheckChannelers(false);
		   }
	}

	void Aggro(Unit *who)
	{
		m_creature->InterruptNonMeleeSpells(false);
		if (m_pInstance)
			 m_pInstance->SetData64(DATA_LEOTHERAS_EVENT_STARTER, who->GetGUID());
	}

	void JustRespawned()
	{
		AddedBanish = false;
		Reset();
	}

	void CastChanneling()
	{
		if (!m_creature->isInCombat() && !m_creature->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
		   if (leotherasGUID)
		      {
		           Creature *leotheras = (Creature *)Unit::GetUnit(*m_creature, leotherasGUID);
		           if (leotheras && leotheras->isAlive())
		               {
		                    DoCast(leotheras, BANISH_BEAM);
		                    if (!AddedBanish)
		                        {
		                        ((boss_leotheras_the_blindAI *)leotheras->AI())->channelers++;
		                        AddedBanish = true;
		                        }
		               }
		      }
	}

	void UpdateAI(const uint32 diff)
	{
		if (m_pInstance)
		{	
		      if (!leotherasGUID)
		          leotherasGUID = m_pInstance->GetData64(DATA_LEOTHERAS);

		      if (!m_creature->isInCombat() && m_pInstance->GetData64(DATA_LEOTHERAS_EVENT_STARTER))
			{
				Unit *victim = NULL;
				victim = Unit::GetUnit(*m_creature, m_pInstance->GetData64(DATA_LEOTHERAS_EVENT_STARTER));
				if(victim)
				AttackStart(victim);
			}
		}
		
		if (!m_creature->SelectHostilTarget() || !m_creature->getVictim())
		{
			CastChanneling();
			return;
		}

		//if (m_pInstance && !m_pInstance->GetData64(DATA_LEOTHERAS_EVENT_STARTER))
			//EnterEvadeMode();

		if (Mindblast_Timer < diff)
		{
			Unit* target = NULL;
            target = SelectUnit(SELECT_TARGET_RANDOM,0);

			if ( target )DoCast(target, SPELL_MINDBLAST);

			Mindblast_Timer = 10000 + rand()%5000;
		}else Mindblast_Timer -= diff;

		if(Earthshock_Timer < diff)
		{
			Map *map = m_creature->GetMap();
			Map::PlayerList const &PlayerList = map->GetPlayers();
			for(Map::PlayerList::const_iterator itr = PlayerList.begin();itr != PlayerList.end(); ++itr)
            {
                if (Player* i_pl = itr->getSource())
                {
				    bool isCasting = false;
				    for(uint8 i = 0; i < CURRENT_MAX_SPELL; ++i)
if (Spell* spell = i_pl->GetCurrentSpell(CurrentSpellTypes(i)))
						    isCasting = true;

				    if(isCasting)
				    {
					    DoCast(i_pl, SPELL_EARTHSHOCK);
					    break;
				    }
                }
			}
			Earthshock_Timer = 8000 + rand()%7000;
		}else Earthshock_Timer -= diff;
		DoMeleeAttackIfReady();
	}

	void JustDied(Unit *killer)
	{
		if (AddedBanish)
			if (m_pInstance && leotherasGUID)
			{
				Creature *leotheras = (Creature *)Unit::GetUnit(*m_creature, leotherasGUID);
				if(leotheras)
					((boss_leotheras_the_blindAI *)leotheras->AI())->channelers--;
			}
	}
};

CreatureAI* GetAI_boss_leotheras_the_blind(Creature* pCreature)
{
    return new boss_leotheras_the_blindAI(pCreature);
}

CreatureAI* GetAI_boss_leotheras_the_blind_demonform(Creature* pCreature)
{
    return new boss_leotheras_the_blind_demonformAI(pCreature);
}

CreatureAI* GetAI_mob_greyheart_spellbinder(Creature* pCreature)
{
	return new mob_greyheart_spellbinderAI(pCreature);
}

CreatureAI* GetAI_mob_inner_demon(Creature* pCreature)
{
	return new mob_inner_demonAI(pCreature);
}

void AddSC_boss_leotheras_the_blind()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "boss_leotheras_the_blind";
    newscript->GetAI = &GetAI_boss_leotheras_the_blind;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "boss_leotheras_the_blind_demonform";
    newscript->GetAI = &GetAI_boss_leotheras_the_blind_demonform;
    newscript->RegisterSelf();

	newscript = new Script;
    newscript->Name = "mob_greyheart_spellbinder";
    newscript->GetAI = &GetAI_mob_greyheart_spellbinder;
    newscript->RegisterSelf();

	newscript = new Script;
    newscript->Name = "mob_inner_demon";
    newscript->GetAI = &GetAI_mob_inner_demon;
    newscript->RegisterSelf();
}
