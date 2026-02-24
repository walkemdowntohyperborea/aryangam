#include "../SDK/SDK.h"

#include "../Features/Visuals/Visuals.h"
#include "../Features/Ticks/Ticks.h"
#include "../Features/CritHack/CritHack.h"
#include "../Features/Visuals/SpectatorList/SpectatorList.h"
#include "../Features/Visuals/Spotify/Spotify.h"
#include "../Features/Backtrack/Backtrack.h"
#include "../Features/Visuals/PlayerConditions/PlayerConditions.h"
#include "../Features/NoSpread/NoSpreadHitscan/NoSpreadHitscan.h"
#include "../Features/Aimbot/Aimbot.h"
#include "../Features/Visuals/ESP/ESP.h"
#include "../Features/Visuals/OffscreenArrows/OffscreenArrows.h"
#include "../Features/Visuals/CameraWindow/CameraWindow.h"
#include "../Features/Visuals/Notifications/Notifications.h"
#include "../Features/Visuals/Radar/Radar.h"
#include "../Features/Aimbot/AutoHeal/AutoHeal.h"
#include "../Features/ImGui/Menu/Menu.h"
#include <ImGui/imgui_impl_dx9.h>

MAKE_HOOK(IEngineVGui_Paint, U::Memory.GetVirtual(I::EngineVGui, 14), void,
	void* rcx, int iMode)
{
	DEBUG_RETURN(IEngineVGui_Paint, rcx, iMode);

	if (G::Unload)
		return CALL_ORIGINAL(rcx, iMode);

	if (iMode & PAINT_INGAMEPANELS && (!(Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())))
	{
		H::Draw.UpdateScreenSize();
		H::Draw.UpdateW2SMatrix();
		H::Draw.Start(true);
		if (auto pLocal = H::Entities.GetLocal())
		{
			F::CameraWindow.Draw();
			F::Visuals.DrawServerHitboxes(pLocal);
			F::Visuals.DrawAntiAim(pLocal);

			F::Visuals.DrawPickupTimers();
			F::ESP.Draw();
			F::Arrows.Draw(pLocal);
			F::Aimbot.Draw(pLocal);
			F::Radar.Run(pLocal);
#ifdef DEBUG_VACCINATOR
			F::AutoHeal.Draw(pLocal);
#endif
		}
		H::Draw.End();
	}

	CALL_ORIGINAL(rcx, iMode);

	if (iMode & PAINT_UIPANELS && (!(Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())))
	{
		H::Draw.UpdateScreenSize();
		H::Draw.Start();
		{
			if (auto pLocal = H::Entities.GetLocal())
			{
				F::NoSpreadHitscan.Draw(pLocal);
				F::PlayerConditions.Draw(pLocal);
				F::SpectatorList.Draw(pLocal);
				F::CritHack.Draw(pLocal);
				F::Ticks.Draw(pLocal);
				F::Visuals.DrawDebugInfo(pLocal);
				F::Backtrack.Draw(pLocal);
			}
			F::Notifications.Draw();
			F::Spotify.Draw();

			if (F::Menu.m_bIsOpen)
				F::Visuals.CatDraw(F::Menu.m_vWindowSize, F::Menu.m_vWindowPos);
		}
		H::Draw.End();
	}
}