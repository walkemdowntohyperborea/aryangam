#pragma once
#include "../../../SDK/SDK.h"

class CAntiAim
{
private:
	void FakeShotAngles(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd);

	float EdgeDistance(CTFPlayer* pEntity, float flEdgeRayYaw, float flOffset);
	void RunOverlapping(CTFPlayer* pEntity, CUserCmd* pCmd, float& flRealYaw, bool bFake, CTFWeaponBase* pWeapon, float flEpsilon = 45.f);
	float GetYawOffset(CTFPlayer* pEntity, bool bFake, CUserCmd* pCmd, CTFWeaponBase* pWeapon);
	float GetBaseYaw(CTFPlayer* pLocal, CUserCmd* pCmd, bool bFake);
	float GetYaw(CTFPlayer* pLocal, CUserCmd* pCmd, bool bFake, CTFWeaponBase* pWeapon);

	float GetPitch(float flCurPitch, CUserCmd* pCmd, CTFWeaponBase* pWeapon);
	void MinWalk(CTFPlayer* pLocal, CUserCmd* pCmd);

public:
	bool AntiAimOn();
	bool YawOn();
	bool ShouldRun(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd);

	int GetEdge(CTFPlayer* pEntity, float flEdgeOrigYaw);
	void Run(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd);

	inline int AntiAimTicks() { return 2; }

	Vec2 vFakeAngles = {};
	Vec2 vRealAngles = {};
	std::vector<std::pair<Vec3, Vec3>> vEdgeTrace = {};
};

ADD_FEATURE(CAntiAim, AntiAim);