#include "../SDK/SDK.h"

MAKE_SIGNATURE(CTFRagdoll_CreateTFRagdoll, "client.dll", "48 89 4C 24 ? 55 53 56 57 48 8D AC 24 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 8B 91", 0x0);

MAKE_HOOK(CTFRagdoll_CreateTFRagdoll, S::CTFRagdoll_CreateTFRagdoll(), void,
	void* rcx)
{
	DEBUG_RETURN(CTFRagdoll_CreateTFRagdoll, rcx);

	if (Vars::Visuals::Removals::Ragdolls.Value)
		return;

	auto pRagdoll = reinterpret_cast<CTFRagdoll*>(rcx);
	if (Vars::Visuals::Effects::RagdollEffects.Value)
	{
		pRagdoll->m_bGib() = false;
		pRagdoll->m_bBurning() = Vars::Visuals::Effects::RagdollEffects.Value & Vars::Visuals::Effects::RagdollEffectsEnum::Burning;
		pRagdoll->m_bElectrocuted() = Vars::Visuals::Effects::RagdollEffects.Value & Vars::Visuals::Effects::RagdollEffectsEnum::Electrocuted;
		pRagdoll->m_bBecomeAsh() = Vars::Visuals::Effects::RagdollEffects.Value & Vars::Visuals::Effects::RagdollEffectsEnum::Ash;
		pRagdoll->m_bDissolving() = Vars::Visuals::Effects::RagdollEffects.Value & Vars::Visuals::Effects::RagdollEffectsEnum::Dissolve;
		pRagdoll->m_bGoldRagdoll() = Vars::Visuals::Effects::RagdollEffects.Value & Vars::Visuals::Effects::RagdollEffectsEnum::Gold;
		pRagdoll->m_bIceRagdoll() = Vars::Visuals::Effects::RagdollEffects.Value & Vars::Visuals::Effects::RagdollEffectsEnum::Ice;
	}
	pRagdoll->m_vecForce() *= Vars::Visuals::Effects::RagdollForce.Value;

	CALL_ORIGINAL(rcx);
}