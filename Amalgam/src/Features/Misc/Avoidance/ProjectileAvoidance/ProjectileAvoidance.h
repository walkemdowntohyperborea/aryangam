#pragma once
#include "../../../../SDK/SDK.h"

class CProjectileAvoidance
{
public:
	void Run(CTFPlayer* pLocal, CUserCmd* pCmd);

private:
	float updateIn = 0.0f;
	float optimal = 0.0f;
	std::vector<std::tuple<Vec3, Vec3, float>> vPath = {};
};

ADD_FEATURE(CProjectileAvoidance, ProjectileAvoidance)