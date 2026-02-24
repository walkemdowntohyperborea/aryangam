#pragma once

 // Fast timer for high-frequency checks
class FastTimer
{
private:
	float m_flLast = 0.f;
	float m_flInterval = 0.f;
	bool m_bCached = false;

public:
	FastTimer() = default;

	bool CheckCached(float flS);
	bool Check(float flS) const;
	void Update();
	bool Run(float flS);
};

class Timer
{
public:
	Timer();
	bool Check(float flS) const;
	bool Run(float flS);
	void Update();

	float m_flLast = 0.f;
};