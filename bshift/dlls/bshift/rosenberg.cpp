/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Rosenberg
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"animation.h"
#include	"soundent.h"
#include	"scientist.h"

#if defined ( BSHIFT_DLL )

//=======================================================
// Rosenberg
//=======================================================
class CRosenberg : public CScientist
{
public:
	void Precache(void);
	void StartTask(Task_t *pTask);

	void DeclineFollowing(void);

	void Scream(void);

	void PainSound(void);

	void TalkInit(void);
};

LINK_ENTITY_TO_CLASS(monster_rosenberg, CRosenberg);

//---------------------------------------------------------
// Purpose: 
//---------------------------------------------------------
void CRosenberg::DeclineFollowing(void)
{
	Talk(10);
	m_hTalkTarget = m_hEnemy;
	PlaySentence("RO_POK", 2, VOL_NORM, ATTN_NORM);
}

//---------------------------------------------------------
// Purpose: 
//---------------------------------------------------------
void CRosenberg::Scream(void)
{
	if (FOkToSpeak())
	{
		Talk(10);
		m_hTalkTarget = m_hEnemy;
		PlaySentence("RO_SCREAM", RANDOM_FLOAT(3, 6), VOL_NORM, ATTN_NORM);
	}
}

//---------------------------------------------------------
// Purpose: 
//---------------------------------------------------------
void CRosenberg::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_SAY_HEAL:
		//		if ( FOkToSpeak() )
		Talk(2);
		m_hTalkTarget = m_hTargetEnt;
		PlaySentence("RO_HEAL", 2, VOL_NORM, ATTN_IDLE);

		TaskComplete();
		break;

	case TASK_SAY_FEAR:
		if (FOkToSpeak())
		{
			Talk(2);
			m_hTalkTarget = m_hEnemy;
			if (m_hEnemy->IsPlayer())
				PlaySentence("RO_PLFEAR", 5, VOL_NORM, ATTN_NORM);
			else
				PlaySentence("RO_FEAR", 5, VOL_NORM, ATTN_NORM);
		}
		TaskComplete();
		break;

	default:
		CScientist::StartTask(pTask);
		break;
	}
}

//---------------------------------------------------------
// Purpose: 
//---------------------------------------------------------
void CRosenberg::Precache(void)
{
	PRECACHE_MODEL("models/scientist.mdl");
	PRECACHE_SOUND("rosenberg/ro_pain1.wav");
	PRECACHE_SOUND("rosenberg/ro_pain2.wav");
	PRECACHE_SOUND("rosenberg/ro_pain3.wav");
	PRECACHE_SOUND("rosenberg/ro_pain4.wav");
	PRECACHE_SOUND("rosenberg/ro_pain5.wav");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}

//---------------------------------------------------------
// Purpose: Init talk data 
//---------------------------------------------------------
void CRosenberg::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientist will try to talk to friends in this order:

	m_szFriends[0] = "monster_scientist";
	m_szFriends[1] = "monster_sitting_scientist";
	m_szFriends[2] = "monster_barney";

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]		= "RO_ANSWER";
	m_szGrp[TLK_QUESTION]	= "RO_QUESTION";
	m_szGrp[TLK_IDLE]		= "RO_IDLE";
	m_szGrp[TLK_STARE]		= "RO_STARE";
	m_szGrp[TLK_USE]		= "RO_OK";
	m_szGrp[TLK_UNUSE]		= "RO_WAIT";
	m_szGrp[TLK_STOP]		= "RO_STOP";
	m_szGrp[TLK_NOSHOOT]	= "RO_SCARED";
	m_szGrp[TLK_HELLO]		= "RO_HELLO";

	m_szGrp[TLK_PLHURT1]	= "!RO_CUREA";
	m_szGrp[TLK_PLHURT2]	= "!RO_CUREB";
	m_szGrp[TLK_PLHURT3]	= "!RO_CUREC";

	m_szGrp[TLK_PHELLO]		= "RO_PHELLO";
	m_szGrp[TLK_PIDLE]		= "RO_PIDLE";
	m_szGrp[TLK_PQUESTION]	= "RO_PQUEST";
	m_szGrp[TLK_SMELL]		= "RO_SMELL";

	m_szGrp[TLK_WOUND]		= "RO_WOUND";
	m_szGrp[TLK_MORTAL]		= "RO_MORTAL";

	// get voice for head
	switch (pev->body % 3)
	{
	default:
	case HEAD_GLASSES:	m_voicePitch = 105; break;	//glasses
	case HEAD_EINSTEIN: m_voicePitch = 100; break;	//einstein
	case HEAD_LUTHER:	m_voicePitch = 95;  break;	//luther
	case HEAD_SLICK:	m_voicePitch = 100;  break;	//rosenberg /* NOTE: Rosenberg uses HEAD_SLICK as model index. */
	}

	/* Rosenberg uses default voice pitch. */
	m_voicePitch = 100;
}

//---------------------------------------------------------
// Purpose: PainSound
//---------------------------------------------------------
void CRosenberg::PainSound(void)
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 4))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "rosenberg/ro_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "rosenberg/ro_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "rosenberg/ro_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "rosenberg/ro_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "rosenberg/ro_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


#endif // BSHIFT_DLL