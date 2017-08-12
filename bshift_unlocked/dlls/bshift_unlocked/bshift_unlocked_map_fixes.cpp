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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "scripted.h"

#define IsCurrentMap( _str ) FStrEq(STRING(gpGlobals->mapname), _str)

//=================================
//
// class CWorldModel
//
//=================================

class CWorldModel : public CBaseAnimating
{
public:
	void Precache(void);
	void Spawn(void);
};

//=================================
//
// class CBaSecurity2ShotgunReplacement
//
//=================================
#define NUMBER_OF_SHOTGUNS_TO_REPLACE 4

class CBaSecurity2ShotgunReplacement : public CBaseEntity
{
public:
	int	Save(CSave &save);
	int	Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	void Spawn(void);
	int	ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void EXPORT WaitAndReplaceShotgunInstances(void);


protected:
	BOOL ReplaceShotgunEntityWithWorldModelWithFixedAngles(const char* targetname);

private:
	BOOL m_shotgunReplaced[NUMBER_OF_SHOTGUNS_TO_REPLACE];
};

//=================================
//
// class MapFixBaSecurity2ShotgunAngles
//
//=================================

class MapFixBaSecurity2ShotgunAngles
{
public:
	void ApplyFix();
};

//=================================
//
// class MapFixes
//
//=================================

class MapFixes
{
public:

	void ApplyMapFixBaSecurity2ShotgunAngles();

protected:
	MapFixBaSecurity2ShotgunAngles* GetBaSecurity2ShotgunAnglesSingleton();
};

//=================================
// CWorldModel
//=================================

LINK_ENTITY_TO_CLASS(world_model, CWorldModel);

void CWorldModel::Precache(void)
{
	PRECACHE_MODEL((char*)STRING(pev->model));
}

void CWorldModel::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), STRING(pev->model));
	UTIL_SetOrigin(pev, pev->origin);
	
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
}


//=================================
// CBaSecurity2ShotgunReplacement
//=================================
TYPEDESCRIPTION CBaSecurity2ShotgunReplacement::m_SaveData[] = 
{
	DEFINE_ARRAY(CBaSecurity2ShotgunReplacement, m_shotgunReplaced, FIELD_BOOLEAN, NUMBER_OF_SHOTGUNS_TO_REPLACE),
};
IMPLEMENT_SAVERESTORE(CBaSecurity2ShotgunReplacement, CBaseEntity);

void CBaSecurity2ShotgunReplacement::Spawn(void)
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	SetThink(&CBaSecurity2ShotgunReplacement::WaitAndReplaceShotgunInstances);
	pev->nextthink = gpGlobals->time + 0.1f;

	memset(m_shotgunReplaced, FALSE, sizeof(m_shotgunReplaced));
}

void CBaSecurity2ShotgunReplacement::WaitAndReplaceShotgunInstances(void)
{
	static const char* shotgunTargetNames[] =
	{
		"shotgun1", "shotgun2", "shotgun3", "shotgun4",
	};

	int numberOfInstancesFixed = 0;

	for (int i = 0; i < ARRAYSIZE(shotgunTargetNames); i++)
	{
		if (m_shotgunReplaced[i] == FALSE)
		{
			m_shotgunReplaced[i] = ReplaceShotgunEntityWithWorldModelWithFixedAngles(shotgunTargetNames[i]);
		}

		if (m_shotgunReplaced[i] == TRUE)
			numberOfInstancesFixed++;
	}

	if (numberOfInstancesFixed == NUMBER_OF_SHOTGUNS_TO_REPLACE)
	{
		SetThink(&CBaSecurity2ShotgunReplacement::SUB_Remove);
		pev->nextthink = gpGlobals->time + 0.2f;
	}
	else
		pev->nextthink = gpGlobals->time + 0.5f;
}

BOOL CBaSecurity2ShotgunReplacement::ReplaceShotgunEntityWithWorldModelWithFixedAngles(const char* targetname)
{
	if (targetname == NULL || *targetname == '\0')
		return FALSE;

	CBaseEntity* shotgun = UTIL_FindEntityByTargetname(NULL, targetname);
	CBaseEntity* scriptedSequence = UTIL_FindEntityByString(NULL, "target", targetname);
	if (shotgun == NULL || scriptedSequence == NULL)
		return FALSE;

	edict_t* pevWorldModel = CREATE_NAMED_ENTITY(MAKE_STRING("world_model"));
	if (FNullEnt(pevWorldModel))
		return FALSE;

	CBaseEntity* worldModel = CBaseEntity::Instance(pevWorldModel);
	if (worldModel == NULL)
		return FALSE;

	UTIL_SetOrigin(worldModel->pev, scriptedSequence->pev->origin);
	worldModel->pev->model = ALLOC_STRING("models/w_shotgun.mdl");
	worldModel->Precache();
	DispatchSpawn(worldModel->edict());
	worldModel->pev->angles = Vector(180, 180, 90);

	UTIL_Remove(scriptedSequence);
	UTIL_Remove(shotgun);

	return TRUE;
}

//=================================
// MapFixBaSecurity2ShotgunAngles
//=================================
void MapFixBaSecurity2ShotgunAngles::ApplyFix()
{
	CBaSecurity2ShotgunReplacement* shotgunReplacement = GetClassPtr((CBaSecurity2ShotgunReplacement*)NULL);
	if (shotgunReplacement != NULL)
		shotgunReplacement->Spawn();
}

//=================================
// MapFixes
//=================================

void MapFixes::ApplyMapFixBaSecurity2ShotgunAngles()
{
	GetBaSecurity2ShotgunAnglesSingleton()->ApplyFix();
}

MapFixBaSecurity2ShotgunAngles* MapFixes::GetBaSecurity2ShotgunAnglesSingleton()
{
	static MapFixBaSecurity2ShotgunAngles singleton;
	return &singleton;
}

MapFixes* GetMapFixesSingleton()
{
	static MapFixes singleton;
	return &singleton;
}

//=================================
// MapFixes_ApplyAllPossibleFixes
//=================================

void MapFixes_ApplyAllPossibleFixes()
{
	if (IsCurrentMap("ba_security2"))
	{
		GetMapFixesSingleton()->ApplyMapFixBaSecurity2ShotgunAngles();
	}
}