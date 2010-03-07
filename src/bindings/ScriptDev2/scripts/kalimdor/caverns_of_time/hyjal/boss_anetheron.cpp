#include "precompiled.h"
#include "def_hyjal.h"
#include "hyjal_trash.h"

#define SPELL_CARRION_SWARM 31306
#define SPELL_SLEEP 31298
#define SPELL_VAMPIRIC_AURA 38196
#define SPELL_INFERNO 31299

#define SAY_ONDEATH "The clock... is still... ticking."
#define SOUND_ONDEATH 10982

#define SAY_ONSLAY1 "Your hopes are lost!"
#define SAY_ONSLAY2 "Scream for me!"
#define SAY_ONSLAY3 "Pity, no time for a slow death!"
#define SOUND_ONSLAY1 10981
#define SOUND_ONSLAY2 11038
#define SOUND_ONSLAY3 11039

#define SAY_SWARM1 "The swarm is eager to feed!"
#define SAY_SWARM2 "Pestilence upon you!"
#define SOUND_SWARM1 10979
#define SOUND_SWARM2 11037

#define SAY_SLEEP1 "You look tired..."
#define SAY_SLEEP2 "Sweet dreams..."
#define SOUND_SLEEP1 10978
#define SOUND_SLEEP2 11545

#define SAY_INFERNO1 "Let fire rain from above!"
#define SAY_INFERNO2 "Earth and sky shall burn!"
#define SOUND_INFERNO1 10980
#define SOUND_INFERNO2 11036

#define SAY_ONAGGRO "You are defenders of a doomed world! Flee here, and perhaps you will prolong your pathetic lives!"
#define SOUND_ONAGGRO 10977

struct MANGOS_DLL_DECL boss_anetheronAI : public ScriptedAI
{
    boss_anetheronAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        pInstance = ((ScriptedInstance*)pCreature->GetInstanceData());
        go = false;    
        pos = 0;
        SpellEntry *TempSpell = (SpellEntry*)GetSpellStore()->LookupEntry(SPELL_SLEEP);
        if(TempSpell && TempSpell->EffectImplicitTargetA[0] != 1)
        {
            TempSpell->EffectImplicitTargetA[0] = 1;
            TempSpell->EffectImplicitTargetB[0] = 0;
        }
        Reset();
    }

    uint32 SwarmTimer;
    uint32 SleepTimer;
    uint32 AuraTimer;
    uint32 InfernoTimer;
    bool go;
    uint32 pos;
        ScriptedInstance* pInstance;
        bool IsEvent;
        uint32 Delay;
        uint32 LastOverronPos;
        bool IsOverrun;
        bool SetupOverrun;
        uint32 OverrunType;
        uint8 faction;
        bool useFlyPath;
        uint32 damageTaken;
        float DummyTarget[3];

    void Reset()
    {
        SwarmTimer = 45000;
        SleepTimer = 60000;
        AuraTimer = 5000;
        InfernoTimer = 45000;

        if(pInstance && IsEvent)
            pInstance->SetData(DATA_ANETHERONEVENT, NOT_STARTED);
    }

    void Aggro(Unit *who)
    {    
        if(pInstance && IsEvent)
            pInstance->SetData(DATA_ANETHERONEVENT, IN_PROGRESS);
        DoPlaySoundToSet(m_creature, SOUND_ONAGGRO);
        m_creature->MonsterYell(SAY_ONAGGRO, LANG_UNIVERSAL, NULL);
    }

    void KilledUnit(Unit *victim)
    {
        switch(rand()%3)
        {
            case 0:
                DoPlaySoundToSet(m_creature, SOUND_ONSLAY1);
                m_creature->MonsterYell(SAY_ONSLAY1, LANG_UNIVERSAL, NULL);
                break;
            case 1:
                DoPlaySoundToSet(m_creature, SOUND_ONSLAY2);
                m_creature->MonsterYell(SAY_ONSLAY2, LANG_UNIVERSAL, NULL);
                break;        
            case 2:
                DoPlaySoundToSet(m_creature, SOUND_ONSLAY3);
                m_creature->MonsterYell(SAY_ONSLAY3, LANG_UNIVERSAL, NULL);
                break;    
        }        
    }

    void JustDied(Unit *victim)
    {
        if(pInstance && IsEvent)
            pInstance->SetData(DATA_ANETHERONEVENT, DONE);
        DoPlaySoundToSet(m_creature, SOUND_ONDEATH);
        m_creature->MonsterYell(SAY_ONDEATH, LANG_UNIVERSAL, NULL);
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target
        if(!m_creature->SelectHostileTarget() || !m_creature->getVictim() )
            return;

        if(SwarmTimer < diff)
        {
            Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
            if(target)
                DoCast(target,SPELL_CARRION_SWARM);

            SwarmTimer = 45000+rand()%15000;
            switch(rand()%2)
            {
                case 0:
                    DoPlaySoundToSet(m_creature, SOUND_SWARM1);
                    m_creature->MonsterYell(SAY_SWARM1, LANG_UNIVERSAL, NULL);
                    break;
                case 1:
                    DoPlaySoundToSet(m_creature, SOUND_SWARM2);
                    m_creature->MonsterYell(SAY_SWARM2, LANG_UNIVERSAL, NULL);
                    break;        
            }    
        }else SwarmTimer -= diff;

        if(SleepTimer < diff)
        {
            for(uint8 i=0;i<3;++i)
            {
                Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0);
                if(target)
                    target->CastSpell(target,SPELL_SLEEP,true);
            }
            SleepTimer = 60000;
            switch(rand()%2)
            {
                case 0:
                    DoPlaySoundToSet(m_creature, SOUND_SLEEP1);
                    m_creature->MonsterYell(SAY_SLEEP1, LANG_UNIVERSAL, NULL);
                    break;
                case 1:
                    DoPlaySoundToSet(m_creature, SOUND_SLEEP2);
                    m_creature->MonsterYell(SAY_SLEEP2, LANG_UNIVERSAL, NULL);
                    break;        
            }    
        }else SleepTimer -= diff;        
        if(AuraTimer < diff)
        {
            DoCast(m_creature, SPELL_VAMPIRIC_AURA,true);
            AuraTimer = 10000+rand()%10000;            
        }else AuraTimer -= diff;
        if(InfernoTimer < diff)
        {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_INFERNO);
            InfernoTimer = 45000;
            switch(rand()%2)
            {
                case 0:
                    DoPlaySoundToSet(m_creature, SOUND_INFERNO1);
                    m_creature->MonsterYell(SAY_INFERNO1, LANG_UNIVERSAL, NULL);
                    break;
                case 1:
                    DoPlaySoundToSet(m_creature, SOUND_INFERNO2);
                    m_creature->MonsterYell(SAY_INFERNO2, LANG_UNIVERSAL, NULL);
                    break;        
            }    
        }else InfernoTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_anetheron(Creature* pCreature)
{
    return new boss_anetheronAI(pCreature);
}

#define SPELL_IMMOLATION 31303
#define SPELL_INFERNO_EFFECT 31302

struct MANGOS_DLL_DECL mob_towering_infernalAI : public ScriptedAI
{
    mob_towering_infernalAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        pInstance = ((ScriptedInstance*)pCreature->GetInstanceData());
        if(pInstance)
            AnetheronGUID = pInstance->GetData64(DATA_ANETHERON);
    }

    uint32 ImmolationTimer;
    uint32 CheckTimer;
    uint64 AnetheronGUID;
    ScriptedInstance* pInstance;

    void Reset()
    {
        DoCast(m_creature, SPELL_INFERNO_EFFECT);
        ImmolationTimer = 5000;
        CheckTimer = 5000;
    }

    void Aggro(Unit *who)
    {    
        
    }

    void KilledUnit(Unit *victim)
    {
                
    }

    void JustDied(Unit *victim)
    {
        
    }

    void MoveInLineOfSight(Unit *who)
    {
        if (m_creature->GetDistance(who) <= 50 && !m_creature->isInCombat() && m_creature->IsHostileTo(who))
        {
            m_creature->AddThreat(who,0.0);
            m_creature->SetActiveObjectState(true);
            m_creature->Attack(who,false);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(CheckTimer < diff)
        {
            if(AnetheronGUID)
            {
                Creature *boss = (Creature*)Unit::GetUnit((*m_creature),AnetheronGUID);
				//Creature *pMidnight = (Creature*)Unit::GetUnit((*m_creature, Midnight);
                if(!boss || (boss && boss->isDead()))
                {
                    m_creature->setDeathState(JUST_DIED);
                    m_creature->RemoveCorpse();
                    return;
                }
            }
            CheckTimer = 5000;
        }else CheckTimer -= diff;

        //Return since we have no target
        if(!m_creature->SelectHostileTarget() || !m_creature->getVictim() )
            return;

        if(ImmolationTimer < diff)
        {
            DoCast(m_creature, SPELL_IMMOLATION);        
            ImmolationTimer = 5000;
        }else ImmolationTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_towering_infernal(Creature* pCreature)
{
    return new mob_towering_infernalAI(pCreature);
}

void AddSC_boss_anetheron()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_anetheron";
    newscript->GetAI = &GetAI_boss_anetheron;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_towering_infernal";
    newscript->GetAI = &GetAI_mob_towering_infernal;
    newscript->RegisterSelf();    
}
