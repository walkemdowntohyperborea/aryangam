#include "Avoidance.h"

#include "ProjectileAvoidance/ProjectileAvoidance.h"
#include "SniperAvoidance/SniperAvoidance.h"

bool CAvoidance::ShouldRun(CTFPlayer* pLocal)
{
	if (!pLocal || !pLocal->IsAlive() || pLocal->IsInvulnerable() || pLocal->IsAGhost())
		return false;

	return true;
}

void CAvoidance::Run(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!ShouldRun(pLocal))
		return;

	F::ProjectileAvoidance.Run(pLocal, pCmd);
	F::SniperAvoidance.Run(pLocal, pCmd);
}