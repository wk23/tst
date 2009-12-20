/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.**See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA**02111-1307**USA
 */

/* ScriptData
SDName: Boss Swamplord Musel'ek
SD%Complete: 99
SDComment:Need the right text id
SDCategory: Coilfang Resevoir, The Underbog
EndScriptData */

/* ContentData
boss_swamplord_muselek
mob_claw
EndContentData */

#include "precompiled.h"

enum
{
	//Spells
    SPELL_AIMED_SHOT                         = 31623,
    SPELL_DETERRENCE                         = 31567,
    SPELL_SHOOT                              = 22907,
    SPELL_RAPTOR_STRIKE                      = 31566,
    SPELL_MULTI_SHOT                         = 34974,
    SPELL_HUNTERS_MARK                       = 31615,
    SPELL_THROW_FREEZING_TRAP                = 31946,
	
	//SAY_AGGRO_1                              = "I will end this quickly...",
    SOUND_AGGRO_1                            = 10364,
    //SAY_AGGRO_2                              = "We fight to the death!",
    SOUND_AGGRO_2                            = 10364,


    //SAY_ENRAGE_CLAW                          = "Beast! Obey me! Kill them at once!",
    SOUND_ENRAGE_CLAW                        = 10364,

    //SAY_DEAD_1                               = "Krypta!",
    SOUND_DEAD_1                             = 10366,
    //SAY_DEAD_2                               = "It is finished.",
    SOUND_DEAD_2                             = 10366
};


struct MANGOS_DLL_DECL boss_muselekAI : public ScriptedAI
{
    boss_muselekAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        pInstance = ((ScriptedInstance*)pCreature->GetInstanceData());
        Reset();
    }

    ScriptedInstance *pInstance;
    bool HeroicMode;

    uint32 AimedShot_Timer;
    uint32 Shoot_Timer;
    uint32 MultiShot_Timer;
    uint32 RaptorStrike_Timer;
    uint32 Deterrence_Timer;
    uint32 FreezingTrap_Timer;
    uint32 HuntersMark_Timer;
    uint32 FreezingWait_Timer;

    void Reset()
    {
        HeroicMode = m_creature->GetMap()->IsRaidOrHeroicDungeon();

        AimedShot_Timer = 6000;
        Shoot_Timer = 2500;
        MultiShot_Timer = 4500;
        RaptorStrike_Timer = 4000;
        Deterrence_Timer = 17000;
        FreezingTrap_Timer = 20000; 
        HuntersMark_Timer = 1000;
        FreezingWait_Timer = 1000;
    }

    void JustDied(Unit* Killer)
    {
        switch(rand()%2)
        {
            case 0:
                //DoYell(SAY_DEAD_1, LANG_UNIVERSAL, NULL);
                DoPlaySoundToSet(m_creature,SOUND_DEAD_1);
                break;
            case 1:
                //DoYell(SAY_DEAD_2, LANG_UNIVERSAL, NULL);
                DoPlaySoundToSet(m_creature,SOUND_DEAD_2);
                break;
        }
    }

    void Aggro(Unit *who)
    {
        switch(rand()%2)
        {
            case 0:
                //DoYell(SAY_AGGRO_1, LANG_UNIVERSAL, NULL);
                DoPlaySoundToSet(m_creature,SOUND_AGGRO_1);
                break;
            case 1:
                //DoYell(SAY_AGGRO_2, LANG_UNIVERSAL, NULL);
                DoPlaySoundToSet(m_creature,SOUND_AGGRO_2);
                break;
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;
 
        if(FreezingTrap_Timer < diff)
        {
            DoCast(m_creature->getVictim(), SPELL_THROW_FREEZING_TRAP);
            FreezingTrap_Timer = 20000;
            FreezingWait_Timer = 2500;
        }else FreezingTrap_Timer -= diff;

        if(FreezingWait_Timer < diff)
        {
            // Run Away
            float x, y, z; // coords that we move to.
            m_creature->getVictim()->GetClosePoint(x, y, z, m_creature->GetObjectSize(), 15);
            m_creature->GetMotionMaster()->MovePoint(1, x, y, z);
            FreezingWait_Timer = 30000;
        }else FreezingWait_Timer -= diff;

        // Check if he is in melee or ranged range 
        if(m_creature->IsWithinDistInMap(m_creature->getVictim(), 5))
        {
            if(RaptorStrike_Timer < diff)
            {
                DoCast(m_creature->getVictim(), SPELL_RAPTOR_STRIKE);
                RaptorStrike_Timer = 9000;
            }else RaptorStrike_Timer -= diff;

            if(Deterrence_Timer < diff)
            {
                DoCast(m_creature->getVictim(), SPELL_DETERRENCE);
                Deterrence_Timer = 30000;
            }else Deterrence_Timer -= diff;

            if(!m_creature->getVictim()->hasUnitState(UNIT_STAT_STUNNED | UNIT_STAT_ROOT | UNIT_STAT_CONFUSED | UNIT_STAT_DISTRACTED))
                DoMeleeAttackIfReady();
        }else
        {
            if(HuntersMark_Timer < diff)
            {
                DoCast(m_creature->getVictim(), SPELL_HUNTERS_MARK);
                HuntersMark_Timer = 25000;
            }else HuntersMark_Timer -= diff; 

            if(AimedShot_Timer < diff)
            {
                DoCast(m_creature->getVictim(), SPELL_AIMED_SHOT);
                AimedShot_Timer = 12000;
            }else AimedShot_Timer -= diff;
            
            if(MultiShot_Timer < diff)
            {
                DoCast(m_creature->getVictim(), SPELL_MULTI_SHOT);
                MultiShot_Timer = 9500;
            }else MultiShot_Timer -= diff;

            if(Shoot_Timer < diff)
            {
                DoCast(m_creature->getVictim(), SPELL_SHOOT);
                Shoot_Timer = 2500;
            }else Shoot_Timer -= diff;
        }
}

};
#define SPELL_ECHOING_ROAR          31429
#define SPELL_FERAL_CHARGE          39435

struct MANGOS_DLL_DECL mob_clawAI : public ScriptedAI
{
    mob_clawAI(Creature* pCreature) : ScriptedAI(pCreature) {Reset();}

    bool HeroicMode;
    uint32 FeralCharge_Timer;
    uint32 EchoingRoar_Timer;

    void Reset()
    {
        HeroicMode = m_creature->GetMap()->IsRaidOrHeroicDungeon();
        FeralCharge_Timer = 2000+rand()%15000;
        EchoingRoar_Timer = 5000;
    }

    void Aggro(Unit *who) 
    { 
      //HeroicMode = m_creature->GetMap()->IsRaidOrHeroicDungeon();
    }

    void UpdateAI(const uint32 diff)
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        if( FeralCharge_Timer < diff )
        {
            //Cast charge and change target
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_FERAL_CHARGE);
            m_creature->getThreatManager().modifyThreatPercent(m_creature->getVictim(),-100);
            FeralCharge_Timer = 12000+rand()%15000;
        }else FeralCharge_Timer -= diff;

        if( EchoingRoar_Timer < diff )
        {
            DoCast(m_creature, SPELL_ECHOING_ROAR);
            EchoingRoar_Timer = 12000+rand()%15000;
        }else EchoingRoar_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_muselekAI(Creature* pCreature)
{
    return new boss_muselekAI(pCreature);
}

CreatureAI* GetAI_mob_clawAI(Creature* pCreature)
{
    return new mob_clawAI(pCreature);
}

void AddSC_boss_swamplord_muselek()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "boss_swamplord_muselek";
    newscript->GetAI = &GetAI_boss_muselekAI;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_claw";
    newscript->GetAI = &GetAI_mob_clawAI;
    newscript->RegisterSelf();
}