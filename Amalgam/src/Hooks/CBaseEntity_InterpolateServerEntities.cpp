#include "../SDK/SDK.h"

MAKE_SIGNATURE(CBaseEntity_InterpolateServerEntities, "client.dll", "4C 8B DC 41 54 41 55 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 45 33 ED 49 89 5B ? 48 8D 1D", 0x0);

MAKE_HOOK(CBaseEntity_InterpolateServerEntities, S::CBaseEntity_InterpolateServerEntities(), void,
	void* rcx)
{
	DEBUG_RETURN(CBaseEntity_InterpolateServerEntities, rcx);

	if (Vars::Misc::Game::AccuracyImprovements.Value)
	{
		static auto cl_extrapolate = U::ConVars.FindVar("cl_extrapolate");
		if (cl_extrapolate && cl_extrapolate->GetInt())
			cl_extrapolate->SetValue(0);
	}

	CALL_ORIGINAL(rcx);
}