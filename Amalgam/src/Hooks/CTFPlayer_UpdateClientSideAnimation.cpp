#include "../SDK/SDK.h"

MAKE_HOOK(CTFPlayer_UpdateClientSideAnimation, S::CTFPlayer_UpdateClientSideAnimation(), void,
	void* rcx)
{	
	DEBUG_RETURN(CTFPlayer_UpdateClientSideAnimation, rcx);

	if (Vars::Misc::Game::AccuracyImprovements.Value)
	{
		auto pLocal = H::Entities.GetLocal();
		if (rcx == pLocal)
		{
			if (!pLocal->InCond(TF_COND_HALLOWEEN_KART))
			{
				auto pWeapon = H::Entities.GetWeapon();
				if (pWeapon)
					pWeapon->UpdateAllViewmodelAddons();

				return;
			}
			CALL_ORIGINAL(rcx);
		}

		if (!G::UpdatingAnims)
			return;
	}

	CALL_ORIGINAL(rcx);
}