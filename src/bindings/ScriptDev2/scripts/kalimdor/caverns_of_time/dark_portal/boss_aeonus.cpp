/* Copyright (C) 2006 - 2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: Boss_Aeonus
SD%Complete: 80
SDComment: Some spells not implemented
SDCategory: Caverns of Time, The Dark Portal
EndScriptData */

#include "precompiled.h"
#include "dark_portal.h"

#define SAY_ENTER         -1269012
#define SAY_AGGRO         -1269013
#define SAY_BANISH        -1269014
#define SAY_SLAY1         -1269015
#define SAY_SLAY2         -1269016
#define SAY_DEATH         -1269017
#define EMOTE_FRENZY      -1269018

#define SPELL_CLEAVE        40504
#define SPELL_TIME_STOP     31422
#define SPELL_ENRAGE        37605
#define SPELL_SAND_BREATH   31473
#define H_SPELL_SAND_BREATH 39049

struct MANGOS_DLL_DECL boss_aeonusAI : public ScriptedAI
{
    boss_aeonusAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        m_bIsHeroicMode = pCreature->GetMap()->IsRaidOrHeroicDungeon();
        Reset();
    }

    ScriptedInstance* m_pInstance;
    bool m_bIsHeroicMode;

    uint32 SandBreath_Timer;
    uint32 TimeStop_Timer;
    uint32 Frenzy_Timer;

    void Reset()
    {
        SandBreath_Timer = 15000+rand()%15000;
        TimeStop_Timer = 10000+rand()%5000;
        Frenzy_Timer = 30000+rand()%15000;
    }

    void Aggro(Unit *who)
    {
        DoScriptText(SAY_AGGRO, m_creature);
    }

    void MoveInLineOfSight(Unit *who)
    {
        //Despawn Time Keeper
        if (who->GetTypeId() == TYPEID_UNIT && who->GetEntry() == NPC_TIME_KEEPER)
        {
            if (m_creature->IsWithinDistInMap(who,20.0f))
            {
                DoScriptText(SAY_BANISH, m_creature);
                m_creature->DealDamage(who, who->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            }
        }

        ScriptedAI::MoveInLineOfSight(who);
    }

    void JustDied(Unit *victim)
    {
        DoScriptText(SAY_DEATH, m_creature);

        if (m_pInstance)
            m_pInstance->SetData(TYPE_RIFT,DONE);
    }

    void KilledUnit(Unit *victim)
    {
        switch(rand()%2)
        {
            case 0: DoScriptText(SAY_SLAY1, m_creature); break;
            case 1: DoScriptText(SAY_SLAY2, m_creature); break;
        }
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        //Sand Breath
        if (SandBreath_Timer < diff)
        {
            DoCast(m_creature->getVictim(), m_bIsHeroicMode ? H_SPELL_SAND_BREATH : SPELL_SAND_BREATH);
            SandBreath_Timer = 15000+rand()%10000;
        }else SandBreath_Timer -= diff;

        //Time Stop
        if (TimeStop_Timer < diff)
        {
            DoCast(m_creature->getVictim(), SPELL_TIME_STOP);
            TimeStop_Timer = 20000+rand()%15000;
        }else TimeStop_Timer -= diff;

        //Frenzy
        if (Frenzy_Timer < diff)
        {
            DoScriptText(EMOTE_FRENZY, m_creature);
            DoCast(m_creature, SPELL_ENRAGE);
            Frenzy_Timer = 20000+rand()%15000;
        }else Frenzy_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_aeonus(Creature* pCreature)
{
    return new boss_aeonusAI(pCreature);
}

void AddSC_boss_aeonus()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "boss_aeonus";
    newscript->GetAI = &GetAI_boss_aeonus;
    newscript->RegisterSelf();
}
