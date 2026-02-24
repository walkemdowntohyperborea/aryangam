#include "../SDK/SDK.h"

MAKE_SIGNATURE(CBaseHudChat_StartMessageMode, "client.dll", "40 53 56 57 41 56 41 57 48 83 EC ? 89 91", 0x0);

class CBaseHudChatInputLine
{
public:
	VIRTUAL_ARGS(InvalidateLayout, void, 66, (bool layoutNow, bool reloadScheme), this, layoutNow, reloadScheme);
};

MAKE_HOOK(CBaseHudChat_StartMessageMode, U::Memory.GetVirtual(I::ClientModeShared->m_pChatElement, 20), void,
	void* rcx, int iMessageModeType)
{
	DEBUG_RETURN(CBaseHudChat_StartMessageMode, rcx, iMessageModeType);

	CALL_ORIGINAL(rcx, iMessageModeType);

	if (iMessageModeType && SDK::StdRandomInt(0, 69) < 5)
	{
		// this has to crash at some point right?
		void* chatInput = *(void**)((uintptr_t)rcx + 0x298);
		void* textPrompt = *(void**)((uintptr_t)chatInput + 0x1F0);
		const void* chatInputVTable = *(void**)textPrompt;
		const uintptr_t setPromptFunc = *(uintptr_t*)((uintptr_t)chatInputVTable + 0x6A8);

		const wchar_t* pszPrompt;
		switch (iMessageModeType)
		{
		case MM_SAY_TEAM:
			pszPrompt = L"A retard would say this in team chat : ";
			break;
		case MM_SAY_PARTY:
			pszPrompt = L"A retard would say this in party chat : ";
			break;
		default:
			pszPrompt = L"A retard would say : ";
		}
		using FnSetPrompt = void(__fastcall*)(void* rcx, const wchar_t* prompt, uint64_t balls);
		reinterpret_cast<FnSetPrompt>(setPromptFunc)((void*)textPrompt, pszPrompt, 0);
		reinterpret_cast<CBaseHudChatInputLine*>(chatInput)->InvalidateLayout(false, false);
	}
}