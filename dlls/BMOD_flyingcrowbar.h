// ---------------------------------------------------------------
// Flying Crowbar Entity. Ver 1.0 as seen in Lambda BubbleMod  
// ---------------------------------------------------------------
// BubbleMod
// 
// AUTHOR
//		  Tyler Lund <halflife@bubblemod.org>
//
// LICENSE																				
//																						  
//		  Permission is granted to anyone to use this  software  for  
//		  any purpose on any computer system, and to redistribute it  
//		  in any way, subject to the following restrictions:			 
//																						  
//		  1. The author is not responsible for the  consequences  of  
//			  use of this software, no matter how awful, even if they  
//			  arise from defects in it.										  
//		  2. The origin of this software must not be misrepresented,  
//			  either by explicit claim or by omission.					  
//		  3. Altered  versions  must  be plainly marked as such, and  
//			  must  not  be  misrepresented  (by  explicit  claim  or  
//			  omission) as being the original software.					 
//		  3a. It would be nice if I got  a  copy  of  your  improved  
//				version  sent to halflife@bubblemod.org. 
//		  4. This notice must not be removed or altered.				  
//																						  
// ---------------------------------------------------------------
// Flying Crowbar Entity

#ifndef C_FLYING_CROWBAR
#define C_FLYING_CROWBAR


class CFlyingCrowbar : public CBaseEntity
{	 
public: 

	void Spawn( void );
	void Precache( void );
	void EXPORT BubbleThink( void );
	void EXPORT SpinTouch( CBaseEntity *pOther );
	CBasePlayer	*m_pPlayer;

	virtual float TouchGravGun( CBaseEntity *attacker, int stage )
	{
		int ang;
		m_hOwner = Instance( attacker->edict() );
		if( stage == 2 )
		{
			UTIL_MakeVectors( attacker->pev->v_angle + attacker->pev->punchangle);
			Vector atarget = UTIL_VecToAngles(gpGlobals->v_forward);
			pev->angles.x = UTIL_AngleMod(pev->angles.x);
			pev->angles.y = UTIL_AngleMod(pev->angles.y);
			pev->angles.z = -90;
			atarget.x = UTIL_AngleMod(atarget.x);
			atarget.y = UTIL_AngleMod(atarget.y);
			atarget.z = UTIL_AngleMod(atarget.z);
			pev->avelocity.x = UTIL_AngleDiff(atarget.x, pev->angles.x) * 10;
			pev->avelocity.y = UTIL_AngleDiff(atarget.y, pev->angles.y) * 10;
			pev->avelocity.z = UTIL_AngleDiff(atarget.z, pev->angles.z) * 10;
			SetThink( NULL );
			pev->dmg = 0;
		}
		if( stage == 3 )
		{
			ang = RANDOM_LONG(-500,-1000);
			pev->avelocity.x = ang;
			pev->dmg -= int(ang/10);
			SetThink( &CFlyingCrowbar::BubbleThink );
			pev->nextthink = gpGlobals->time + 0.25;
		}
		return -(ang/2);
	}
private:
 	EHANDLE m_hOwner;		  // Original owner is stored here so we can
									 // allow the crowbar to hit the user.
};

#endif

