/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
/*

===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"

//===================grenade


LINK_ENTITY_TO_CLASS( grenade, CGrenade )

// Grenades flagged with this will be triggered when the owner calls detonateSatchelCharges
#define SF_DETONATE		0x0001

//
// Grenade Explode
//
void CGrenade::Explode( Vector vecSrc, Vector vecAim )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -32 ), ignore_monsters, ENT( pev ), & tr );

	Explode( &tr, DMG_BLAST );
}

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CGrenade::Explode( TraceResult *pTrace, int bitsDamageType )
{
	UTIL_ScreenShake( pev->origin, 50.0, 140.0, 1.5, 1200 );

	//float flRndSound;// sound randomizer

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if( pTrace->flFraction != 1.0 )
	{
		pev->origin = pTrace->vecEndPos + ( pTrace->vecPlaneNormal * ( pev->dmg - 24 ) * 0.6 );
	}

	int iContents = UTIL_PointContents( pev->origin );

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		if( iContents != CONTENTS_WATER )
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( ( pev->dmg - 50 ) * .60  ); // scale * 10
		WRITE_BYTE( 15 ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );
	entvars_t *pevOwner;
	if( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamage( pev, pevOwner, pev->dmg, CLASS_NONE, bitsDamageType );

	if( RANDOM_FLOAT( 0, 1 ) < 0.5 )
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	}

	//flRndSound = RANDOM_FLOAT( 0, 1 );

	switch( RANDOM_LONG( 0, 2 ) )
	{
		case 0:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM );
			break;
		case 2:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM );
			break;
	}

	pev->effects |= EF_NODRAW;
	SetThink( &CGrenade::Smoke );
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;

	if( iContents != CONTENTS_WATER )
	{
		int sparkCount = RANDOM_LONG( 0, 3 );
		for( int i = 0; i < sparkCount; i++ )
			Create( "spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL );
	}
}

void CGrenade::Smoke( void )
{
	if( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( (int)( ( pev->dmg - 50 ) * 0.80 ) ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();
	}
	UTIL_Remove( this );
}

void CGrenade::Killed( entvars_t *pevAttacker, int iGib )
{
	Detonate();
}

// Timed grenade, this think is called when time runs out.
void CGrenade::DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time;
}

void CGrenade::PreDetonate( void )
{
	CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, 400, 0.3 );

	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time + 1;
}

void CGrenade::Detonate( void )
{
	TraceResult tr;
	Vector vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector( 0, 0, 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, ENT(pev), &tr );

	Explode( &tr, DMG_BLAST );
}


//
// Contact grenade, explode when it touches something
// 
void CGrenade::ExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT( pev ), &tr );

	Explode( &tr, DMG_BLAST );
}

void CGrenade::DangerSoundThink( void )
{
	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, (int)pev->velocity.Length(), 0.2 );
	pev->nextthink = gpGlobals->time + 0.2;

	if( pev->waterlevel != 0 )
	{
		pev->velocity = pev->velocity * 0.5;
	}
}

void CGrenade::BounceTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if( pOther->edict() == pev->owner )
		return;

	// only do damage if we're moving fairly fast
	if( m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100 )
	{
		entvars_t *pevOwner = VARS( pev->owner );
		if( pevOwner )
		{
			TraceResult tr = UTIL_GetGlobalTrace();
			ClearMultiDamage();
			pOther->TraceAttack( pevOwner, 1, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			ApplyMultiDamage( pev, pevOwner );
		}
		m_flNextAttack = gpGlobals->time + 1.0; // debounce
	}

	Vector vecTestVelocity;
	// pev->avelocity = Vector( 300, 300, 300 );

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity; 
	vecTestVelocity.z *= 0.45;

	if( !m_fRegisteredSound && vecTestVelocity.Length() <= 60 )
	{
		//ALERT( at_console, "Grenade Registered!: %f\n", vecTestVelocity.Length() );

		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving. 
		// go ahead and emit the danger sound.

		// register a radius louder than the explosion, so we make sure everyone gets out of the way
		CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, (int)( pev->dmg / 0.4 ), 0.3 );
		m_fRegisteredSound = TRUE;
	}

	if( pev->flags & FL_ONGROUND )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;

		pev->sequence = RANDOM_LONG( 1, 1 );
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if( pev->framerate > 1.0 )
		pev->framerate = 1;
	else if( pev->framerate < 0.5 )
		pev->framerate = 0;
}

void CGrenade::SlideTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if( pOther->edict() == pev->owner )
		return;

	// pev->avelocity = Vector( 300, 300, 300 );
	if( pev->flags & FL_ONGROUND )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;

		if( pev->velocity.x != 0 || pev->velocity.y != 0 )
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CGrenade::BounceSound( void )
{
	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit1.wav", 0.25, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit2.wav", 0.25, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit3.wav", 0.25, ATTN_NORM );
		break;
	}
}

void CGrenade::TumbleThink( void )
{
	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if( pev->dmgtime - 1 < gpGlobals->time )
	{
		CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin + pev->velocity * ( pev->dmgtime - gpGlobals->time ), 400, 0.1 );
	}

	if( pev->dmgtime <= gpGlobals->time )
	{
		SetThink( &CGrenade::Detonate );
	}
	if( pev->waterlevel != 0 )
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}

void CGrenade::Spawn( void )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "grenade" );

	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_YES;
	pev->health = 10;
	SET_MODEL( ENT( pev ), "models/grenade.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pev->dmg = 100;
	m_fRegisteredSound = FALSE;
}

CGrenade *CGrenade::ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->Spawn();
	// contact grenades arc lower
	pGrenade->pev->gravity = 0.5;// lower gravity since grenade is aerodynamic and engine doesn't know it.
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles( pGrenade->pev->velocity );
	pGrenade->pev->owner = ENT( pevOwner );

	// make monsters afaid of it while in the air
	pGrenade->SetThink( &CGrenade::DangerSoundThink );
	pGrenade->pev->nextthink = gpGlobals->time;

	// Tumble in air
	pGrenade->pev->avelocity = Vector(RANDOM_LONG( -100, -500 ),RANDOM_LONG( -100, 300 ),RANDOM_LONG( -200, 200 ));

	// Explode on contact
	pGrenade->SetTouch( &CGrenade::ExplodeTouch );

	pGrenade->pev->dmg = gSkillData.plrDmgM203Grenade;

	return pGrenade;
}

CGrenade *CGrenade::ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->Spawn();
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	//pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles( pGrenade->pev->velocity );
	pGrenade->pev->owner = ENT( pevOwner );

	pGrenade->SetTouch( &CGrenade::BounceTouch );	// Bounce if touched

	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that 
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed(). 

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink( &CGrenade::TumbleThink );
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if( time < 0.1 )
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector( 0, 0, 0 );
	}

	pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->pev->framerate = 1.0;

	// Tumble through the air
	// pGrenade->pev->avelocity.x = -400;

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8;

	SET_MODEL( ENT( pGrenade->pev ), "models/w_grenade.mdl" );
	pGrenade->pev->dmg = 100;

	return pGrenade;
}

CGrenade *CGrenade::ShootSatchelCharge( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->pev->movetype = MOVETYPE_BOUNCE;
	pGrenade->pev->classname = MAKE_STRING( "grenade" );

	pGrenade->pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pGrenade->pev ), "models/grenade.mdl" );	// Change this to satchel charge model

	UTIL_SetSize( pGrenade->pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pGrenade->pev->dmg = 200;
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = g_vecZero;
	pGrenade->pev->owner = ENT( pevOwner );

	// Detonate in "time" seconds
	pGrenade->SetThink( &CBaseEntity::SUB_DoNothing );
	pGrenade->SetUse( &CGrenade::DetonateUse );
	pGrenade->SetTouch( &CGrenade::SlideTouch );
	pGrenade->pev->spawnflags = SF_DETONATE;

	pGrenade->pev->friction = 0.9;

	return pGrenade;
}

void CGrenade::UseSatchelCharges( entvars_t *pevOwner, SATCHELCODE code )
{
	edict_t *pentFind;
	edict_t *pentOwner;

	if( !pevOwner )
		return;

	CBaseEntity *pOwner = CBaseEntity::Instance( pevOwner );

	pentOwner = pOwner->edict();

	pentFind = FIND_ENTITY_BY_CLASSNAME( NULL, "grenade" );
	while( !FNullEnt( pentFind ) )
	{
		CBaseEntity *pEnt = Instance( pentFind );
		if( pEnt )
		{
			if( FBitSet( pEnt->pev->spawnflags, SF_DETONATE ) && pEnt->pev->owner == pentOwner )
			{
				if( code == SATCHEL_DETONATE )
					pEnt->Use( pOwner, pOwner, USE_ON, 0 );
				else // SATCHEL_RELEASE
					pEnt->pev->owner = NULL;
			}
		}
		pentFind = FIND_ENTITY_BY_CLASSNAME( pentFind, "grenade" );
	}
}


// ______GUNMOD GRENADE______
LINK_ENTITY_TO_CLASS( gm_grenade, CGMGrenade )

void CGMGrenade::BounceTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if( pOther->edict() == pev->owner )
		return;

	float m_flFloorFriction = 0.5;
	// only do damage if we're moving fairly fast
	if( m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100 )
	{
		entvars_t *pevOwner = VARS( pev->owner );
		if( pevOwner )
		{
			TraceResult tr = UTIL_GetGlobalTrace();
			ClearMultiDamage();
			pOther->TraceAttack( pevOwner, 1, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			ApplyMultiDamage( pev, pevOwner );
		}
		m_flNextAttack = gpGlobals->time + 1.0; // debounce
	}

	Vector vecTestVelocity;
	// pev->avelocity = Vector( 300, 300, 300 );

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity; 
	vecTestVelocity.z *= 0.45;

	pev->velocity = pev->velocity + pOther->pev->velocity;
	float dp = cos(M_PI / 180 * UTIL_AngleDiff(UTIL_VecToAngles(pev->velocity).y, pev->angles.y));
	if (pev->flags & FL_ONGROUND || fabs(pev->velocity.z) < 40)
	{
		pev->velocity.x *= fabs(dp) * 0.8 + 0.2;
		pev->velocity.y *= fabs(dp) * 0.8 + 0.2;
		pev->velocity.z -= 20;
		pev->avelocity.x = -dp*pev->velocity.Length()* 1.5;
		pev->avelocity.y = 0;
		pev->avelocity.z = 0;
		pev->angles.z += UTIL_AngleDiff(90, pev->angles.z) * 0.7;
	}
	else
	{
		pev->velocity.z *= 0.3;
		pev->velocity.y *= 0.7;
		pev->velocity.x *= 0.7;
	}
		//pev->avelocity.z = pev->avelocity.z*0.5 + RANDOM_FLOAT ( 1, -1 );
	BounceSound();
	pev->framerate = pev->velocity.Length() / 200.0;
	if( pev->framerate > 1.0 )
		pev->framerate = 1;
	else if( pev->framerate < 0.2 )
	{
		SetThink(&CGMGrenade::AngleThink);
		pev->nextthink = gpGlobals->time + 0.1;
		if( pev->angles.z == 0 || pev->angles.z == 90 )
			pev->framerate = 0;
		else
			pev->framerate = 0.2;
	}
}

void CGMGrenade::TumbleThink( void )
{
	if(CheckTimer())
		return;
	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if( pev->waterlevel != 0 )
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}

void CGMGrenade::Spawn( void )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "gm_grenade" );

	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/grenade.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pev->dmg = 100;
	pev->health = 15;
	pev->takedamage = DAMAGE_YES;
	m_fRegisteredSound = FALSE;
	SetThink( &CGMGrenade::TumbleThink );
	pev->nextthink = gpGlobals->time;


}

CGMGrenade *CGMGrenade::ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngVel ,float time )
{
	CGMGrenade *pGrenade = GetClassPtr( (CGMGrenade *)NULL );
	pGrenade->Spawn();
	PRECACHE_MODEL( "sprites/glow02.spr" );

	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	//pGrenade->pev->avelocity = vecAngVel;
	pGrenade->pev->angles = UTIL_VecToAngles( pGrenade->pev->velocity );
	int m_iTrail = PRECACHE_MODEL( "sprites/smoke.spr" );

/*
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT((pGrenade->entindex() & 0x0FFF) | (( 1 & 0xF) << 12 ));	// entity
		WRITE_SHORT( m_iTrail );	// model ( entityIndex & 0x0FFF ) | ( ( pev->sequence & 0xF000 ) << 12 ); 
		WRITE_BYTE( 3 ); // life
		WRITE_BYTE( 2 );  // width
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 0 );   // r, g, b
		WRITE_BYTE( 0 );   // r, g, b
		WRITE_BYTE( 150 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
*/
	pGrenade->pev->owner = ENT( pevOwner );
	pGrenade->SetTouch( &CGMGrenade::BounceTouch );	// Bounce if touched

	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that 
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed(). 

	pGrenade->pev->dmgtime = gpGlobals->time + time;

	if( time < 0.1 )
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector( 0, 0, 0 );
	}

	pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->pev->framerate = 1.0;

	// Tumble through the air
	// pGrenade->pev->avelocity.x = -400;

	pGrenade->pev->friction = 0.8;
	pGrenade->pev->gravity = 1;
	SET_MODEL( ENT( pGrenade->pev ), "models/w_grenade.mdl" );
	pGrenade->pev->dmg = 100;

	return pGrenade;
}


int CGMGrenade::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	Vector r = (pevInflictor->origin - pev->origin);
	pev->health -= flDamage;
	pev->velocity = r * flDamage / -7;
	pev->avelocity.x = pev->avelocity.x*0.5 + RANDOM_FLOAT(100, -100);
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CGMGrenade::AngleThink( void )
{
	if( CheckTimer() )
		return;
	pev->angles.z += UTIL_AngleDiff(90, pev->angles.z) * 0.7;
	if (fabs(UTIL_AngleDiff(90, pev->angles.z)) > 0.1)
		pev->nextthink = gpGlobals->time + 0.1;
	else
		pev->nextthink = pev->dmgtime;

	//ALERT( at_console, "AngleThink: %f %f %f\n", pev->angles.x, pev->angles.y, pev->angles.z );
	pev->avelocity.y = pev->avelocity.z = 0;

	pev->angles.x = UTIL_AngleMod( pev->angles.x );
	pev->angles.y = UTIL_AngleMod( pev->angles.y );
	pev->angles.z = UTIL_AngleMod( pev->angles.z );

}
BOOL CGMGrenade::CheckTimer()
{
	if( pev->dmgtime - 1 < gpGlobals->time )
	{
		CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin + pev->velocity * ( pev->dmgtime - gpGlobals->time ), 400, 0.1 );
	}

	if( pev->dmgtime <= gpGlobals->time )
	{
		Detonate();
		return true;
	}
	return false;
}


//======================end grenade
