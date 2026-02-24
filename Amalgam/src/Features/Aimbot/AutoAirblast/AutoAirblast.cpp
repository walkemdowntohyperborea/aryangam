#include "AutoAirblast.h"

#include "../AimbotProjectile/AimbotProjectile.h"
#include "../../Simulation/ProjectileSimulation/ProjectileSimulation.h"
#include "../../Backtrack/Backtrack.h"

static inline bool SupportsAirBlastFunction(CTFWeaponBase* pWeapon, EFlameThrowerAirblastFunction eFunction)
{
	if (!pWeapon)
		return false;

	int iSupportedAirBlastFunctions = SDK::AttribHookValue(0, "airblast_functionality_flags", pWeapon);
	if (iSupportedAirBlastFunctions == 0)
		return true;

	return (iSupportedAirBlastFunctions & eFunction) != 0;
}

static inline bool ShouldTarget(CBaseEntity* pEntity, CTFPlayer* pLocal)
{
	bool bIsTeam = pEntity->m_iTeamNum() == pLocal->m_iTeamNum();
	if (bIsTeam && !pEntity->IsPlayer())
		return false;

	if (pEntity->IsPlayer())
	{
		if (bIsTeam)
		{
			if (!(Vars::Aimbot::Projectile::AutoAirblast.Value & Vars::Aimbot::Projectile::AutoAirblastEnum::Teammates))
				return false;

			auto pPlayer = pEntity->As<CTFPlayer>();
			if (!pPlayer->InCond(TF_COND_BURNING))
				return false;

			if (!SupportsAirBlastFunction(H::Entities.GetWeapon(), TF_FUNCTION_AIRBLAST_PUT_OUT_TEAMMATES))
				return false;
		}
		else
		{
			if (!(Vars::Aimbot::Projectile::AutoAirblast.Value & Vars::Aimbot::Projectile::AutoAirblastEnum::Melee))
				return false;

			auto pPlayer = pEntity->As<CTFPlayer>();
			auto pWeapon = pPlayer->m_hActiveWeapon()->As<CTFWeaponBase>();
			auto eWeaponType = SDK::GetWeaponType(pWeapon);
			if (eWeaponType != EWeaponType::MELEE && !pPlayer->InCond(TF_COND_MEGAHEAL))
				return false;
		}
	}
	else
	{
		switch (pEntity->GetClassID())
		{
		case ETFClassID::CTFProjectile_SpellFireball:
		case ETFClassID::CTFProjectile_SpellBats:
			return false;
		case ETFClassID::CTFGrenadePipebombProjectile:
		{
			auto pGrenade = pEntity->As<CTFGrenadePipebombProjectile>();
			if (pGrenade->m_bTouched() || pGrenade->m_iType() == TF_PROJECTILE_PIPEBOMB_PRACTICE)
				return false;
			break;
		}
		case ETFClassID::CTFProjectile_Cleaver:
		case ETFClassID::CTFProjectile_Jar:
		case ETFClassID::CTFProjectile_JarGas:
		case ETFClassID::CTFProjectile_Flare:
		case ETFClassID::CTFProjectile_Arrow:
		case ETFClassID::CTFProjectile_HealingBolt:
			if (pEntity->m_MoveType() == MOVETYPE_NONE)
				return false;
		}

		if (auto pWeapon = F::ProjSim.GetEntities(pEntity).first)
		{
			if (!SDK::AttribHookValue(1, "mult_dmg", pWeapon))
				return false;
		}
	}

	return true;
}

bool CAutoAirblast::CanAirblastEntity(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CBaseEntity* pEntity, const Vec3& vAngle)
{
	auto flRadius = SDK::AttribHookValue(1, "deflection_size_multiplier", pWeapon) * 128.f;
	if (pEntity->IsProjectile())
	{
		if (SDK::AttribHookValue(0, "airblast_deflect_projectiles_disabled", pWeapon))
			return false;

		Vec3 vForward; Math::AngleVectors(vAngle, &vForward);
		Vec3 vOrigin = pLocal->GetShootPos() + vForward * flRadius;

		CBaseEntity* pTarget;
		for (CEntitySphereQuery sphere(vOrigin, flRadius);
			pTarget = sphere.GetCurrentEntity();
			sphere.NextEntity())
		{
			if (pTarget == pEntity)
				break;
		}

		return pTarget == pEntity && SDK::VisPosWorld(pLocal, pEntity, pLocal->GetShootPos(), pEntity->GetAbsOrigin());
	}
	else if(pEntity->IsPlayer())
	{
		if (SDK::AttribHookValue(0, "airblast_pushback_disabled", pWeapon))
			return false;

		if (pLocal->m_vecOrigin().DistTo(pEntity->m_vecOrigin()) > pEntity->As<CTFPlayer>()->m_hActiveWeapon()->As<CTFWeaponBase>()->GetSwingRange() * 1.2f)
			return false;

		Vec3 vForward = pLocal->GetAbsVelocity();
		if (!pLocal->m_hGroundEntity() || vForward.Length() == 0.0f)
			Math::AngleVectors(vAngle, &vForward);
		vForward.z = 0.0f;
		vForward.Normalize();

		truncatedcone_t tTestCone;
		tTestCone.normal = vForward;
		tTestCone.origin = pLocal->GetEyePosition();
		tTestCone.h = 2.f * flRadius;
		tTestCone.theta = SDK::AttribHookValue(1.f, "mult_airblast_cone_scale", pWeapon) * 35.f;

		Vec3 vTargetAbsMins = pEntity->GetAbsOrigin() + pEntity->GetCollideable()->OBBMins();
		Vec3 vTargetAbsMaxs = pEntity->GetAbsOrigin() + pEntity->GetCollideable()->OBBMaxs();

		return I::PhysicsCollision->IsBoxIntersectingCone(vTargetAbsMins, vTargetAbsMaxs, tTestCone);
	}

	return false;
}

void CAutoAirblast::Run(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	if (!(Vars::Aimbot::Projectile::AutoAirblast.Value & Vars::Aimbot::Projectile::AutoAirblastEnum::Enabled) || !G::CanSecondaryAttack)
		return;

	const int iWeaponID = pWeapon->GetWeaponID();
	if (iWeaponID != TF_WEAPON_FLAMETHROWER && iWeaponID != TF_WEAPON_FLAME_BALL || 
		SDK::AttribHookValue(0, "airblast_disabled", pWeapon) || 
		pLocal->m_nWaterLevel() == WL_Eyes)
		return;

	static auto tf_flamethrower_burstammo = U::ConVars.FindVar("tf_flamethrower_burstammo");
	int iAmmoPerShot = tf_flamethrower_burstammo->GetInt() * SDK::AttribHookValue(1, "mult_airblast_cost", pWeapon);
	int iAmmo = pLocal->GetAmmoCount(pWeapon->m_iPrimaryAmmoType());
	int iBuffType = SDK::AttribHookValue(0, "set_buff_type", pWeapon);
	int iChargedAirblast = SDK::AttribHookValue(0, "set_charged_airblast", pWeapon);
	if (iAmmo < iAmmoPerShot || iBuffType || iChargedAirblast)
		return;

	bool bShouldBlast = false;
	const Vec3 vEyePos = pLocal->GetShootPos();

	float flLatency = std::max(F::Backtrack.GetReal() - 0.03f, 0.f);
	for (auto pProjectile : H::Entities.GetGroup(EntityEnum::WorldProjectile))
	{
		if (!ShouldTarget(pProjectile, pLocal))
			continue;

		Vec3 vOrigin;
		if (!SDK::PredictOrigin(vOrigin, pProjectile->m_vecOrigin(), F::ProjSim.GetVelocity(pProjectile), flLatency))
			continue;

		if (!(Vars::Aimbot::Projectile::AutoAirblast.Value & Vars::Aimbot::Projectile::AutoAirblastEnum::IgnoreFOV)
			&& Math::CalcFov(I::EngineClient->GetViewAngles(), Math::CalcAngle(vEyePos, vOrigin)) > Vars::Aimbot::Projectile::AimFOV.Value)
			continue;

		Vec3 vRestoreOrigin = pProjectile->GetAbsOrigin();
		pProjectile->SetAbsOrigin(vOrigin);
		if (Vars::Aimbot::Projectile::AutoAirblast.Value & Vars::Aimbot::Projectile::AutoAirblastEnum::Redirect)
		{
			Vec3 vAngle = Math::CalcAngle(vEyePos, vOrigin);
			if (CanAirblastEntity(pLocal, pWeapon, pProjectile, vAngle))
			{
				bShouldBlast = true;
				if (!F::AimbotProjectile.AutoAirblast(pLocal, pWeapon, pCmd, pProjectile))
				{
					SDK::FixMovement(pCmd, vAngle);
					pCmd->viewangles = vAngle;
					G::PSilentAngles = true;
				}
			}
		}
		else if (CanAirblastEntity(pLocal, pWeapon, pProjectile, pCmd->viewangles))
			bShouldBlast = true;
		pProjectile->SetAbsOrigin(vRestoreOrigin);

		if (bShouldBlast)
			break;
	}

	if (Vars::Aimbot::Projectile::AutoAirblast.Value & Vars::Aimbot::Projectile::AutoAirblastEnum::Teammates)
	{
		for (auto pEntity : H::Entities.GetGroup(EntityEnum::PlayerTeam))
		{
			if (!ShouldTarget(pEntity, pLocal))
				continue;

			Vec3 vOrigin;
			if (!SDK::PredictOrigin(vOrigin, pEntity->m_vecOrigin(), pEntity->GetAbsVelocity(), flLatency))
				continue;

			if (!(Vars::Aimbot::Projectile::AutoAirblast.Value & Vars::Aimbot::Projectile::AutoAirblastEnum::IgnoreFOV)
				&& Math::CalcFov(I::EngineClient->GetViewAngles(), Math::CalcAngle(vEyePos, vOrigin)) > Vars::Aimbot::Projectile::AimFOV.Value)
				continue;

			Vec3 vRestoreOrigin = pEntity->GetAbsOrigin();
			pEntity->SetAbsOrigin(vOrigin);

			Vec3 vAngle = Math::CalcAngle(vEyePos, vOrigin);
			if (CanAirblastEntity(pLocal, pWeapon, pEntity, vAngle))
			{
				bShouldBlast = true;

				SDK::FixMovement(pCmd, vAngle);
				pCmd->viewangles = vAngle;
				G::PSilentAngles = true;
			}
			pEntity->SetAbsOrigin(vRestoreOrigin);
		}
	}
	if (Vars::Aimbot::Projectile::AutoAirblast.Value & Vars::Aimbot::Projectile::AutoAirblastEnum::Melee)
	{
		for (auto pEntity : H::Entities.GetGroup(EntityEnum::PlayerEnemy))
		{
			if (!ShouldTarget(pEntity, pLocal))
				continue;

			Vec3 vOrigin;
			if (!SDK::PredictOrigin(vOrigin, pEntity->m_vecOrigin(), pEntity->GetAbsVelocity(), flLatency))
				continue;

			if (!(Vars::Aimbot::Projectile::AutoAirblast.Value & Vars::Aimbot::Projectile::AutoAirblastEnum::IgnoreFOV)
				&& Math::CalcFov(I::EngineClient->GetViewAngles(), Math::CalcAngle(vEyePos, vOrigin)) > Vars::Aimbot::Projectile::AimFOV.Value)
				continue;

			Vec3 vRestoreOrigin = pEntity->GetAbsOrigin();
			pEntity->SetAbsOrigin(vOrigin);

			Vec3 vAngle = Math::CalcAngle(vEyePos, vOrigin);
			if (CanAirblastEntity(pLocal, pWeapon, pEntity, vAngle))
			{
				bShouldBlast = true;

				SDK::FixMovement(pCmd, vAngle);
				pCmd->viewangles = vAngle;
				G::PSilentAngles = true;
			}
			pEntity->SetAbsOrigin(vRestoreOrigin);
		}
	}

	if (bShouldBlast)
	{
		G::Attacking = true;
		pCmd->buttons |= IN_ATTACK2;
	}
}