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
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "gamerules.h"
#include "baseitemarmor.h"

extern int gmsgItemPickup;

void CBaseItemArmor::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev), GetWorldModel());
	BaseClass::Spawn();
}
void CBaseItemArmor::Precache(void)
{
	PRECACHE_MODEL((char*)GetWorldModel());
	PRECACHE_SOUND((char*)GetPickupSound());
}
BOOL CBaseItemArmor::MyTouch(CBasePlayer *player)
{
	if (CanPlayerPickupArmor(player))
	{
		player->pev->armorvalue += GetArmorValueToGiveToPlayer();
		player->pev->armorvalue = min(player->pev->armorvalue, MAX_NORMAL_BATTERY);

		PlayPickupSound(player);

		MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, player->pev);
		WRITE_STRING(STRING(pev->classname));
		MESSAGE_END();

		return TRUE;
	}
	return FALSE;
}

const char* CBaseItemArmor::GetPickupSound()
{
	return "items/gunpickup2.wav";
}

BOOL CBaseItemArmor::CanPlayerPickupArmor(CBasePlayer *player)
{
	if (player->pev->deadflag != DEAD_NO)
		return FALSE;

	if (player->pev->armorvalue >= MAX_NORMAL_BATTERY)
		return FALSE;

	return TRUE;
}

void CBaseItemArmor::PlayPickupSound(CBasePlayer* toucher)
{
	EMIT_SOUND(toucher->edict(), CHAN_ITEM, GetPickupSound(), 1, ATTN_NORM);
}