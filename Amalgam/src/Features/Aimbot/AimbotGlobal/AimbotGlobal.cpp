#include "AimbotGlobal.h"

#include "../Aimbot.h"
#include "../AutoDetonate/AutoDetonate.h"
#include "../../Players/PlayerUtils.h"
#include "../../Ticks/Ticks.h"
#include "../../EnginePrediction/EnginePrediction.h"

void CAimbotGlobal::SortTargets(std::vector<Target_t>& vTargets, int iMethod)
{
	std::sort(vTargets.begin(), vTargets.end(), [&](const Target_t& a, const Target_t& b) -> bool
		{
			switch (iMethod)
			{
			case Vars::Aimbot::General::TargetSelectionEnum::FOV: return a.m_flFOVTo < b.m_flFOVTo;
			case Vars::Aimbot::General::TargetSelectionEnum::Distance: return a.m_flDistTo < b.m_flDistTo;
			default: return false;
			}
		});
}

void CAimbotGlobal::SortPriority(std::vector<Target_t>& vTargets)
{	// Sort by priority
	std::sort(vTargets.begin(), vTargets.end(), [&](const Target_t& a, const Target_t& b) -> bool
		{
			if (Vars::Aimbot::General::PreferMedics.Value)
				return ((a.m_pEntity->IsPlayer() && a.m_pEntity->As<CTFPlayer>()->m_iClass() == TF_CLASS_MEDIC) && (b.m_pEntity->IsPlayer() && b.m_pEntity->As<CTFPlayer>()->m_iClass() != TF_CLASS_MEDIC));
			return a.m_nPriority > b.m_nPriority;
		});
}

// this won't prevent shooting bones outside of fov
bool CAimbotGlobal::PlayerBoneInFOV(CTFPlayer* pTarget, Vec3 vLocalPos, Vec3 vLocalAngles, float& flFOVTo, Vec3& vPos, Vec3& vAngleTo, float aimFOV, int iHitboxes)
{
	matrix3x4* aBones = F::Backtrack.GetBones(pTarget);
	if (!Vars::Visuals::Removals::Interpolation.Value)
	{
		std::vector<TickRecord*> vRecords = {};
		if (F::Backtrack.GetRecords(pTarget, vRecords) && !vRecords.empty())
		{
			float flLerp = Vars::Visuals::Removals::Lerp.Value ? 0.f : G::Lerp;
			for (auto pRecord : vRecords)
			{
				if (F::EnginePrediction.m_flOldCurrentTime - pRecord->m_flSimTime > flLerp)
				{
					aBones = pRecord->m_aBones;
					break;
				}
			}
		}
	}

	if(!aBones)
		return false;

	float flMinFOV = 180.f;
	for (int nHitbox = 0; nHitbox < pTarget->GetNumOfHitboxes(); nHitbox++)
	{
		if (!IsHitboxValid(pTarget, nHitbox, iHitboxes))
			continue;

		Vec3 vCurPos = pTarget->GetHitboxCenter(aBones, nHitbox);
		Vec3 vCurAngleTo = Math::CalcAngle(vLocalPos, vCurPos);
		float flCurFOVTo = Math::CalcFov(vLocalAngles, vCurAngleTo);

		if (flCurFOVTo < flMinFOV)
		{
			vPos = vCurPos;
			vAngleTo = vCurAngleTo;
			flFOVTo = flMinFOV = flCurFOVTo;
		}
	}

	return flMinFOV < aimFOV;
}

bool CAimbotGlobal::IsHitboxValid(CBaseEntity* pEntity, int nHitbox, int iHitboxes)
{
	switch (pEntity->GetHitboxToBase(nHitbox))
	{
	case -1: return true;
	case HITBOX_HEAD: return iHitboxes & Vars::Aimbot::Hitscan::HitboxesEnum::Head;
	case HITBOX_SPINE0:
	case HITBOX_SPINE1:
	case HITBOX_SPINE2:
	case HITBOX_SPINE3: return iHitboxes & Vars::Aimbot::Hitscan::HitboxesEnum::Body;
	case HITBOX_PELVIS: return iHitboxes & Vars::Aimbot::Hitscan::HitboxesEnum::Pelvis;
	case HITBOX_LEFT_UPPERARM:
	case HITBOX_LEFT_FOREARM:
	case HITBOX_LEFT_HAND:
	case HITBOX_RIGHT_UPPERARM:
	case HITBOX_RIGHT_FOREARM:
	case HITBOX_RIGHT_HAND: return iHitboxes & Vars::Aimbot::Hitscan::HitboxesEnum::Arms;
	case HITBOX_LEFT_THIGH:
	case HITBOX_LEFT_CALF:
	case HITBOX_LEFT_FOOT:
	case HITBOX_RIGHT_THIGH:
	case HITBOX_RIGHT_CALF:
	case HITBOX_RIGHT_FOOT: return iHitboxes & Vars::Aimbot::Hitscan::HitboxesEnum::Legs;
	}

	return false;
}

bool CAimbotGlobal::ShouldMultipoint(CBaseEntity* pEntity, int nHitbox, int iHitboxes)
{
	if (Vars::Aimbot::Hitscan::MultipointScale.Value <= 0.f)
		return false;

	if (!iHitboxes)
		return true;

	switch (pEntity->GetHitboxToBase(nHitbox))
	{
	case -1: return true;
	case HITBOX_HEAD: return iHitboxes & Vars::Aimbot::Hitscan::HitboxesEnum::Head;
	case HITBOX_SPINE0:
	case HITBOX_SPINE1:
	case HITBOX_SPINE2:
	case HITBOX_SPINE3: return iHitboxes & Vars::Aimbot::Hitscan::HitboxesEnum::Body;
	case HITBOX_PELVIS: return iHitboxes & Vars::Aimbot::Hitscan::HitboxesEnum::Pelvis;
	case HITBOX_LEFT_UPPERARM:
	case HITBOX_LEFT_FOREARM:
	case HITBOX_LEFT_HAND:
	case HITBOX_RIGHT_UPPERARM:
	case HITBOX_RIGHT_FOREARM:
	case HITBOX_RIGHT_HAND: return iHitboxes & Vars::Aimbot::Hitscan::HitboxesEnum::Arms;
	case HITBOX_LEFT_THIGH:
	case HITBOX_LEFT_CALF:
	case HITBOX_LEFT_FOOT:
	case HITBOX_RIGHT_THIGH:
	case HITBOX_RIGHT_CALF:
	case HITBOX_RIGHT_FOOT: return iHitboxes & Vars::Aimbot::Hitscan::HitboxesEnum::Legs;
	}

	return false;
}

bool CAimbotGlobal::ShouldIgnore(CBaseEntity* pEntity, CTFPlayer* pLocal, CTFWeaponBase* pWeapon, bool bAutoDetonate)
{
	if (pEntity->IsDormant())
		return true;

	if (auto pGameRules = I::TFGameRules())
	{
		if (pGameRules->m_bTruceActive() && (FriendlyFire() || pLocal->m_iTeamNum() != pEntity->m_iTeamNum()))
			return true;
	}

	switch (pEntity->GetClassID())
	{
	case ETFClassID::CTFPlayer:
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		if (pPlayer == pLocal || !pPlayer->IsAlive() || pPlayer->IsAGhost())
			return true;

		if (!FriendlyFire() && pLocal->m_iTeamNum() == pEntity->m_iTeamNum())
			return false;

		if (F::PlayerUtils.IsIgnored(pPlayer->entindex())
			|| Vars::Aimbot::General::Ignore.Value & Vars::Aimbot::General::IgnoreEnum::Unprioritized && !F::PlayerUtils.IsPrioritized(pPlayer->entindex()))
			return true;

		if (Vars::Aimbot::General::Ignore.Value & Vars::Aimbot::General::IgnoreEnum::Friends && H::Entities.IsFriend(pPlayer->entindex())
			|| Vars::Aimbot::General::Ignore.Value & Vars::Aimbot::General::IgnoreEnum::Party && H::Entities.InParty(pPlayer->entindex())
			|| Vars::Aimbot::General::Ignore.Value & Vars::Aimbot::General::IgnoreEnum::Invulnerable && pPlayer->IsInvulnerable() && SDK::AttribHookValue(0, "crit_forces_victim_to_laugh", pWeapon) <= 0
			|| Vars::Aimbot::General::Ignore.Value & Vars::Aimbot::General::IgnoreEnum::Invisible && pPlayer->IsInvisible(Vars::Aimbot::General::IgnoreInvisible.Value / 100.f)
			|| Vars::Aimbot::General::Ignore.Value & Vars::Aimbot::General::IgnoreEnum::DeadRinger && pPlayer->m_bFeignDeathReady()
			|| Vars::Aimbot::General::Ignore.Value & Vars::Aimbot::General::IgnoreEnum::Taunting && pPlayer->IsTaunting()
			|| Vars::Aimbot::General::Ignore.Value & Vars::Aimbot::General::IgnoreEnum::Disguised && pPlayer->InCond(TF_COND_DISGUISED))
			return true;
		if (Vars::Aimbot::General::Ignore.Value & Vars::Aimbot::General::IgnoreEnum::Vaccinator)
		{
			switch (G::PrimaryWeaponType)
			{
			case EWeaponType::HITSCAN:
				if (pPlayer->InCond(TF_COND_MEDIGUN_UBER_BULLET_RESIST) && SDK::AttribHookValue(0, "mod_pierce_resists_absorbs", pWeapon) != 0)
					return true;
				break;
			case EWeaponType::PROJECTILE:
				switch (pWeapon->GetWeaponID())
				{
				case TF_WEAPON_FLAMETHROWER:
				case TF_WEAPON_FLAREGUN:
					if (pPlayer->InCond(TF_COND_MEDIGUN_UBER_FIRE_RESIST))
						return true;
					break;
				case TF_WEAPON_COMPOUND_BOW:
					if (pPlayer->InCond(TF_COND_MEDIGUN_UBER_BULLET_RESIST))
						return true;
					break;
				default:
					if (pPlayer->InCond(TF_COND_MEDIGUN_UBER_BLAST_RESIST))
						return true;
				}
			}
		}

		return false;
	}
	case ETFClassID::CObjectSentrygun:
	{
		if (bAutoDetonate && !(Vars::Aimbot::Projectile::AutoDetonate.Value & Vars::Aimbot::Projectile::AutoDetonateEnum::Sentry))
			return true;

		int weaponID = pWeapon->GetWeaponID();
		if (pLocal->m_iTeamNum() == pEntity->m_iTeamNum() && (weaponID == TF_WEAPON_WRENCH || weaponID == TF_WEAPON_FIREAXE || weaponID == TF_WEAPON_SHOTGUN_BUILDING_RESCUE))
			return false; // let melee aimbot deal with this stuff

		if (!(Vars::Aimbot::General::Target.Value & Vars::Aimbot::General::TargetEnum::Sentry))
			return true;

		return false;
	}
	case ETFClassID::CObjectDispenser:
	{
		if (bAutoDetonate && !(Vars::Aimbot::Projectile::AutoDetonate.Value & Vars::Aimbot::Projectile::AutoDetonateEnum::Dispenser))
			return true;

		int weaponID = pWeapon->GetWeaponID();
		if (pLocal->m_iTeamNum() == pEntity->m_iTeamNum() && (weaponID == TF_WEAPON_WRENCH || weaponID == TF_WEAPON_FIREAXE || weaponID == TF_WEAPON_SHOTGUN_BUILDING_RESCUE))
			return false; // let melee aimbot deal with this stuff
	
		if (!(Vars::Aimbot::General::Target.Value & Vars::Aimbot::General::TargetEnum::Dispenser))
			return true;

		return false;
	}
	case ETFClassID::CObjectTeleporter:
	{
		if (bAutoDetonate && !(Vars::Aimbot::Projectile::AutoDetonate.Value & Vars::Aimbot::Projectile::AutoDetonateEnum::Teleporter))
			return true;

		int weaponID = pWeapon->GetWeaponID();
		if (pLocal->m_iTeamNum() == pEntity->m_iTeamNum() && (weaponID == TF_WEAPON_WRENCH || weaponID == TF_WEAPON_FIREAXE || weaponID == TF_WEAPON_SHOTGUN_BUILDING_RESCUE))
			return false; // let melee aimbot deal with this stuff

		if (!(Vars::Aimbot::General::Target.Value & Vars::Aimbot::General::TargetEnum::Teleporter))
			return true;
		
		return false;
	}
	case ETFClassID::CTFGrenadePipebombProjectile:
	{
		auto pGrenade = pEntity->As<CTFGrenadePipebombProjectile>();
		if (pGrenade->m_iType() != TF_GL_MODE_REMOTE_DETONATE || pLocal->m_iTeamNum() == pEntity->m_iTeamNum())
			return true;

		if (bAutoDetonate)
		{
			if (!(Vars::Aimbot::Projectile::AutoDetonate.Value & Vars::Aimbot::Projectile::AutoDetonateEnum::PlayerStickies))
				return true;

			auto pSecondary = pLocal->GetWeaponFromSlot(SLOT_SECONDARY);
			if(pSecondary && SDK::AttribHookValue(0.f, "stickies_detonate_stickies", pSecondary) == 0.f)
				return true;
		}
		else if (!(Vars::Aimbot::General::Target.Value & Vars::Aimbot::General::TargetEnum::Stickies) ||
			!pGrenade->m_bTouched() || pEntity->m_vecOrigin().DistTo(pLocal->m_vecOrigin()) > Vars::Aimbot::General::StickyShootRange.Value ||
			pWeapon->GetWeaponID() == TF_WEAPON_PIPEBOMBLAUNCHER && SDK::AttribHookValue(0.f, "stickies_detonate_stickies", pWeapon) == 0.f)
			return true;

		return false;
	}
	case ETFClassID::CTFBaseBoss:
	case ETFClassID::CTFTankBoss:
	case ETFClassID::CEyeballBoss:
	case ETFClassID::CHeadlessHatman:
	case ETFClassID::CMerasmus:
	case ETFClassID::CZombie:
	{
		if (!(Vars::Aimbot::General::Target.Value & Vars::Aimbot::General::TargetEnum::NPCs))
			return true;

		if(pEntity->GetClassID() == ETFClassID::CEyeballBoss
			? pLocal->m_iTeamNum() != TF_TEAM_HALLOWEEN
			: pLocal->m_iTeamNum() == pEntity->m_iTeamNum())
			return true;

		return false;
	}
	case ETFClassID::CTFPumpkinBomb:
	case ETFClassID::CTFGenericBomb:
	{
		if (!(Vars::Aimbot::General::Target.Value & Vars::Aimbot::General::TargetEnum::Bombs))
			return true;

		return false;
	}
	}

	return true;
}

int CAimbotGlobal::GetPriority(int iIndex)
{
	return F::PlayerUtils.GetPriority(iIndex);
}

bool CAimbotGlobal::FriendlyFire()
{
	static auto mp_friendlyfire = U::ConVars.FindVar("mp_friendlyfire");
	return mp_friendlyfire->GetBool();
}

bool CAimbotGlobal::ShouldAim()
{
	switch (Vars::Aimbot::General::AimType.Value)
	{
	case Vars::Aimbot::General::AimTypeEnum::Plain:
	case Vars::Aimbot::General::AimTypeEnum::Silent:
		if (!G::CanPrimaryAttack && !G::Reloading && !F::Ticks.IsTimingUnsure())
			return false;
	}

	return true;
}

bool CAimbotGlobal::ShouldHoldAttack(CTFWeaponBase* pWeapon)
{
	switch (Vars::Aimbot::General::AimHoldsFire.Value)
	{
	case Vars::Aimbot::General::AimHoldsFireEnum::MinigunOnly:
		if (pWeapon->GetWeaponID() != TF_WEAPON_MINIGUN)
			break;
		[[fallthrough]];
	case Vars::Aimbot::General::AimHoldsFireEnum::Always:
		if (!F::Aimbot.m_bRunningSecondary && !G::CanPrimaryAttack && G::LastUserCmd->buttons & IN_ATTACK && Vars::Aimbot::General::AimType.Value && !pWeapon->IsInReload())
			return true;
	}
	return false;
}

// will not predict for projectile weapons
bool CAimbotGlobal::ValidBomb(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CBaseEntity* pBomb, bool bCheckWeaponType)
{
	if (bCheckWeaponType && G::PrimaryWeaponType == EWeaponType::PROJECTILE)
		return false;

	Vec3 vOrigin = pBomb->m_vecOrigin();

	CBaseEntity* pEntity;
	for (CEntitySphereQuery sphere(vOrigin, 300.f);
		pEntity = sphere.GetCurrentEntity();
		sphere.NextEntity())
	{
		if (pEntity == pLocal || pEntity->IsPlayer() && (!pEntity->As<CTFPlayer>()->IsAlive() || pEntity->As<CTFPlayer>()->IsAGhost()) 
			|| !FriendlyFire() && pEntity->m_iTeamNum() == pLocal->m_iTeamNum())
			continue;

		Vec3 vPos; reinterpret_cast<CCollisionProperty*>(pEntity->GetCollideable())->CalcNearestPoint(vOrigin, &vPos);
		if (vOrigin.DistTo(vPos) > 300.f)
			continue;

		if (pEntity->IsPlayer() || pEntity->IsBuilding() || pEntity->IsNPC())
		{
			if (ShouldIgnore(pEntity->As<CTFPlayer>(), pLocal, pWeapon))
				continue;

			if (!SDK::VisPosCollideable(pBomb, pEntity, vOrigin, pEntity->IsPlayer() ? pEntity->m_vecOrigin() + pEntity->As<CTFPlayer>()->GetViewOffset() : pEntity->GetCenter(), MASK_SHOT))
				continue;

			return true;
		}
	}

	return false;
}
