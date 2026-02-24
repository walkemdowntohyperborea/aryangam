#pragma once

#include "../../../SDK/SDK.h"

class CStickyJump
{
public:
	void Run(CUserCmd* pCmd);
};

ADD_FEATURE(CStickyJump, StickyJump)