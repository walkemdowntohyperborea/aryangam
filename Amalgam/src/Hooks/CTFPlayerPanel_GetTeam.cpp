#include "../Features/Players/PlayerUtils.h"
#include "../Features/Visuals/Groups/Groups.h"

MAKE_SIGNATURE(CTFPlayerPanel_GetTeam, "client.dll", "8B 91 ? ? ? ? 83 FA ? 74 ? 48 8B 05", 0x0);
MAKE_SIGNATURE(CTFTeamStatusPlayerPanel_Update, "client.dll", "40 56 57 48 83 EC ? 48 83 3D", 0x0);
MAKE_SIGNATURE(VGui_Panel_SetBgColor, "client.dll", "89 91 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC 48 8B 41", 0x0);
MAKE_SIGNATURE(CTFTeamStatusPlayerPanel_Update_GetTeam_Call, "client.dll", "8B 9F ? ? ? ? 40 32 F6", 0x0);
MAKE_SIGNATURE(CTFTeamStatusPlayerPanel_Update_SetBgColor_Call, "client.dll", "48 8B 8F ? ? ? ? 4C 8B 6C 24 ? 48 85 C9 0F 84 ? ? ? ? 40 38 B7", 0x0);
MAKE_SIGNATURE(VGui_Panel_SetFgColor, "client.dll", "89 91 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 83 EC ? 48 8B 01 4C 8B EA 4C 8B F9 FF 90 ? ? ? ? 4C 8B F0 48 85 C0 74 ? 49 8B CD E8 ? ? ? ? 49 63 6E ? 4C 8B E0 48 85 ED 7E ? 33 DB 8B FB 0F 1F 44 00 ? 49 8B 36 49 8B D4 48 03 F7 48 8B 0E E8 ? ? ? ? 85 C0 74 ? 48 FF C3 48 83 C7 ? 48 3B DD 7C ? 4D 8B 46 ? 4D 85 C0 74 ? 49 8B D4 49 8B CF E8 ? ? ? ? 48 8B F0 48 85 F6 74 ? 48 8B 4E ? E8 ? ? ? ? 48 85 C0 74 ? 4C 8B 10 4C 8B CE 4D 8B C5 49 8B D7 48 8B C8 41 FF 52 ? B0 ? EB ? 32 C0 48 8B 5C 24 ? 48 8B 6C 24 ? 48 8B 74 24 ? 48 83 C4 ? 41 5F 41 5E 41 5D 41 5C 5F C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24 ? 57 48 83 EC ? 48 8B D9 4C 8B C1 48 8D 0D ? ? ? ? 8B FA E8 ? ? ? ? 89 7B ? 48 8B 5C 24 ? 48 83 C4 ? 5F C3 CC CC CC CC 40 53 57 41 57 48 83 EC ? 48 8B 1D ? ? ? ? 4C 8B F9 48 8B 01 4C 89 64 24 ? 44 0F B6 E2 4C 89 74 24 ? 48 8B 3B FF 10 45 0F B6 C4 48 8B CB 48 8B D0 FF 97 ? ? ? ? 48 89 6C 24 ? 45 33 F6 48 89 74 24 ? 66 66 0F 1F 84 00 ? ? ? ? 48 8B 1D ? ? ? ? 48 85 DB 74 ? 49 8B 07 49 8B CF 48 8B 3B FF 10 48 8B D0 48 8B CB FF 97 ? ? ? ? 48 8B 1D ? ? ? ? EB ? 33 C0 44 3B F0 7D ? 48 8B 2B E8 ? ? ? ? 49 8B 17 49 8B CF 48 8B F8 FF 12 45 8B C6 48 8B CB 48 8B D0 FF 95 ? ? ? ? 4C 8B C7 48 8B CB 48 8B D0 FF 95 ? ? ? ? 48 85 C0 74 ? 4C 8B 00 41 0F B6 D4 48 8B C8 41 FF 90 ? ? ? ? 41 FF C6 EB ? 4C 8B 74 24 ? 45 84 E4 4C 8B 64 24 ? 48 8B 74 24 ? 48 8B 6C 24 ? 75 ? 49 8B 07 49 8B CF FF 90 ? ? ? ? 48 8B F8 48 85 C0 74 ? 48 8B 10 48 8B C8 FF 92 ? ? ? ? 49 8B 17 49 8B CF 48 8B D8 FF 12 48 3B D8 75 ? 48 8B 07 33 D2 48 8B CF 48 83 C4 ? 41 5F 5F 5B 48 FF 60 ? 48 83 C4 ? 41 5F 5F 5B C3 CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B 1D ? ? ? ? 41 8B F0 48 8B 01 8B EA 48 8B 3B FF 10 44 8B CE 44 8B C5 48 8B D0 48 8B CB 48 8B 47 ? 48 8B 5C 24 ? 48 8B 6C 24 ? 48 8B 74 24 ? 48 83 C4 ? 5F 48 FF E0 CC 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B 1D ? ? ? ? 0F B6 F2 48 8B 01 48 8B 3B FF 10 44 0F B6 C6 48 8B CB 48 8B D0 FF 97 ? ? ? ? 48 8B 0D ? ? ? ? 48 8B 01 48 8B 5C 24 ? 48 8B 74 24 ? 48 83 C4 ? 5F 48 FF A0 ? ? ? ? CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC 48 89 74 24 ? 57 48 83 EC ? 48 8B F1 48 8B FA 48 8B 49 ? 48 85 C9 74 ? 48 85 D2 74 ? 4C 8B CA 48 8B C1 4C 2B C9 66 0F 1F 84 00 ? ? ? ? 44 0F B6 00 42 0F B6 14 08 44 2B C2 75 ? 48 FF C0 85 D2 75 ? 45 85 C0 74 ? E8 ? ? ? ? 48 C7 46 ? ? ? ? ? 48 85 FF 74 ? 48 89 5C 24 ? 48 C7 C3 ? ? ? ? 0F 1F 84 00 ? ? ? ? 48 FF C3 80 3C 1F ? 75 ? FF C3 48 63 CB E8 ? ? ? ? 44 8B C3 48 89 46 ? 48 8B D7 48 8B C8 E8 ? ? ? ? 48 8B 5C 24 ? 48 8B 74 24 ? 48 83 C4 ? 5F C3 CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24", 0x0);
MAKE_SIGNATURE(VGui_Panel_SetFgColor_RetAddr, "client.dll", "48 8B 8F ? ? ? ? 48 8B 01 FF 90 ? ? ? ? 0F 2F 35", 0x0);

static int s_iPlayerIndex;

static inline Color_t GetScoreboardColor(int iIndex)
{
	if (iIndex == I::EngineClient->GetLocalPlayer())
		return Vars::Colors::Local.Value;

	if (auto pEntity = I::ClientEntityList->GetClientEntity(iIndex)->As<CBaseEntity>())
	{
		Group_t* pGroup{};
		if (F::Groups.GroupsActive() && F::Groups.GetGroup(pEntity, pGroup, false))
			return F::Groups.GetColor(pEntity, pGroup);
	}
		
	return { 0,0,0,0 };
}

static inline Color_t GetHealthColor(CBaseEntity* pEntity, Group_t* pGroup, bool bAllowOverheal)
{
	if (pEntity->IsPlayer())
	{
		const auto pPlayer = pEntity->As<CTFPlayer>();

		const float flHealth = pPlayer->m_iHealth(), flMaxHealth = pPlayer->GetMaxHealth();
		const float health = bAllowOverheal ? std::max(flHealth / flMaxHealth, 0.f) : std::clamp(flHealth / flMaxHealth, 0.f, 1.f);

		if (health > 1.0f)
			return pGroup->m_tHealthColorOverheal;
		else if (health < 0.5f)
			return pGroup->m_tHealthColorLow.Lerp(pGroup->m_tHealthColorMid, health * 2.0f);
		else
			return pGroup->m_tHealthColorMid.Lerp(pGroup->m_tHealthColorHigh, (health - 0.5f) * 2.0f);
	}

	return Color_t(255, 255, 255, 255);
}

MAKE_HOOK(CTFPlayerPanel_GetTeam, S::CTFPlayerPanel_GetTeam(), int,
	void* rcx)
{
	DEBUG_RETURN(CTFPlayerPanel_GetTeam, rcx);

	static const auto dwDesired = S::CTFTeamStatusPlayerPanel_Update_GetTeam_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	if (Vars::Visuals::UI::RevealScoreboard.Value && dwRetAddr == dwDesired)
	{
		if (auto pLocal = H::Entities.GetLocal())
			return pLocal->m_iTeamNum();
	}

	return CALL_ORIGINAL(rcx);
}

MAKE_HOOK(CTFTeamStatusPlayerPanel_Update, S::CTFTeamStatusPlayerPanel_Update(), bool,
	void* rcx)
{
	DEBUG_RETURN(CTFTeamStatusPlayerPanel_Update, rcx);

	s_iPlayerIndex = *reinterpret_cast<int*>(uintptr_t(rcx) + 580);
	return CALL_ORIGINAL(rcx);
}

MAKE_HOOK(VGui_Panel_SetFgColor, S::VGui_Panel_SetFgColor(), void,
	void* rcx, Color_t color)
{
	DEBUG_RETURN(VGui_Panel_SetFgColor, rcx, color);

	if (!F::Groups.GroupsActive())
		return CALL_ORIGINAL(rcx, color);

	if (reinterpret_cast<std::uintptr_t>(_ReturnAddress()) == S::VGui_Panel_SetFgColor_RetAddr())
	{ // i'd rather not try to change overheal color because its set in the hud schema and then i would have to implement a bunch of classes and shit
		if (auto pEntity = I::ClientEntityList->GetClientEntity(s_iPlayerIndex)->As<CBaseEntity>())
		{
			auto pLocal = H::Entities.GetLocal();
			if(!pEntity || !pEntity->IsPlayer() || !pLocal)
				return CALL_ORIGINAL(rcx, color);

			auto pPlayer = pEntity->As<CTFPlayer>();
			if (Vars::Visuals::UI::RevealScoreboard.Value || pPlayer->m_iTeamNum() == pLocal->m_iTeamNum())
			{
				if(Group_t* pGroup = {}; F::Groups.GetGroup(pEntity, pGroup))
					color = GetHealthColor(pPlayer, pGroup, false);
			}
					
		}
	}
	return CALL_ORIGINAL(rcx, color);
}

MAKE_HOOK(VGui_Panel_SetBgColor, S::VGui_Panel_SetBgColor(), void,
	void* rcx, Color_t color)
{
	DEBUG_RETURN(VGui_Panel_SetBgColor, rcx, color);

	static const auto dwDesired = S::CTFTeamStatusPlayerPanel_Update_SetBgColor_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	if (dwRetAddr == dwDesired && Vars::Visuals::UI::ScoreboardColors.Value)
	{
		Color_t tColor = GetScoreboardColor(s_iPlayerIndex);
		if (tColor.a)
		{
			auto pResource = H::Entities.GetResource();
			if (pResource && !pResource->m_bAlive(s_iPlayerIndex))
				tColor = tColor.Lerp({ 127, 127, 127, tColor.a }, 0.5f);

			color = tColor;
		}
	}

	CALL_ORIGINAL(rcx, color);
}