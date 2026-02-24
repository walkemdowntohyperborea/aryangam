#pragma once

#include "../../../SDK/SDK.h"

class CAvoidance
{
private:
	bool ShouldRun(CTFPlayer* pLocal);

public:
	void Run(CTFPlayer* pLocal, CUserCmd* pCmd);
};

ADD_FEATURE(CAvoidance, Avoidance)