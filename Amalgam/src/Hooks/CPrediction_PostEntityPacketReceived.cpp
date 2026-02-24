#include "../Features/Misc/AntiAutobalance/AntiAutobalance.h"

#ifdef ANTIAUTOBALANCETESTING
#include "../SDK/SDK.h"

MAKE_SIGNATURE(CPrediction_PostEntityPacketReceived, "client.dll", "48 83 EC ? 48 8B 0D ? ? ? ? 48 8B 01 FF 90 ? ? ? ? 85 C0", 0x0);

MAKE_HOOK(CPrediction_PostEntityPacketReceived, S::CPrediction_PostEntityPacketReceived(), void)
{
	DEBUG_RETURN(CPrediction_PostEntityPacketReceived);

	CALL_ORIGINAL();
	F::AntiAutobalance.Run();
}
#endif