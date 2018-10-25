/***
*
*	Copyright (c) nillerusr All rights FUCKED
*
****/


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "player.h"
#include "explode.h"
#include "gamerules.h"
#include "shake.h"
#include "prop.h"

extern int gmsgSayText;

class CBottle : public CProp
{
public:
	void Spawn( void );
	void PropRespawn();
	void EXPORT BottleTouch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS(bottle, CBottle);

void CBottle::Spawn(void)
{

	Precache();

	if( minsH == g_vecZero )
	{
		// default barrel parameters
		minsV = Vector(-10, -10, -17);
		maxsV = Vector(10, 10, 18);
		minsH = Vector(-10, -10, -10);
		maxsH = Vector(10, 10, 13);
	}
	m_flCollideFriction = 0.7;
	m_flFloorFriction = 0.5;
	spawnOrigin = pev->origin;
	spawnAngles = pev->angles;
	m_flSpawnHealth = pev->health;
	if( m_flSpawnHealth <= 0 )
		m_flSpawnHealth = 30;
	if( !m_flRespawnTime )
		m_flRespawnTime = 80;
	pev->dmg = 100;
	PropRespawn();
}


void CBottle::PropRespawn()
{
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector( 0, 0, 0) );
	pev->effects &= ~EF_NODRAW;
	if( pev->spawnflags & SF_PROP_FIXED )
		pev->movetype = MOVETYPE_NONE;
	else
		pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_YES;
	pev->health = m_flSpawnHealth;
	pev->velocity = pev->avelocity = g_vecZero;
	pev->angles = spawnAngles;
	pev->deadflag = DEAD_NO;
	SET_MODEL( ENT(pev), STRING(pev->model) );
	m_oldshape = (PropShape)-1;
	CheckRotate();

	if( !(pev->spawnflags & SF_PROP_FIXED) )
	{
		SetTouch( &CProp::BounceTouch );
		SetUse( &CProp::PropUse );
	}
	else
	{
		SetUse( NULL );
		SetTouch( &CBottle::BottleTouch );
	}

	pev->framerate = 1.0f;
	UTIL_SetOrigin( pev, spawnOrigin );
	m_owner2 = NULL;
	m_attacker = NULL;
}

void CBottle::BottleTouch( CBaseEntity *pOther )
{
	if( !pOther->IsPlayer() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer*)pOther;

	if( !pPlayer->m_bSitted && pPlayer->pev->button & IN_DUCK && pOther->pev->absmin.z >= (pev->origin.z + pev->maxs.z -1) )
	{
		pPlayer->m_bSitted = TRUE;
		ALERT(at_console,"ПРИСЕЛ!\n");
		UTIL_ShowMessage( "Congratulations!\nYou sat down on the bottle", pPlayer );
		char text[256] = "";

		if( pPlayer->pev->netname )
			_snprintf( text, sizeof(text), "   %s sat down on the bottle\n", STRING( pPlayer->pev->netname ) );

		MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
			WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
			WRITE_STRING( text );
		MESSAGE_END();
	}
	//ALERT(at_console,"%f >= %f\n" ,pOther->pev->absmin.z, pev->maxs.z+pev->origin.z-1);
}
