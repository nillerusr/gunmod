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
/*

===== generic grenade.cpp ========================================================

*/
#ifndef PROP_H
#define PROP_H

#define SF_PROP_RESPAWN		8 // enable autorespawn
#define SF_PROP_BREAKABLE	16 // enable break/explode
#define SF_PROP_FIXED		32 // don't move untill touch
#define SF_PROP_TOLCHOK		64 // Black toilet of death

typedef enum { expRandom, expDirected} Explosions;
typedef enum { matGlass = 0, matWood, matMetal, matFlesh, matCinderBlock, matCeilingTile, matComputer, matUnbreakableGlass, matRocks, matNone, matLastMaterial } Materials;

// round() has problems, so just implement it here
static inline int myround(float f)
{
	return (int)(f + 0.5);
}

//===================grenade

enum PropShape
{
	SHAPE_CYL_H = 0,
	SHAPE_CYL_V,
	SHAPE_BOX,
	SHAPE_GENERIC,
	SHAPE_SPHERE,
	SHAPE_NOROTATE
};

class CProp : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache();

	void EXPORT BounceTouch(CBaseEntity *pOther);
	void EXPORT TolchokTouch(CBaseEntity *pOther);
	//void EXPORT SlideTouch(CBaseEntity *pOther);
	virtual void PropUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void Force(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual int	ObjectCaps(void) { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE; }
	virtual void BounceSound(void);
	virtual int	BloodColor(void) { return DONT_BLEED; }
	virtual void Killed(entvars_t *pevAttacker, int iGib);

	int		Save( CSave &save );
	int		Restore( CRestore &restore );

	virtual float TouchGravGun( CBaseEntity *attacker, int stage )
	{
		float speed = 2500;
		if( pev->deadflag || (pev->spawnflags & SF_PROP_FIXED))
			return 0;
		pev->movetype = MOVETYPE_BOUNCE;
		if(stage)
		{
			pev->nextthink = gpGlobals->time + m_flRespawnTime;
			SetThink( &CProp::RespawnThink );
		}
		if( stage == 2 )
		{
			UTIL_MakeVectors( attacker->pev->v_angle + attacker->pev->punchangle);
			Vector atarget = UTIL_VecToAngles(gpGlobals->v_forward);
			pev->angles.x = UTIL_AngleMod(pev->angles.x);
			pev->angles.y = UTIL_AngleMod(pev->angles.y);
			pev->angles.z = UTIL_AngleMod(pev->angles.z);
			atarget.x = UTIL_AngleMod(atarget.x);
			atarget.y = UTIL_AngleMod(atarget.y);
			atarget.z = UTIL_AngleMod(atarget.z);
			pev->avelocity.x = UTIL_AngleDiff(atarget.x, pev->angles.x) * 10;
			pev->avelocity.y = UTIL_AngleDiff(atarget.y, pev->angles.y) * 10;
			pev->avelocity.z = UTIL_AngleDiff(atarget.z, pev->angles.z) * 10;
		}
		if( stage == 3 )
		{
			pev->avelocity.y = pev->avelocity.y*1.5 + RANDOM_FLOAT(100, -100);
			pev->avelocity.x = pev->avelocity.x*1.5 + RANDOM_FLOAT(100, -100);
		}
		if( !m_attacker || m_attacker == this )
		{
			m_owner2 = attacker;
			m_attacker = attacker;
			return speed;
		}
		if( !m_owner2 && m_attacker && ( pev->velocity.Length() < 400) )
			m_attacker = attacker;
			return speed;
		if( ( stage == 2 ) && ( m_attacker == attacker ) )
		{
			m_owner2 = attacker;

			return speed;
		}
		return 0;
	}
	void CheckRotate();
	void EXPORT RespawnThink();
	void EXPORT AngleThink();
	void EXPORT DeployThink();
	void EXPORT DieThink();
	void DamageSound( void );
	void PropRespawn();
	void KeyValue( KeyValueData* pkvd);

	static const char *pSoundsWood[];
	static const char *pSoundsFlesh[];
	static const char *pSoundsGlass[];
	static const char *pSoundsMetal[];
	static const char *pSoundsConcrete[];
	static const char *pSpawnObjects[];

	inline BOOL		Explodable( void ) { return ExplosionMagnitude() > 0; }
	inline int		ExplosionMagnitude( void ) { return pev->impulse; }
	inline void		ExplosionSetMagnitude( int magnitude ) { pev->impulse = magnitude; }


	static void MaterialSoundPrecache( Materials precacheMaterial );
	static void MaterialSoundRandom( edict_t *pEdict, Materials soundMaterial, float volume );
	static const char **MaterialSoundList( Materials precacheMaterial, int &soundCount );
	void EXPORT		Die( void );

	float m_flFloorFriction;
	float m_flCollideFriction;

	// hull sizes
	Vector minsH, maxsH;
	Vector minsV, maxsV;

	// spawn backup;
	Vector spawnOrigin;
	Vector spawnAngles;

	EHANDLE m_owner2;
	EHANDLE m_attacker;
	float m_flNextAttack;
	float m_flRespawnTime;
	PropShape m_shape;
	PropShape m_oldshape;
	EHANDLE m_pHolstered;
	float m_flSpawnHealth;
	int			m_idShard;
	float		m_angle;
	int			m_iszGibModel;
	Materials	m_Material;
	Explosions	m_Explosion;
	int m_iaCustomAnglesX[10];
	int m_iaCustomAnglesZ[10];
	float m_flTouchTimer;
	int m_iTouchCounter;

	float m_flLastGravgun;
	static TYPEDESCRIPTION m_SaveData[];
};
#endif
