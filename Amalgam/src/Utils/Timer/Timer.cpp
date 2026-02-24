#include "Timer.h"

#include "../../SDK/SDK.h"

bool FastTimer::CheckCached(float flS)
{
	if (m_bCached && m_flInterval == flS)
	{
		float flCurrent = SDK::PlatFloatTime();
		return flCurrent - m_flLast >= flS;
	}

	m_flInterval = flS;
	m_bCached = true;
	return Check(flS);
}

bool FastTimer::Check(float flS) const
{
	float flCurrentTime = SDK::PlatFloatTime();
	return flCurrentTime - m_flLast >= flS;
}

void FastTimer::Update()
{
	m_flLast = SDK::PlatFloatTime();
}

bool FastTimer::Run(float flS)
{
	if (CheckCached(flS))
	{
		Update();
		return true;
	}
	return false;
}

Timer::Timer()
{
	m_flLast = SDK::PlatFloatTime();
}

bool Timer::Check(float flS) const
{
	float flCurrentTime = SDK::PlatFloatTime();
	return flCurrentTime - m_flLast >= flS;
}

bool Timer::Run(float flS)
{
	if (Check(flS))
	{
		Update();
		return true;
	}
	return false;
}

void Timer::Update()
{
	m_flLast = SDK::PlatFloatTime();
}