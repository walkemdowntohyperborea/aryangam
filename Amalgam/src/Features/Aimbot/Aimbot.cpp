#include "Aimbot.h"

#include "AimbotHitscan/AimbotHitscan.h"
#include "AimbotProjectile/AimbotProjectile.h"
#include "AimbotMelee/AimbotMelee.h"
#include "AutoDetonate/AutoDetonate.h"
#include "AutoAirblast/AutoAirblast.h"
#include "AutoHeal/AutoHeal.h"
#include "AutoRocketJump/AutoRocketJump.h"
#include "../Misc/Misc.h"
#include "../Visuals/Visuals.h"

bool CAimbot::ShouldRun(CTFPlayer* pLocal, CTFWeaponBase* pWeapon)
{
	if (!pWeapon || !pLocal->CanAttack()
		|| !SDK::AttribHookValue(1, "mult_dmg", pWeapon)
		|| I::EngineVGui->IsGameUIVisible())
		return false;

	return true;
}

void CAimbot::RunAimbot(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd, bool bSecondaryType)
{
	m_bRunningSecondary = bSecondaryType;
	EWeaponType eWeaponType = !m_bRunningSecondary ? G::PrimaryWeaponType : G::SecondaryWeaponType;

	bool bOriginal;
	if (m_bRunningSecondary)
		bOriginal = G::CanPrimaryAttack, G::CanPrimaryAttack = G::CanSecondaryAttack;

	if(Vars::Aimbot::General::AutoShootSpells.Value)
		F::AimbotProjectile.Run(pLocal, pWeapon, pCmd);
	else
	{
		switch (eWeaponType)
		{
		case EWeaponType::HITSCAN: F::AimbotHitscan.Run(pLocal, pWeapon, pCmd); break;
		case EWeaponType::PROJECTILE: F::AimbotProjectile.Run(pLocal, pWeapon, pCmd); break;
		case EWeaponType::MELEE: F::AimbotMelee.Run(pLocal, pWeapon, pCmd); break;
		}
	}

	if (m_bRunningSecondary)
		G::CanPrimaryAttack = bOriginal;
}

void CAimbot::RunMain(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	if (F::AimbotProjectile.m_iLastTickCancel)
	{
		pCmd->weaponselect = F::AimbotProjectile.m_iLastTickCancel;
		F::AimbotProjectile.m_iLastTickCancel = 0;
	}

	m_bRan = false;
	if (abs(G::AimTarget.m_iTickCount - I::GlobalVars->tickcount) > G::AimTarget.m_iDuration)
		G::AimTarget = {};
	if (abs(G::AimPoint.m_iTickCount - I::GlobalVars->tickcount) > G::AimPoint.m_iDuration)
		G::AimPoint = {};

	if (pCmd->weaponselect)
		return;

	F::AutoRocketJump.Run(pLocal, pWeapon, pCmd);
	if (!ShouldRun(pLocal, pWeapon))
		return;

	F::AutoDetonate.Run(pLocal, pCmd);
	F::AutoAirblast.Run(pLocal, pWeapon, pCmd);
	F::AutoHeal.Run(pLocal, pWeapon, pCmd);

	RunAimbot(pLocal, pWeapon, pCmd);
	RunAimbot(pLocal, pWeapon, pCmd, true);
}

void CAimbot::Run(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	Store(false);

	RunMain(pLocal, pWeapon, pCmd);

	G::Attacking = SDK::IsAttacking(pLocal, pWeapon, pCmd, true);
}

void CAimbot::Draw(CTFPlayer* pLocal)
{
	if (!Vars::Aimbot::General::FOVCircle.Value || !Vars::Colors::FOVCircle.Value.a || !pLocal->CanAttack(false))
		return;

	auto pWeapon = H::Entities.GetWeapon();
	if (!pLocal || !pWeapon || pWeapon && !SDK::AttribHookValue(1, "mult_dmg", pWeapon))
		return;

	float flFOV = 0.f;
	if (pWeapon->GetWeaponID() == TF_WEAPON_MEDIGUN || pWeapon->GetWeaponID() == TF_WEAPON_CROSSBOW)
		flFOV = Vars::Aimbot::Healing::AimFOV.Value;
	else
	{
		EWeaponType SecondaryWeaponType = EWeaponType::UNKNOWN;
		EWeaponType PrimaryWeaponType = SDK::GetWeaponType(pWeapon, &SecondaryWeaponType);
		switch (SecondaryWeaponType != EWeaponType::UNKNOWN ? SecondaryWeaponType : PrimaryWeaponType)
		{
		case EWeaponType::HITSCAN:
			flFOV = Vars::Aimbot::Hitscan::AimFOV.Value;
			break;
		case EWeaponType::PROJECTILE:
			flFOV = Vars::Aimbot::Projectile::AimFOV.Value;
			break;
		case EWeaponType::MELEE:
			flFOV = Vars::Aimbot::Melee::AimFOV.Value;
		}
	}

	if (flFOV >= 90.f)
		return;

	float flRadius = tanf(DEG2RAD(flFOV)) / tanf(DEG2RAD(pLocal->m_iFOV()) / 2) * float(H::Draw.m_nScreenW) * (4.f / 6.f) / (16.f / 9.f);
	H::Draw.LineCircle(H::Draw.m_nScreenW / 2, H::Draw.m_nScreenH / 2, flRadius, 68, Vars::Colors::FOVCircle.Value);
}


void CAimbot::Store(CBaseEntity* pEntity, size_t iSize)
{
	if (!Vars::Visuals::Simulation::RealPath.Value)
		return;

	if (!pEntity->IsPlayer())
		return;

	if (auto pResource = H::Entities.GetResource())
	{
		m_tPath.m_vPath.push_back({ pEntity->m_vecOrigin(), pEntity->GetAbsVelocity(), SDK::MaxSpeed(pEntity->As<CTFPlayer>()) });
		m_tPath.m_flTime = I::GlobalVars->curtime + Vars::Visuals::Simulation::DrawDuration.Value;
		m_tPath.m_tColor = Vars::Colors::RealPath.Value;
		m_tPath.m_iStyle = Vars::Visuals::Simulation::RealPath.Value;
		m_iSize = iSize;
		m_iPlayer = pResource->m_iUserID(pEntity->entindex());
	}
}

void CAimbot::Store(bool bFrameStageNotify)
{
	if (!Vars::Visuals::Simulation::RealPath.Value)
		return;

	int iLag = 1;
	if (bFrameStageNotify)
	{
		static int iStaticTickcout = I::GlobalVars->tickcount;
		iLag = I::GlobalVars->tickcount - iStaticTickcout;
		iStaticTickcout = I::GlobalVars->tickcount;
	}

	if (!m_tPath.m_flTime)
		return;
	else if (m_tPath.m_vPath.size() >= m_iSize || m_tPath.m_flTime < I::GlobalVars->curtime)
	{
		if (m_tPath.m_tColor = Vars::Colors::RealPath.Value, m_tPath.m_bZBuffer = true; m_tPath.m_tColor.a)
			G::PathStorage.push_back(m_tPath);
		if (m_tPath.m_tColor = Vars::Colors::RealPathIgnoreZ.Value, m_tPath.m_bZBuffer = false; m_tPath.m_tColor.a)
			G::PathStorage.push_back(m_tPath);
		m_tPath = {};
		return;
	}

	int iIndex = I::EngineClient->GetPlayerForUserID(m_iPlayer);
	if (bFrameStageNotify ? iIndex == I::EngineClient->GetLocalPlayer() : iIndex != I::EngineClient->GetLocalPlayer())
		return;

	auto pPlayer = I::ClientEntityList->GetClientEntity(iIndex)->As<CTFPlayer>();
	if (!pPlayer)
		return;

	for (int i = 0; i < iLag; i++)
		m_tPath.m_vPath.push_back({ pPlayer->m_vecOrigin(), pPlayer->GetAbsVelocity(), SDK::MaxSpeed(pPlayer) });
}