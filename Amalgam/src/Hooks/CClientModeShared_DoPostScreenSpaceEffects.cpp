#include "../SDK/SDK.h"

#include "../Features/Visuals/Chams/Chams.h"
#include "../Features/Visuals/ESP/ESP.h"
#include "../Features/Visuals/Glow/Glow.h"
#include "../Features/Visuals/CameraWindow/CameraWindow.h"
#include "../Features/Visuals/Visuals.h"
#include "../Features/Visuals/Materials/Materials.h"
#include "../Features/Spectate/Spectate.h"

MAKE_HOOK(CClientModeShared_DoPostScreenSpaceEffects, U::Memory.GetVirtual(I::ClientModeShared, 39), bool,
	void* rcx, const CViewSetup* pSetup)
{
	DEBUG_RETURN(CClientModeShared_DoPostScreenSpaceEffects, rcx, pSetup);

	if (Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return CALL_ORIGINAL(rcx, pSetup);

	auto pLocal = H::Entities.GetLocal();
	auto pWeapon = H::Entities.GetWeapon();
	if (pLocal)
	{
		if (pWeapon)
		{
			F::Visuals.SplashRadius(pLocal);
			F::Visuals.ProjectileTrace(pLocal, pWeapon);
		}
		F::ESP.DrawSoundESP();
	}

	if (!F::CameraWindow.m_bDrawing)
	{
		F::Visuals.DrawEffects();
		F::Chams.m_mEntities.clear();
		if (!I::EngineVGui->IsGameUIVisible() && F::Materials.m_bLoaded)
		{
			F::Chams.RenderMain();
			F::Glow.RenderMain();
		}
	}

	return CALL_ORIGINAL(rcx, pSetup);
}