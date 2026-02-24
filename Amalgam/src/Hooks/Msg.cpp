#include "../SDK/SDK.h"

MAKE_SIGNATURE(CMoveHelperClient_ProcessImpact_Msg_Call1, "client.dll", "49 8B 76 ? 48 8B CE E8 ? ? ? ? 49 8B 7E", 0x0);
MAKE_SIGNATURE(CMoveHelperClient_ProcessImpact_Msg_Call2, "client.dll", "49 8B 06 49 8B CE FF 50 ? 48 8B 7C 24 ? 48 8B 74 24", 0x0);

MAKE_SIGNATURE(DevMsgRT, "client.dll", "48 89 4C 24 ? 48 89 54 24 ? 4C 89 44 24 ? 4C 89 4C 24 ? 48 83 EC ? 48 8B 05", 0x0);
MAKE_SIGNATURE(CBaseAnimating_SetupBones_DevMsgRT_Call, "client.dll", "48 8B 05 ? ? ? ? F3 0F 10 00 F3 0F 11 05", 0x0);

MAKE_HOOK(Msg, U::Memory.GetModuleExport<void*>("tier0.dll", "Msg"), void,
	const char* pMsgFormat, ...)
{
	const auto dwRetAddr = (uintptr_t)_ReturnAddress();
	static const auto dwDesired1 = S::CMoveHelperClient_ProcessImpact_Msg_Call1();
	static const auto dwDesired2 = S::CMoveHelperClient_ProcessImpact_Msg_Call2();
	if (dwRetAddr == dwDesired1 || dwRetAddr == dwDesired2)
		return;

	// restore original call args
	char buffer[2048];
	va_list args;
	va_start(args, pMsgFormat);
	vsnprintf(buffer, sizeof(buffer), pMsgFormat, args);
	va_end(args);

	CALL_ORIGINAL("%s", buffer);
}

MAKE_HOOK(DevMsgRT, S::DevMsgRT(), void,
	char const* pMsg, ...)
{	// stop the extremely annoying bone access not allowed message
	const auto dwRetAddr = (uintptr_t)_ReturnAddress();
	static const auto dwDesired = S::CBaseAnimating_SetupBones_DevMsgRT_Call();
	if (dwRetAddr == dwDesired)
		return;

	// restore original call args
	char buffer[2048];
	va_list args;
	va_start(args, pMsg);
	vsnprintf(buffer, sizeof(buffer), pMsg, args);
	va_end(args);

	CALL_ORIGINAL("%s", buffer);
}