#include "../Features/Players/PlayerUtils.h"
#include "../Features/Spectate/Spectate.h"
#include "../Features/Visuals/Groups/Groups.h"

MAKE_SIGNATURE(CSniperDot_ClientThink, "client.dll", "40 57 48 83 EC ? 48 8B F9 48 8B 0D ? ? ? ? E8 ? ? ? ? 84 C0", 0x0);
MAKE_SIGNATURE(CNewParticleEffect_SetControlPoint, "client.dll", "48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 83 B9 ? ? ? ? ? 49 8B F0", 0x0);

MAKE_HOOK(CSniperDot_ClientThink, S::CSniperDot_ClientThink(), void,
	void* rcx)
{
	DEBUG_RETURN(CSniperDot_ClientThink, rcx);

	const auto pGameRules = I::TFGameRules();
	if (!pGameRules)
		return;

	const auto pDotEntity = (CBaseEntity*)((uintptr_t)rcx - 24);
	if (!F::Groups.GroupsActive() || !pDotEntity)
		return CALL_ORIGINAL(rcx);

	const auto pOwner = pDotEntity->m_hOwnerEntity()->As<CTFPlayer>();
	if (!pOwner || !pOwner->IsAlive() || pOwner->m_iClass() != TF_CLASS_SNIPER)
		return CALL_ORIGINAL(rcx);

	const auto pLocal = H::Entities.GetLocal();
	if (!pLocal || pOwner == pLocal)
		return CALL_ORIGINAL(rcx);

	if(pOwner == pLocal->m_hObserverTarget().Get() && pLocal->m_iObserverMode() == OBS_MODE_FIRSTPERSON)
		return CALL_ORIGINAL(rcx);

	Group_t* pGroup;
	if (!F::Groups.GetGroup(pOwner, pLocal, pGroup, false) || !pGroup->m_bSightlines)
		return CALL_ORIGINAL(rcx);

	const bool bOldMvM = pGameRules->m_bPlayingMannVsMachine();
	const int iOldTeamNum = pOwner->m_iTeamNum();

	pGameRules->m_bPlayingMannVsMachine() = true;
	pOwner->m_iTeamNum() = TF_TEAM_PVE_INVADERS;

	CALL_ORIGINAL(rcx);

	pGameRules->m_bPlayingMannVsMachine() = bOldMvM;
	pOwner->m_iTeamNum() = iOldTeamNum;

	const uintptr_t dwLaserBeamEffect = *reinterpret_cast<uintptr_t*>((uintptr_t)rcx + 1984);
	if (dwLaserBeamEffect)
	{
		// fix up the origin for crouching because it appears like they are standing often
		Vec3 vOrigin = pOwner->m_vecOrigin() + Vec3(0.f, 0.f, ((pOwner->IsDucking() ? 45.f : 75.f) * pOwner->m_flModelScale()));
		S::CNewParticleEffect_SetControlPoint.Call<void>(dwLaserBeamEffect, 1, vOrigin);

		Color_t tColor = F::Groups.GetColor(pOwner, pGroup);
		S::CNewParticleEffect_SetControlPoint.Call<void>(dwLaserBeamEffect, 2, Vec3((float)tColor.r, (float)tColor.g, (float)tColor.b));
	}
}