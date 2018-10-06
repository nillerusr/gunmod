#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gunmod.h"
#include "gravgunmod.h"
#include "player.h"
#include "weapons.h"

cvar_t mp_gunmod = { "mp_gunmod", "1", FCVAR_SERVER };
cvar_t mp_tolchock = { "mp_tolchock", "0", FCVAR_SERVER };
#define W_count 21
#define A_count 10

struct we{
    char *w;
    bool e;
} weaponlist[W_count]={
    {"weapon_crowbar", 1},
    {"weapon_knife", 0},
    {"weapon_gravgun", 0},
    {"weapon_glock", 1},
    {"weapon_python", 0},
    {"weapon_mp5", 0},
    {"weapon_shotgun", 0},
    {"weapon_crossbow", 0},
    {"weapon_ar2", 0},
    {"weapon_m249", 0},
    {"weapon_rpg", 0},
    {"weapon_gauss", 0},
    {"weapon_egon", 0},
    {"weapon_hornetgun", 0},
    {"weapon_displacer", 0},
    {"weapon_handgrenade", 0},
    {"weapon_satchel", 0},
    {"weapon_tripmine", 0},
    {"weapon_snark", 0},
    {"weapon_shockrifle", 0},
    {"weapon_big_cock", 0}
};

struct an{
    char *a;
    int n;
    int max;
} ammolist[A_count]={
    {"AR2", 0, 180},
    {"9mm", 68, _9MM_MAX_CARRY},
    {"rockets", 0, ROCKET_MAX_CARRY},
    {"uranium", 0, URANIUM_MAX_CARRY},
    {"bolts", 0, BOLT_MAX_CARRY},
    {"357", 0, _357_MAX_CARRY},
    {"556", 0, 250},
    {"ARgrenades", 0, M203_GRENADE_MAX_CARRY},
    {"AR2grenades", 0, 3},
    {"buckshot", 0, BUCKSHOT_MAX_CARRY}
};

void GM_RegisterCVars( void )
{
    CVAR_REGISTER( &mp_gunmod );
    CVAR_REGISTER( &mp_tolchock );
    g_engfuncs.pfnAddServerCommand( "mp_allow_weapon", GM_AllowWeapon );
    g_engfuncs.pfnAddServerCommand( "mp_allow_ammo", GM_AllowAmmo );
}


void GM_AllowWeapon( void )
{
    bool exists = FALSE;

    if(CMD_ARGC() < 3)
    {
	ALERT(at_console,"Usage: mp_allow_weapon <weapon> <state>\n");
        for(int i=0;i<W_count;i++)
	    ALERT(at_console,"%s = %d\n",weaponlist[i].w,weaponlist[i].e);
	return;
    }

    for(int i=0;i<W_count;i++)
    {
	if(!strcmp(weaponlist[i].w,CMD_ARGV(1)))
	{
	    weaponlist[i].e = atoi(CMD_ARGV(2));
	    exists = TRUE;
	}
    }
    if( !exists )
	ALERT(at_error,"%s doesn\'t exit\n",CMD_ARGV(1));
}

void GM_AddWeapons( CBasePlayer *pPlayer )
{
    char weap[16];
    for(int i=0;i<W_count;i++)
    {
	if(weaponlist[i].e)
	    pPlayer->GiveNamedItem( weaponlist[i].w );
    }
}

void GM_AllowAmmo( void )
{
    bool exists = FALSE;
    
    if(CMD_ARGC() < 3)
    {
	ALERT(at_console,"Usage: mp_allow_ammo <ammo> <count>\n");
        for(int i=0;i<A_count;i++)
	    ALERT(at_console,"%s = %d\n",ammolist[i].a,ammolist[i].n);
	return;
    }
    
    for(int i=0;i<A_count;i++)
    {
	if(!strcmp(ammolist[i].a,CMD_ARGV(1)))
	{
	    ammolist[i].n = atoi(CMD_ARGV(2));
	    exists = TRUE;
	}
    }
    if( !exists )
	ALERT(at_error,"ammo %s doesn\'t exit\n",CMD_ARGV(1));
}

void GM_AddAmmo( CBasePlayer *pPlayer )
{
    for(int i=0;i<A_count;i++)
    {
	if(ammolist[i].n)
	    pPlayer->GiveAmmo( ammolist[i].n, ammolist[i].a, ammolist[i].max );
    }
}
