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
SDName: Orgrimmar
SD%Complete: 100
SDComment: Quest support: 2460, 5727, 6566
SDCategory: Orgrimmar
EndScriptData */

/* ContentData
npc_neeru_fireblade     npc_text + gossip options text missing
npc_shenthul
npc_thrall_warchief
EndContentData */

#include "precompiled.h"

/*######
## npc_neeru_fireblade
######*/

#define QUEST_5727  5727

bool GossipHello_npc_neeru_fireblade(Player *player, Creature *_Creature)
{
    if (_Creature->isQuestGiver())
        player->PrepareQuestMenu( _Creature->GetGUID() );

    if (player->GetQuestStatus(QUEST_5727) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM(0, "You may speak frankly, Neeru...", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    player->SEND_GOSSIP_MENU(4513, _Creature->GetGUID());
    return true;
}

bool GossipSelect_npc_neeru_fireblade(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF+1:
            player->ADD_GOSSIP_ITEM(0, "[PH] ...", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
            player->SEND_GOSSIP_MENU(4513, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            player->CLOSE_GOSSIP_MENU();
            player->AreaExploredOrEventHappens(QUEST_5727);
            break;
    }
    return true;
}

/*######
## npc_shenthul
######*/

enum
{
    QUEST_SHATTERED_SALUTE  = 2460
};

struct MANGOS_DLL_DECL npc_shenthulAI : public ScriptedAI
{
    npc_shenthulAI(Creature* c) : ScriptedAI(c) { Reset(); }

    bool CanTalk;
    bool CanEmote;
    uint32 Salute_Timer;
    uint32 Reset_Timer;
    uint64 playerGUID;

    void Reset()
    {
        CanTalk = false;
        CanEmote = false;
        Salute_Timer = 6000;
        Reset_Timer = 0;
        playerGUID = 0;
    }

    void Aggro(Unit* who) { }

   void ReciveEmote(Player *player, uint32 emote)
    {
        if (((emote == TEXTEMOTE_SALUTE) || (emote == EMOTE_ONESHOT_SALUTE))&& player->GetQuestStatus(QUEST_SHATTERED_SALUTE) == QUEST_STATUS_INCOMPLETE)
        {
player->AreaExploredOrEventHappens(QUEST_SHATTERED_SALUTE);
            if (CanEmote)
            {
                player->AreaExploredOrEventHappens(QUEST_SHATTERED_SALUTE);
                Reset();
            }
        }
    }


    void UpdateAI(const uint32 diff)
    {
        if (CanEmote)
        {
            if (Reset_Timer < diff)
            {
                if (Player* pPlayer = (Player*)Unit::GetUnit((*m_creature),playerGUID))
                {
                    if (pPlayer->GetTypeId() == TYPEID_PLAYER && pPlayer->GetQuestStatus(QUEST_SHATTERED_SALUTE) == QUEST_STATUS_INCOMPLETE)
                        pPlayer->FailQuest(QUEST_SHATTERED_SALUTE);
                }
                Reset();
            } else Reset_Timer -= diff;
        }

        if (CanTalk && !CanEmote)
        {
            if (Salute_Timer < diff)
            {
                m_creature->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
                CanEmote = true;
                Reset_Timer = 60000;
            } else Salute_Timer -= diff;
        }

        if (!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;

        DoMeleeAttackIfReady();
    }
};
bool ReciveEmote_npc_shenthul(Player *player, Creature *_Creature, uint32 emote)
{
   if( emote == TEXTEMOTE_SALUTE && player->GetQuestStatus(QUEST_SHATTERED_SALUTE) == QUEST_STATUS_INCOMPLETE )
      if( ((npc_shenthulAI*)_Creature->AI())->CanEmote )
        {
           player->AreaExploredOrEventHappens(QUEST_SHATTERED_SALUTE);
           ((npc_shenthulAI*)_Creature->AI())->Reset();
        }
   return true;
}

CreatureAI* GetAI_npc_shenthul(Creature *_Creature)
{
    return new npc_shenthulAI(_Creature);
}

bool QuestAccept_npc_shenthul(Player* player, Creature* creature, Quest const* quest)
{
    if (quest->GetQuestId() == QUEST_SHATTERED_SALUTE)
    {
        ((npc_shenthulAI*)creature->AI())->CanTalk = true;
        ((npc_shenthulAI*)creature->AI())->playerGUID = player->GetGUID();
    }
    return true;
}

/*######
## npc_thrall_warchief
######*/

#define QUEST_6566              6566

#define SPELL_CHAIN_LIGHTNING   16033
#define SPELL_SHOCK             16034

//TODO: verify abilities/timers
struct MANGOS_DLL_DECL npc_thrall_warchiefAI : public ScriptedAI
{
    npc_thrall_warchiefAI(Creature* c) : ScriptedAI(c) { Reset(); }

    uint32 ChainLightning_Timer;
    uint32 Shock_Timer;

    void Reset()
    {
        ChainLightning_Timer = 2000;
        Shock_Timer = 8000;
    }

    void Aggro(Unit *who) {}

    void UpdateAI(const uint32 diff)
    {
        if(!m_creature->SelectHostilTarget() || !m_creature->getVictim())
            return;

        if( ChainLightning_Timer < diff )
        {
            DoCast(m_creature->getVictim(),SPELL_CHAIN_LIGHTNING);
            ChainLightning_Timer = 9000;
        }else ChainLightning_Timer -= diff;

        if( Shock_Timer < diff )
        {
            DoCast(m_creature->getVictim(),SPELL_SHOCK);
            Shock_Timer = 15000;
        }else Shock_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_npc_thrall_warchief(Creature *_Creature)
{
    return new npc_thrall_warchiefAI (_Creature);
}

bool GossipHello_npc_thrall_warchief(Player *player, Creature *_Creature)
{
    if (_Creature->isQuestGiver())
        player->PrepareQuestMenu( _Creature->GetGUID() );

    if (player->GetQuestStatus(QUEST_6566) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM(0, "Please share your wisdom with me, Warchief.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    player->SEND_GOSSIP_MENU(_Creature->GetNpcTextId(), _Creature->GetGUID());
    return true;
}

bool GossipSelect_npc_thrall_warchief(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF+1:
            player->ADD_GOSSIP_ITEM(0, "What discoveries?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
            player->SEND_GOSSIP_MENU(5733, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            player->ADD_GOSSIP_ITEM(0, "Usurper?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
            player->SEND_GOSSIP_MENU(5734, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+3:
            player->ADD_GOSSIP_ITEM(0, "With all due respect, Warchief - why not allow them to be destroyed? Does this not strengthen our position?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
            player->SEND_GOSSIP_MENU(5735, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+4:
            player->ADD_GOSSIP_ITEM(0, "I... I did not think of it that way, Warchief.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
            player->SEND_GOSSIP_MENU(5736, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+5:
            player->ADD_GOSSIP_ITEM(0, "I live only to serve, Warchief! My life is empty and meaningless without your guidance.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+6);
            player->SEND_GOSSIP_MENU(5737, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+6:
            player->ADD_GOSSIP_ITEM(0, "Of course, Warchief!", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+7);
            player->SEND_GOSSIP_MENU(5738, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+7:
            player->CLOSE_GOSSIP_MENU();
            player->AreaExploredOrEventHappens(QUEST_6566);
            break;
    }
    return true;
}

/*######
## npc_eitrigg q4941
######*/

bool GossipHello_npc_eitrigg(Player *player, Creature *_Creature)
{
    if (_Creature->isQuestGiver())
        player->PrepareQuestMenu( _Creature->GetGUID() );

    if (player->GetQuestStatus(4941) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM( 0, "Maybe I need to talk about this with Trall?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    player->SEND_GOSSIP_MENU(_Creature->GetNpcTextId(), _Creature->GetGUID());

    return true;
}

bool GossipSelect_npc_eitrigg(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF+1:
            player->CLOSE_GOSSIP_MENU();
            player->AreaExploredOrEventHappens(4941);
            break;
    }
    return true;
}

void AddSC_orgrimmar()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "npc_neeru_fireblade";
    newscript->pGossipHello =  &GossipHello_npc_neeru_fireblade;
    newscript->pGossipSelect = &GossipSelect_npc_neeru_fireblade;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_shenthul";
    //newscript->ReciveEmote =&ReciveEmote_npc_shenthul;
    //newscript->pReceiveEmote = &ReciveEmote_npc_shenthul;
    newscript->GetAI = &GetAI_npc_shenthul;

    newscript->pQuestAccept =  &QuestAccept_npc_shenthul;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_thrall_warchief";
    newscript->GetAI = &GetAI_npc_thrall_warchief;
    newscript->pGossipHello =  &GossipHello_npc_thrall_warchief;
    newscript->pGossipSelect = &GossipSelect_npc_thrall_warchief;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_eitrigg";
    newscript->pGossipHello =  &GossipHello_npc_eitrigg;
    newscript->pGossipSelect = &GossipSelect_npc_eitrigg;
    newscript->RegisterSelf();
}
