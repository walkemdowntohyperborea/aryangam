#pragma once
#include "../../../SDK/SDK.h"

enum EFlameThrowerAirblastFunction
{
	TF_FUNCTION_AIRBLAST_PUSHBACK = 0x01,
	TF_FUNCTION_AIRBLAST_PUT_OUT_TEAMMATES = 0x02,
	TF_FUNCTION_AIRBLAST_REFLECT_PROJECTILES = 0x04,
	TF_FUNCTION_AIRBLAST_PUSHBACK__STUN = 0x08,
	TF_FUNCTION_AIRBLAST_PUSHBACK__VIEW_PUNCH = 0x10,
};

class CAutoAirblast
{
public:
	void Run(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd);
	bool CanAirblastEntity(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CBaseEntity* pEntity, const Vec3& vAngle);
};

ADD_FEATURE(CAutoAirblast, AutoAirblast);