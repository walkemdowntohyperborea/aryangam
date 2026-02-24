#pragma once
#include "../../../../SDK/SDK.h"

class CSniperAvoidance
{
public:
	void Run(CTFPlayer* pLocal, CUserCmd* pCmd);

private:
	float updateIn = 0.0f;
	float optimal = 0.0f;
};

ADD_FEATURE(CSniperAvoidance, SniperAvoidance)