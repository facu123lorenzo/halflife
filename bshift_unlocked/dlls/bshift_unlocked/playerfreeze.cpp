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
#include "player.h"

class CTriggerPlayerFreeze : public CPointEntity
{
public:
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn(void);
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

protected:
	CBasePlayer* FindPlayer();

private:

	BOOL m_playerFrozen;
};

LINK_ENTITY_TO_CLASS(trigger_playerfreeze, CTriggerPlayerFreeze);

TYPEDESCRIPTION	CTriggerPlayerFreeze::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerPlayerFreeze, m_playerFrozen, FIELD_BOOLEAN),
};
IMPLEMENT_SAVERESTORE(CTriggerPlayerFreeze, CPointEntity);


void CTriggerPlayerFreeze::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	m_playerFrozen = FALSE;
}

void CTriggerPlayerFreeze::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBasePlayer* player = FindPlayer();

	if (player != NULL)
	{
		if (!m_playerFrozen)
			player->EnableControl(FALSE);
		else
			player->EnableControl(TRUE);

		m_playerFrozen = !m_playerFrozen;
	}
}

CBasePlayer* CTriggerPlayerFreeze::FindPlayer()
{
	return dynamic_cast<CBasePlayer*>(
		UTIL_FindEntityByClassname(NULL, "player"));
}