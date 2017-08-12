/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "decals.h"
#include "shake.h"

#define WARP_DAMAGE_DURATION	0.1f

class CWarpDamage : public CBaseEntity
{
public:
	void Spawn(void);
	void EXPORT WarpDamageTouch(CBaseEntity* pOther);
};

void CWarpDamage::Spawn(void)
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;

	SetTouch(&CWarpDamage::WarpDamageTouch);

	SetThink(&CWarpDamage::SUB_Remove);
	pev->nextthink = gpGlobals->time + pev->dmgtime;
}

void CWarpDamage::WarpDamageTouch(CBaseEntity* pOther)
{
	if (pOther == NULL || !pOther->IsPlayer())
		return;

	pOther->TakeDamage( pev, pev, pOther->pev->health + 1, DMG_ENERGYBEAM | DMG_ALWAYSGIB );
}

#define LIGHTNING_SPRITE "sprites/lgtning.spr"
#define GLOW1_SPRITE "sprites/Fexplo1.spr"
#define GLOW2_SPRITE "sprites/XFlare1.spr"
#define WARP_SOUND1	"debris/beamstart2.wav"
#define WARP_SOUND2	"debris/beamstart7.wav"

class CWarpBall : public CPointEntity
{
	typedef CPointEntity BaseClass;
public:
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn(void);
	void Precache(void);
	void KeyValue( KeyValueData* pkvd );

	void EXPORT EffectUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT EffectThink(void);

protected:
	void StartEffect();
	void SpawnBeam();
	void SpawnSprite1();
	void SpawnSprite2();
	void SpawnShakeEffect();
	void SpawnDamage();
	void PlayWarpSound1();
	void PlayWarpSound2();
private:
	void RemoveWarpEntities();
	CBaseEntity* CreateBeam(const char* spriteName);
	CBaseEntity* CreateSprite(const char* spriteName, BOOL playOnce = TRUE);

	float m_damageDelay;
	int m_radius;

	float m_startTime;
	int m_state;
	EHANDLE m_hBeam;
	EHANDLE m_hSprite1;
	EHANDLE m_hSprite2;
	EHANDLE m_hDamage;
};

LINK_ENTITY_TO_CLASS(env_warpball, CWarpBall);

TYPEDESCRIPTION	CWarpBall::m_SaveData[] =
{
	DEFINE_FIELD(CWarpBall, m_damageDelay, FIELD_FLOAT),
	DEFINE_FIELD(CWarpBall, m_radius, FIELD_INTEGER),
	DEFINE_FIELD(CWarpBall, m_startTime, FIELD_TIME),
	DEFINE_FIELD(CWarpBall, m_state, FIELD_INTEGER),
	DEFINE_FIELD(CWarpBall, m_hBeam, FIELD_EHANDLE),
	DEFINE_FIELD(CWarpBall, m_hSprite1, FIELD_EHANDLE),
	DEFINE_FIELD(CWarpBall, m_hSprite2, FIELD_EHANDLE),
	DEFINE_FIELD(CWarpBall, m_hDamage, FIELD_EHANDLE),
};
IMPLEMENT_SAVERESTORE(CWarpBall, CPointEntity);

void CWarpBall::Spawn(void)
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	m_state = 0;
	m_startTime = 0;
	m_hBeam = NULL;
	m_hSprite1 = NULL;
	m_hSprite2 = NULL;
	m_hDamage = NULL;

	SetThink(NULL);
	SetUse(&CWarpBall::EffectUse);
}

void CWarpBall::Precache(void)
{
	BaseClass::Precache();

	PRECACHE_MODEL(LIGHTNING_SPRITE);
	PRECACHE_MODEL(GLOW1_SPRITE);
	PRECACHE_MODEL(GLOW2_SPRITE);
	PRECACHE_SOUND(WARP_SOUND1);
	PRECACHE_SOUND(WARP_SOUND2);
}

void CWarpBall::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		m_radius = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damage_delay"))
	{
		m_damageDelay = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		BaseClass::KeyValue(pkvd);
}

void CWarpBall::EffectUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	StartEffect();
	SetUse(NULL);
}

void CWarpBall::EffectThink(void)
{
	switch (m_state)
	{
	case 0:
		SpawnSprite1();
		SpawnSprite2();
		SpawnBeam();
		SpawnShakeEffect();
		PlayWarpSound1();
		m_state = 1;
		pev->nextthink = gpGlobals->time + 0.3f;
		break;
	case 1:
		SpawnDamage();
		m_state = 2;
		pev->nextthink = gpGlobals->time + 0.2f;
		break;
	case 2:
		PlayWarpSound2();
		m_state = 3;
		pev->nextthink = gpGlobals->time + 0.5f;
		break;
	case 3:
		RemoveWarpEntities();
		m_state = 4;
		pev->nextthink = gpGlobals->time + 0.2f;
	case 4:
	default:
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time;
		break;
	}
}

void CWarpBall::StartEffect()
{
	m_startTime = gpGlobals->time;

	SetThink(&CWarpBall::EffectThink);
	pev->nextthink = gpGlobals->time;
}

void CWarpBall::SpawnBeam()
{
	m_hBeam = CreateBeam(LIGHTNING_SPRITE);

	if (m_hBeam == NULL)
		return;

	m_hBeam->pev->origin = pev->origin;
	m_hBeam->pev->angles = pev->angles;
	m_hBeam->pev->owner = NULL;
	m_hBeam->pev->renderamt = 150;
	m_hBeam->pev->rendercolor = Vector(0, 255, 0);
	m_hBeam->pev->framerate = 0;
	m_hBeam->pev->frame = 0;
	m_hBeam->pev->renderfx = 0;

	DispatchSpawn(m_hBeam->edict());

	m_hBeam->pev->nextthink = gpGlobals->time;
}

void CWarpBall::SpawnSprite1()
{
	m_hSprite1 = CreateSprite(GLOW1_SPRITE);
	if (m_hSprite1 == NULL)
		return;

	m_hSprite1->pev->origin = pev->origin;
	m_hSprite1->pev->angles = pev->angles;
	m_hSprite1->pev->owner = NULL;
	m_hSprite1->pev->rendermode = kRenderGlow;
	m_hSprite1->pev->renderfx = kRenderFxNoDissipation;
	m_hSprite1->pev->renderamt = 255;
	m_hSprite1->pev->rendercolor = Vector(77, 210, 130);
	m_hSprite1->pev->framerate = 10;
	m_hSprite1->pev->scale = 1.0f;

	DispatchSpawn(m_hSprite1->edict());
}

void CWarpBall::SpawnSprite2()
{
	CBaseEntity* m_hSprite2 = CreateSprite(GLOW2_SPRITE);
	if (m_hSprite2 == NULL)
		return;

	m_hSprite2->pev->origin = pev->origin;
	m_hSprite2->pev->angles = pev->angles;
	m_hSprite2->pev->owner = NULL;
	m_hSprite2->pev->rendermode = kRenderGlow;
	m_hSprite2->pev->renderfx = kRenderFxNoDissipation;
	m_hSprite2->pev->renderamt = 255;
	m_hSprite2->pev->rendercolor = Vector(184, 250, 214);
	m_hSprite2->pev->framerate = 10;
	m_hSprite2->pev->scale = 1.0f; // 0.5

	DispatchSpawn(m_hSprite2->edict());
}

void CWarpBall::SpawnShakeEffect()
{
	UTIL_ScreenShake( pev->origin, 4, 100, 2, 1000);
}
void CWarpBall::SpawnDamage()
{
	m_hDamage = GetClassPtr((CWarpDamage*)NULL);
	if (m_hDamage == NULL)
		return;

	UTIL_SetOrigin(m_hDamage->pev, pev->origin);
	m_hDamage->pev->angles = pev->angles;
	m_hDamage->pev->dmgtime = gpGlobals->time + WARP_DAMAGE_DURATION;
	m_hDamage->Spawn();

	Vector mins = -Vector(
		VEC_HUMAN_HULL_MIN.x + 16, 
		VEC_HUMAN_HULL_MIN.y + 16,
		VEC_HUMAN_HULL_MIN.z);

	Vector maxs = Vector(
		VEC_HUMAN_HULL_MAX.x + 16,
		VEC_HUMAN_HULL_MAX.y + 16,
		VEC_HUMAN_HULL_MAX.z + 16);

	UTIL_SetSize(m_hDamage->pev, mins, maxs);
}

void CWarpBall::PlayWarpSound1()
{
	EMIT_SOUND(ENT(pev), CHAN_AUTO, WARP_SOUND1, 1.0, ATTN_NORM);
}

void CWarpBall::PlayWarpSound2()
{
	EMIT_SOUND(ENT(pev), CHAN_AUTO, WARP_SOUND2, 1.0, ATTN_NORM);
}

void CWarpBall::RemoveWarpEntities()
{
	if (m_hSprite1 != NULL)
	{
		UTIL_Remove(m_hSprite1);
		m_hSprite1 = NULL;
	}

	if (m_hSprite2 != NULL)
	{
		UTIL_Remove(m_hSprite2);
		m_hSprite2 = NULL;
	}

	if (m_hBeam != NULL)
	{
		UTIL_Remove(m_hBeam);
		m_hBeam = NULL;
	}

	if (m_hDamage != NULL)
	{
		UTIL_Remove(m_hDamage);
		m_hDamage = NULL;
	}
}

static void SetEntityKeyValue(CBaseEntity* entity, const char* keyName, const char* value)
{
	static KeyValueData kvd;
	kvd.szKeyName = (char*)keyName;
	kvd.szValue = (char*)value;
	entity->KeyValue(&kvd);
}

CBaseEntity* CWarpBall::CreateBeam(const char* spriteName)
{
	edict_t* pevBeam = CREATE_NAMED_ENTITY(MAKE_STRING("env_beam"));
	if (FNullEnt(pevBeam))
		return NULL;

	CBaseEntity* pBeam = CBaseEntity::Instance(pevBeam);

	if (pBeam == NULL)
		return NULL;

	char stringRadius[8];
	snprintf(stringRadius, ARRAYSIZE(stringRadius), "%d", m_radius);
	SetEntityKeyValue(pBeam, "Radius", stringRadius);

	SetEntityKeyValue(pBeam, "LightningStart", STRING(pev->targetname));
	SetEntityKeyValue(pBeam, "life", ".5");
	SetEntityKeyValue(pBeam, "BoltWidth", "18");
	SetEntityKeyValue(pBeam, "NoiseAmplitude", "65");
	SetEntityKeyValue(pBeam, "texture", spriteName);
	SetEntityKeyValue(pBeam, "TextureScroll", "35");
	SetEntityKeyValue(pBeam, "framestart", "0");
	SetEntityKeyValue(pBeam, "StrikeTime", "-.5");
	SetEntityKeyValue(pBeam, "damage", "0");

	pBeam->pev->spawnflags = SF_BEAM_TOGGLE | SF_BEAM_SPARKEND;

	return pBeam;
}

CBaseEntity* CWarpBall::CreateSprite(const char* spriteName, BOOL playOnce)
{
	edict_t* pevSprite = CREATE_NAMED_ENTITY(MAKE_STRING("env_sprite"));
	if (FNullEnt(pevSprite))
		return NULL;

	CBaseEntity* pSprite = CBaseEntity::Instance(pevSprite);

	if (pSprite == NULL)
		return NULL;

	pSprite->pev->model = MAKE_STRING(spriteName);

	if (playOnce)
		pSprite->pev->spawnflags = SF_SPRITE_ONCE;

	return pSprite;
}