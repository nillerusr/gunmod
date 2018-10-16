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
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "customweapons.h"
#include "unpredictedweapon.h"

class CKnife : public CBasePlayerWeaponU
{
public:
	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 1; }
	void EXPORT SwingAgain(void);
	void EXPORT Smack(void);
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack(void);
	void SecondaryAttack(void);
	int Swing(int fFirst);
	BOOL Deploy(void);
	void Holster(int skiplocal = 0);
	int m_iSwing;
	TraceResult m_trHit;
};

void FindHullIntersection(const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity);

#define	KNIFE_BODYHIT_VOLUME 128
#define	KNIFE_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS(weapon_knife, CKnife);

enum knife_e {
	KNIFE_IDLE1 = 0,
	KNIFE_DRAW,
	KNIFE_HOLSTER,
	KNIFE_ATTACK1,
	KNIFE_ATTACK1MISS,
	KNIFE_ATTACK2,
	KNIFE_ATTACK2HIT,
	KNIFE_ATTACK3,
	KNIFE_ATTACK3HIT,
	KNIFE_IDLE2,
	KNIFE_IDLE3,
	KNIFE_CHARGE,
	KNIFE_STAB,
};


void CKnife::Spawn()
{
	Precache();
	m_iId = WEAPON_KNIFE;
	SET_MODEL(ENT(pev), "models/w_knife.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CKnife::Precache(void)
{
	PRECACHE_MODEL("models/v_knife.mdl");
	PRECACHE_MODEL("models/w_knife.mdl");
	PRECACHE_MODEL("models/p_knife.mdl");
	PRECACHE_SOUND("weapons/knife_hit_flesh1.wav");
	PRECACHE_SOUND("weapons/knife_hit_flesh2.wav");
	PRECACHE_SOUND("weapons/knife_hit_wall1.wav");
	PRECACHE_SOUND("weapons/knife_hit_wall2.wav");
	PRECACHE_SOUND("weapons/knife1.wav");
	PRECACHE_SOUND("weapons/knife2.wav");
	PRECACHE_SOUND("weapons/knife3.wav");
}

int CKnife::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_KNIFE;
	p->iWeight = 0;
	return 1;
}

BOOL CKnife::Deploy()
{
	return DefaultDeploy("models/v_knife.mdl", "models/p_knife.mdl", KNIFE_DRAW, "crowbar");
}

void CKnife::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	SendWeaponAnim(KNIFE_HOLSTER);
}

void CKnife::PrimaryAttack()
{
	if (!Swing(1))
	{
		SetThink(&CKnife::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CKnife::SecondaryAttack( void )
{
	TraceResult tr;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

	if (tr.flFraction >= 1.0)
	{
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT(m_pPlayer->pev), &tr);
		if (tr.flFraction < 1.0)
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
			if (!pHit || pHit->IsBSPModel())
				FindHullIntersection(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict());
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife1.wav", 1, ATTN_NORM); break;
	case 1:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife2.wav", 1, ATTN_NORM); break;
	case 2:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife3.wav", 1, ATTN_NORM); break;
	}
	
	if (tr.flFraction >= 1.0)
		SendWeaponAnim(KNIFE_ATTACK3);
	else
	{
		SendWeaponAnim(KNIFE_ATTACK3);
		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage();

		// do damage
		pEntity->TraceAttack(m_pPlayer->pev, 40, gpGlobals->v_forward, &tr, DMG_CLUB);
		
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				// play thwack or smack sound
				switch (RANDOM_LONG(0, 1))
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_flesh1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_flesh2.wav", 1, ATTN_NORM); break;
				}
				m_pPlayer->m_iWeaponVolume = KNIFE_BODYHIT_VOLUME;
				if (!pEntity->IsAlive())
					return;
				else
					flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2, BULLET_PLAYER_CROWBAR);

			if (g_pGameRules->IsMultiplayer())
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play crowbar strike
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_wall1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_wall2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * KNIFE_WALLHIT_VOLUME;

		SetThink(&CKnife::Smack);
		pev->nextthink = gpGlobals->time + 0.1;
	}
	m_flNextSecondaryAttack = gpGlobals->time + 0.8;

}


void CKnife::Smack()
{
	UTIL_Sparks( m_trHit.vecEndPos );
	DecalGunshot(&m_trHit, BULLET_PLAYER_CROWBAR);
}


void CKnife::SwingAgain(void)
{
	Swing(0);
}


int CKnife::Swing(int fFirst)
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

#ifndef CLIENT_DLL
	if (tr.flFraction >= 1.0)
	{
		UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT(m_pPlayer->pev), &tr);
		if (tr.flFraction < 1.0)
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
			if (!pHit || pHit->IsBSPModel())
				FindHullIntersection(vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict());
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife1.wav", 1, ATTN_NORM); break;
	case 1:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife2.wav", 1, ATTN_NORM); break;
	case 2:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife3.wav", 1, ATTN_NORM); break;
	}
	
	if (tr.flFraction >= 1.0)
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = gpGlobals->time + 0.5;

			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);
		}
		switch (((m_iSwing++) % 2) + 1)
		{
		case 0:
			SendWeaponAnim(KNIFE_ATTACK1MISS); break;
		case 1:
			SendWeaponAnim(KNIFE_ATTACK2); break;
		case 2:
			SendWeaponAnim(KNIFE_ATTACK3); break;
		}
	}
	else
	{
		switch (((m_iSwing++) % 2) + 1)
		{
		case 0:
			SendWeaponAnim(KNIFE_ATTACK1); break;
		case 1:
			SendWeaponAnim(KNIFE_ATTACK2HIT); break;
		case 2:
			SendWeaponAnim(KNIFE_ATTACK3HIT); break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage();

		// do damage
		pEntity->TraceAttack(m_pPlayer->pev, 15, gpGlobals->v_forward, &tr, DMG_CLUB);
		
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				// play thwack or smack sound
				switch (RANDOM_LONG(0, 1))
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_flesh1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_flesh2.wav", 1, ATTN_NORM); break;
				}
				m_pPlayer->m_iWeaponVolume = KNIFE_BODYHIT_VOLUME;
				if (!pEntity->IsAlive())
					return TRUE;
				else
					flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2, BULLET_PLAYER_CROWBAR);

			if (g_pGameRules->IsMultiplayer())
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play crowbar strike
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_wall1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit_wall2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * KNIFE_WALLHIT_VOLUME;
		m_flNextPrimaryAttack = gpGlobals->time + 0.25;

		SetThink(&CKnife::Smack);
		pev->nextthink = gpGlobals->time + 0.2;
	}
	return fDidHit;
}
