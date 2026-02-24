#include "../SDK/SDK.h"

MAKE_SIGNATURE(CSequenceTransitioner_CheckForSequenceChange, "client.dll", "48 85 D2 0F 84 ? ? ? ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24", 0x0);

MAKE_HOOK(CSequenceTransitioner_CheckForSequenceChange, S::CSequenceTransitioner_CheckForSequenceChange(), void,
	void* rcx, CStudioHdr* hdr, int nCurSequence, bool bForceNewSequence, bool bInterpolate)
{
	DEBUG_RETURN(CSequenceTransitioner_CheckForSequenceChange, rcx, hdr, nCurSequence, bForceNewSequence, bInterpolate);

	if (Vars::Misc::Game::AccuracyImprovements.Value)
		bInterpolate = false;

	CALL_ORIGINAL(rcx, hdr, nCurSequence, bForceNewSequence, bInterpolate);
}