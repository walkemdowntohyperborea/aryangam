#include "../SDK/SDK.h"
#include "../Features/Visuals/Groups/Groups.h"

MAKE_SIGNATURE(CHudChat_GetClientColor, "client.dll", "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 55 41 56 48 83 EC ? 48 8B 1D", 0x0);

static inline int ColorToInt(const Color_t& col)
{
	return col.b << 16 | col.g << 8 | col.r;
}

MAKE_HOOK(CHudChat_GetClientColor, S::CHudChat_GetClientColor(), int*,
	void* rcx, int* iOutColor, int clientIndex)
{
	DEBUG_RETURN(CHudChat_GetClientColor, rcx, iOutColor, clientIndex);

	static thread_local int iChatColor;

	if (!Vars::Visuals::UI::ChatColors.Value || clientIndex == 0 || !F::Groups.GroupsActive())
		return CALL_ORIGINAL(rcx, iOutColor, clientIndex);

	auto pLocal = H::Entities.GetLocal();
	if (!pLocal)
		return CALL_ORIGINAL(rcx, iOutColor, clientIndex);

	if (clientIndex == I::EngineClient->GetLocalPlayer())
	{
		iChatColor = ColorToInt(Vars::Colors::Local.Value);
		return &iChatColor;
	}

	if (auto pPlayer = I::ClientEntityList->GetClientEntity(clientIndex)->As<CTFPlayer>())
	{
		Group_t* pGroup{}; 
		if (F::Groups.GroupsActive() && F::Groups.GetGroup(pPlayer, pGroup, false))
		{
			iChatColor = ColorToInt(F::Groups.GetColor(pPlayer, pGroup));
			return &iChatColor; // this is like really dangerous and could cause crashes but it seems to work fine?
		}
	}

	return CALL_ORIGINAL(rcx, iOutColor, clientIndex);
}