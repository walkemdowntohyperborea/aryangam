#pragma once

#include "../../../SDK/SDK.h"

class CAnnouncer
{
private:
	bool IsEventProperForKillCounter(const char* sWeaponName);
	void Reset();

	int iKillCounter = 0;
	int iKillstreakCounter = 0;
	float flLastKillTime = 0.0f;
public:
	void Event(IGameEvent* pEvent, uint32_t uHash);
};

ADD_FEATURE(CAnnouncer, Announcer);