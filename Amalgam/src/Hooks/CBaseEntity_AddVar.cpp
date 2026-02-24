#include "../SDK/SDK.h"

MAKE_SIGNATURE(CBaseEntity_AddVar, "client.dll", "48 89 5C 24 ? 48 89 6C 24 ? 57 41 56 41 57 48 83 EC ? 33 DB 48 89 74 24", 0x0);

MAKE_HOOK(CBaseEntity_AddVar, S::CBaseEntity_AddVar(), void,
	CBaseEntity* rcx, void* data, IInterpolatedVar* watcher, int type, bool bSetup)
{
	DEBUG_RETURN(CBaseEntity_AddVar, rcx, data, watcher, type, bSetup);

	if (Vars::Misc::Game::AccuracyImprovements.Value && watcher)
	{
		uint32_t uHash = FNV1A::Hash32(watcher->GetDebugName());
		if (uHash == FNV1A::Hash32Const("C_BaseEntity::m_iv_vecVelocity") ||
			uHash == FNV1A::Hash32Const("C_BaseAnimating::m_iv_flPoseParameter") ||
			uHash == FNV1A::Hash32Const("C_BaseAnimating::m_iv_flCycle") ||
			uHash == FNV1A::Hash32Const("CMultiPlayerAnimState::m_iv_flMaxGroundSpeed"))
			return;

		if (rcx != H::Entities.GetLocal() &&
			uHash == FNV1A::Hash32Const("C_TFPlayer::m_iv_angEyeAngles"))
			return;
	}

	CALL_ORIGINAL(rcx, data, watcher, type, bSetup);
}