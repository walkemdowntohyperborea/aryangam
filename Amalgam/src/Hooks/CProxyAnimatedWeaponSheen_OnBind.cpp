#include "../SDK/SDK.h"
#pragma warning(disable: 4305) // we are doing it purposefully here

MAKE_SIGNATURE(CProxyAnimatedWeaponSheen_OnBind, "client.dll", "48 89 54 24 ? 55 57 41 54 48 8D 6C 24", 0x0);

struct CProxyAnimatedWeaponSheen
{
	__int64 idk;
	IMaterialVar* m_AnimatedTextureVar;
	IMaterialVar* m_AnimatedTextureFrameNumVar;
	float m_FrameRate;
	__declspec(align(4)) bool m_WrapAnimation;
	IMaterialVar* m_pSheenIndexVar;
	IMaterialVar* m_pTintVar;
	IMaterialVar* m_pSheenVar;
	IMaterialVar* m_pSheenMaskVar;
	IMaterialVar* m_pScaleXVar;
	IMaterialVar* m_pScaleYVar;
	IMaterialVar* m_pOffsetXVar;
	IMaterialVar* m_pOffsetYVar;
	IMaterialVar* m_pDirectionVar;
	float m_flNextStartTime;
	float m_flScaleX;
	float m_flScaleY;
	float m_flSheenOffsetX;
	float m_flSheenOffsetY;
	int m_iSheenDir;
};

MAKE_HOOK(CProxyAnimatedWeaponSheen_OnBind, S::CProxyAnimatedWeaponSheen_OnBind(), void,
	CProxyAnimatedWeaponSheen* rcx, void* pEntity)
{
	DEBUG_RETURN(CProxyAnimatedWeaponSheen_OnBind, rcx, pEntity);

	CALL_ORIGINAL(rcx, pEntity);

	if (!(Vars::Visuals::World::Modulations.Value & Vars::Visuals::World::ModulationsEnum::KillstreakSheen)
		|| Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot() || !rcx)
		return;

	IMaterialVar* m_pTintVar = rcx->m_pTintVar;
	if (!m_pTintVar)
		return;

	const float* color = m_pTintVar->GetVecValueInternal();
	if (!color || !color[3])
		return;

	const Color_t tColor = Vars::Colors::SheenModulation.Value;
	float flColors[4] = { tColor.r / 255.f, tColor.g / 255.f, tColor.b / 255.f, tColor.a / 255.f };
	m_pTintVar->SetVecValue(flColors, 4);
}