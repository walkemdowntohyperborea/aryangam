#pragma once
#include "../../SDK/SDK.h"
#include <ImGui/imgui_impl_dx9.h>

struct PickupData_t
{
	int m_iType = 0;
	float m_flTime = 0.f;
	Vec3 m_vLocation;
};

class CVisuals
{
private:
	int m_nHudZoom = 0;
	std::vector<PickupData_t> m_vPickups;

public:
	void Event(IGameEvent* pEvent, uint32_t uHash);

	void ProjectileTrace(CTFPlayer* pPlayer, CTFWeaponBase* pWeapon, const bool bInterp = true);
	void SplashRadius(CTFPlayer* pLocal);
	void DrawAntiAim(CTFPlayer* pLocal);
	void DrawPickupTimers();
	void DrawDebugInfo(CTFPlayer* pLocal);

	std::vector<DrawBox_t> GetHitboxes(matrix3x4* aBones, CBaseAnimating* pEntity, std::vector<int> vHitboxes = {}, int iTarget = -1);
	void DrawEffects();
	void DrawServerHitboxes(CTFPlayer* pLocal);

	void FOV(CTFPlayer* pLocal, CViewSetup* pView);
	void ThirdPerson(CTFPlayer* pLocal, CViewSetup* pView);

	void OverrideWorldTextures();
	void Modulate();
	void RestoreWorldModulation();

	void CreateMove(CTFPlayer* pLocal, CTFWeaponBase* pWeapon);

	void CatDraw(ImVec2 vWindowSize, ImVec2 vWindowPos);
};

ADD_FEATURE(CVisuals, Visuals);