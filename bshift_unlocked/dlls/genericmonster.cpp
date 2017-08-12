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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 
#if defined ( BSHIFT_UNLOCKED_DLL )
#define	SF_GENERICMONSTER_TURN_HEAD_WHEN_SPEAKING	8

#define TALKRANGE_MIN 500.0	
#define TLK_STARE_DIST	128

#define HEAD_TURN_RATE 25.0f
#endif // defined ( BSHIFT_UNLOCKED_DLL )

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGenericMonster : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask ( void );

#if defined ( BSHIFT_UNLOCKED_DLL )
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void RunAI(void);
	void PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener );

protected:

	virtual void UpdateLookat();
	BOOL ShouldUpdateLookat();
	void TurnHeadTowardListener();
	void RestoreHeadYaw();
	void StopTalking();
	BOOL IsTalking();
	BOOL CanTurnHead() { return (pev->spawnflags & SF_GENERICMONSTER_TURN_HEAD_WHEN_SPEAKING) != 0; }
	float IdleHeadTurn(Vector &vecFriend);

private:
	float m_flStopTalkTime;
	EHANDLE	m_hTalkTarget;
	float m_headYaw;
#endif // defined ( BSHIFT_UNLOCKED_DLL )
};
LINK_ENTITY_TO_CLASS( monster_generic, CGenericMonster );

#if defined ( BSHIFT_UNLOCKED_DLL )
TYPEDESCRIPTION	CGenericMonster::m_SaveData[] =
{
	DEFINE_FIELD(CGenericMonster, m_flStopTalkTime, FIELD_TIME ),
	DEFINE_FIELD(CGenericMonster, m_hTalkTarget, FIELD_EHANDLE),
	DEFINE_FIELD(CGenericMonster, m_headYaw, FIELD_FLOAT),
};
IMPLEMENT_SAVERESTORE(CGenericMonster, CBaseMonster);
#endif // defined ( BSHIFT_UNLOCKED_DLL )

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGenericMonster :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGenericMonster :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
	default:
		ys = 90;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericMonster :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CGenericMonster :: ISoundMask ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CGenericMonster :: Spawn()
{
	Precache();

	SET_MODEL( ENT(pev), STRING(pev->model) );

/*
	if ( FStrEq( STRING(pev->model), "models/player.mdl" ) )
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
*/

	if ( FStrEq( STRING(pev->model), "models/player.mdl" ) || FStrEq( STRING(pev->model), "models/holo.mdl" ) )
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 8;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	if ( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}

#if defined ( BSHIFT_UNLOCKED_DLL )
	m_hTalkTarget = NULL;
	m_headYaw = SetBoneController(0, 0);
	m_flStopTalkTime = 0;
#endif // defined ( BSHIFT_UNLOCKED_DLL )
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGenericMonster :: Precache()
{
	PRECACHE_MODEL( (char *)STRING(pev->model) );
}

#if defined ( BSHIFT_UNLOCKED_DLL )

void CGenericMonster::RunAI(void)
{
	if (m_flStopTalkTime != 0 && m_flStopTalkTime <= gpGlobals->time)
		StopTalking();

	if (CanTurnHead() && ShouldUpdateLookat())
		UpdateLookat();

	CBaseMonster::RunAI();
}

void CGenericMonster::PlayScriptedSentence(const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener)
{
	m_flStopTalkTime = gpGlobals->time + duration;
	m_hTalkTarget = pListener;

	CBaseMonster::PlayScriptedSentence(pszSentence, duration, volume, attenuation, bConcurrent, pListener);
}

void CGenericMonster::UpdateLookat()
{
	if (IsTalking())
		TurnHeadTowardListener();
	else
		RestoreHeadYaw();
}

BOOL CGenericMonster::ShouldUpdateLookat()
{
	if ((m_hTalkTarget != NULL && IsTalking()) || fabs(m_headYaw) > 0)
		return TRUE;

	return FALSE;
}

void CGenericMonster::TurnHeadTowardListener()
{
	if (m_hTalkTarget != NULL)
	{
		float targetYaw = IdleHeadTurn(m_hTalkTarget->pev->origin);

		// Slowly rotate the head toward listener. 
		m_headYaw = UTIL_ApproachAngle(targetYaw, m_headYaw, HEAD_TURN_RATE);

		SetBoneController(0, m_headYaw);
	}
}

void CGenericMonster::RestoreHeadYaw()
{
	if (m_headYaw == 0)
		return;

	// Slowly rotate the head back at it's neutral position. 
	m_headYaw = UTIL_ApproachAngle(0, m_headYaw, HEAD_TURN_RATE);

	SetBoneController(0, m_headYaw);
}

void CGenericMonster::StopTalking()
{
	m_hTalkTarget = NULL;
	m_flStopTalkTime = 0;
}

BOOL CGenericMonster::IsTalking()
{
	if (m_flStopTalkTime > gpGlobals->time)
		return TRUE;

	return FALSE;
}

float CGenericMonster::IdleHeadTurn(Vector &vecFriend)
{
	float yaw = VecToYaw(vecFriend - pev->origin) - pev->angles.y;

	if (yaw > 180) yaw -= 360;
	if (yaw < -180) yaw += 360;

	return yaw;
}

#endif // defined ( BSHIFT_UNLOCKED_DLL )

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
