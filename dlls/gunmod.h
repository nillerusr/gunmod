#ifndef GUNMOD_H
#define GUNMOD_H

extern cvar_t mp_gunmod;
extern cvar_t mp_tolchock;

void GM_RegisterCVars( void );
void GM_AllowWeapon( void );
void GM_AddWeapons( CBasePlayer *pPlayer );
void GM_AllowAmmo( void );
void GM_AddAmmo( CBasePlayer *pPlayer );

#endif
