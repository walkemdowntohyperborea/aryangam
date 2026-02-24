#include "../SDK/SDK.h"
#include "../Features/CritHack/CritHack.h"

MAKE_SIGNATURE(CTFPlayerShared_InCond, "client.dll", "48 89 5C 24 ? 57 48 83 EC ? 8B DA 48 8B F9 83 FA ? 7D", 0x0);
MAKE_SIGNATURE(CTFPlayer_ShouldDraw_InCond_Call, "client.dll", "84 C0 74 ? 32 C0 48 8B 74 24", 0x0);
MAKE_SIGNATURE(CTFWearable_ShouldDraw_InCond_Call, "client.dll", "84 C0 0F 85 ? ? ? ? 41 BF", 0x0);
MAKE_SIGNATURE(CHudScope_ShouldDraw_InCond_Call, "client.dll", "84 C0 74 ? 48 8B CB E8 ? ? ? ? 48 85 C0 74 ? 48 8B CB E8 ? ? ? ? 48 8B C8 48 8B 10 FF 92 ? ? ? ? 83 F8 ? 0F 94 C0", 0x0);
MAKE_SIGNATURE(CTFPlayer_CreateMove_InCondTaunt_Call, "client.dll", "84 C0 75 ? BA ? ? ? ? 48 8D 8E ? ? ? ? E8 ? ? ? ? 84 C0 75 ? 45 32 FF", 0x0);
MAKE_SIGNATURE(CTFPlayer_CreateMove_InCondKart_Call, "client.dll", "84 C0 74 ? 4C 8B C3", 0x0);
MAKE_SIGNATURE(CTFInput_ApplyMouse_InCond_Call, "client.dll", "84 C0 74 ? F3 0F 10 9B", 0x0);

MAKE_SIGNATURE(CTFConditionList_InCond, "client.dll", "8B 41 ? 0F A3 D0 0F 92 C0 C3 CC CC CC CC CC CC 40 53", 0x0);
MAKE_SIGNATURE(CTFPlayerShared_UpdateCritBoostEffect_Call, "client.dll", "45 33 FF 84 C0 0F 85 ? ? ? ? 8B 86", 0x0);
MAKE_SIGNATURE(CTFPlayerShared_IsCritBoosted, "client.dll", "48 89 5C 24 ? 57 48 83 EC ? 48 8B D9 0F 29 7C 24", 0x0);
MAKE_SIGNATURE(CTFPlayerShared_IsCritBoosted_CProxyModelGlowColor_OnBind_Call, "client.dll", "48 8B CF 84 C0 74 ? BA", 0x0);
MAKE_SIGNATURE(CTFPlayerShared_UpdateCritBoostEffect, "client.dll", "48 8B C4 48 89 58 ? 55 56 41 57", 0x0);

MAKE_HOOK(CTFPlayerShared_InCond, S::CTFPlayerShared_InCond(), bool,
	void* rcx, ETFCond nCond)
{
	DEBUG_RETURN(CTFPlayerShared_InCond, rcx, nCond);

	const auto dwZoomPlayer = S::CTFPlayer_ShouldDraw_InCond_Call();
	const auto dwZoomWearable = S::CTFWearable_ShouldDraw_InCond_Call();
	const auto dwZoomHudScope = S::CHudScope_ShouldDraw_InCond_Call();
	const auto dwTaunt = S::CTFPlayer_CreateMove_InCondTaunt_Call();
	const auto dwKart1 = S::CTFPlayer_CreateMove_InCondKart_Call();
	const auto dwKart2 = S::CTFInput_ApplyMouse_InCond_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	auto GetOuter = [&rcx]() -> CBaseEntity*
		{
			static const auto iShared = U::NetVars.GetNetVar("CTFPlayer", "m_Shared");
			static const auto iBombHeadStage = U::NetVars.GetNetVar("CTFPlayer", "m_nHalloweenBombHeadStage");
			static const auto iOffset = iBombHeadStage - iShared + 0x4;
			return *reinterpret_cast<CBaseEntity**>(uintptr_t(rcx) + iOffset);
		};

	switch (nCond)
	{
	case TF_COND_ZOOMED:
		if (dwRetAddr == dwZoomPlayer || dwRetAddr == dwZoomWearable || Vars::Visuals::Removals::Scope.Value && dwRetAddr == dwZoomHudScope)
			return false;
		break;
	case TF_COND_DISGUISED:
		if (Vars::Visuals::Removals::Disguises.Value && H::Entities.GetLocal() != GetOuter())
			return false;
		break;
	case TF_COND_TAUNTING:
		if (Vars::Misc::Automation::TauntControl.Value && dwRetAddr == dwTaunt)
			return false;
		if (Vars::Visuals::Removals::Taunts.Value && H::Entities.GetLocal() != GetOuter())
			return false;
		break;
	case TF_COND_HALLOWEEN_KART:
		if (Vars::Misc::Automation::KartControl.Value && (dwRetAddr == dwKart1 || dwRetAddr == dwKart2))
			return false;
		break;
	case TF_COND_FREEZE_INPUT:
		if (!CALL_ORIGINAL(rcx, TF_COND_HALLOWEEN_KART) || Vars::Misc::Automation::KartControl.Value)
			return false;
	}

	return CALL_ORIGINAL(rcx, nCond);
}

enum ECritBoostUpdateType { kCritBoost_Ignore, kCritBoost_ForceRefresh };
MAKE_HOOK(CTFPlayerShared_IsCritBoosted, S::CTFPlayerShared_IsCritBoosted(), bool, 
	void* rcx)
{
	DEBUG_RETURN(CTFPlayerShared_IsCritBoosted, rcx);

	static bool bPrevForceState = false;

	const auto pLocal = H::Entities.GetLocal();
	const auto pWeapon = H::Entities.GetWeapon();
	if (!pLocal || !pWeapon || pLocal->IsAGhost())
		return CALL_ORIGINAL(rcx);

	const int iSlot = pWeapon->GetSlot();
	const bool bIsStreamingCrits = pWeapon->m_flCritTime() > TICKS_TO_TIME(pLocal->m_nTickBase());

	const bool bPressed = F::CritHack.IsForcingCrits(G::CurrentUserCmd) || bIsStreamingCrits;
	bool bNowForce = Vars::CritHack::CritVisualEffects.Value && bPressed;
	const bool bCanApplyEffect = !pLocal->IsCritBoosted() && !pLocal->deadflag()
		&& (F::CritHack.WeaponCanCrit(pWeapon) 
		&& !(F::CritHack.IsCritBanned() && iSlot != SLOT_MELEE)
			&& F::CritHack.GetAvailableCrits() > 0 && U::ConVars.FindVar("tf_weapon_criticals")->GetInt())
		|| bIsStreamingCrits;

	const auto pLocalShared = pLocal->m_Shared();
	if (bNowForce && bCanApplyEffect && !bPrevForceState)
	{
		S::CTFPlayerShared_UpdateCritBoostEffect.Call<void>(pLocalShared, kCritBoost_ForceRefresh);
		bPrevForceState = true;
	}
	else if (bPrevForceState && (!bNowForce || !bCanApplyEffect))
	{
		S::CTFPlayerShared_UpdateCritBoostEffect.Call<void>(pLocalShared, kCritBoost_ForceRefresh);
		bPrevForceState = false;
	}
	if (!bNowForce || !bCanApplyEffect)
		return CALL_ORIGINAL(rcx);

	const auto dwRetAddr = uintptr_t(_ReturnAddress());
	static const auto dwCall = S::CTFPlayerShared_IsCritBoosted_CProxyModelGlowColor_OnBind_Call();
	if (dwRetAddr == dwCall)
	{
		const auto pShared = reinterpret_cast<CTFPlayer*>(rcx);
		if (pShared && pShared == pLocalShared)
			return true;
	}

	return CALL_ORIGINAL(rcx);
}

MAKE_HOOK(CTFConditionList_InCond, S::CTFConditionList_InCond(), bool,
	void* rcx, ETFCond type)
{
	DEBUG_RETURN(CTFConditionList_InCond, rcx, type);

	if (!Vars::CritHack::CritVisualEffects.Value)
		return CALL_ORIGINAL(rcx, type);

	const auto dwRetAddr = uintptr_t(_ReturnAddress());
	static const auto dwWeapon = S::CTFPlayerShared_UpdateCritBoostEffect_Call();
	if (dwRetAddr == dwWeapon)
	{
		const auto pLocal = H::Entities.GetLocal();
		if (!pLocal || pLocal && (uintptr_t)rcx != ((uintptr_t)pLocal->m_Shared() + 0x100)) // shared m_ConditionList (find by printing rcx)
			return CALL_ORIGINAL(rcx, type);

		const auto pWeapon = pLocal->m_hActiveWeapon()->As<CTFWeaponBase>();
		if (!pWeapon)
			return CALL_ORIGINAL(rcx, type);

		const int iSlot = pWeapon->GetSlot();
		const bool bPressed = F::CritHack.IsForcingCrits(G::CurrentUserCmd) || pWeapon->m_flCritTime() > TICKS_TO_TIME(pLocal->m_nTickBase());

		if (!bPressed || F::CritHack.IsCritBanned() && iSlot != SLOT_MELEE
			|| F::CritHack.GetAvailableCrits() <= 0 || !F::CritHack.WeaponCanCrit(pWeapon)
			|| pLocal->IsAGhost() || pLocal->deadflag()
			|| pLocal->IsCritBoosted() || !U::ConVars.FindVar("tf_weapon_criticals")->GetInt())
		{
			return CALL_ORIGINAL(rcx, type);
		}
			
		return true;
	}
	return CALL_ORIGINAL(rcx, type);
}