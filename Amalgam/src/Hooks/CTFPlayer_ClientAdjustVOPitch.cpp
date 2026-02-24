#include "../SDK/SDK.h"

MAKE_SIGNATURE(CTFPlayer_ClientAdjustVOPitch, "client.dll", "48 89 5C 24 ? 57 48 83 EC ? 0F 29 74 24 ? 48 8B DA 48 8B F9", 0x0);
MAKE_SIGNATURE(IsLocalPlayerUsingVisionFilterFlags, "client.dll", "48 89 5C 24 ? 56 48 83 EC ? 8B 1D", 0x0);	

MAKE_HOOK(CTFPlayer_ClientAdjustVOPitch, S::CTFPlayer_ClientAdjustVOPitch(), void,
	void* rcx, int& pitch)
{
	DEBUG_RETURN(CTFPlayer_ClientAdjustVOPitch, rcx, pitch);

	if (S::IsLocalPlayerUsingVisionFilterFlags.Call<bool>(TF_VISION_FILTER_PYRO))
		pitch *= Vars::Visuals::Effects::PyrovisionPitch.Value;
	else
		CALL_ORIGINAL(rcx, pitch);
}