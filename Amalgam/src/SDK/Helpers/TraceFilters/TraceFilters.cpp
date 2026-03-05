#include "TraceFilters.h"

#include "../../SDK.h"

bool CTraceFilterHitscan::ShouldHitEntity(IHandleEntity* pServerEntity, int nContentsMask)
{
	if (!pServerEntity || pServerEntity == pSkip)
		return false;

	auto pEntity = reinterpret_cast<CBaseEntity*>(pServerEntity);
	if (iTeam == -1) iTeam = pSkip ? pSkip->m_iTeamNum() : 0;
	if (iType != SKIP_CHECK && !vWeapons.empty())
	{
		if (auto pWeapon = pSkip && pSkip->IsPlayer() ? pSkip->As<CTFPlayer>()->m_hActiveWeapon()->As<CTFWeaponBase>() : nullptr)
		{
			int iWeaponID = pWeapon->GetWeaponID();
			bWeapon = std::find(vWeapons.begin(), vWeapons.end(), iWeaponID) != vWeapons.end();
		}
		vWeapons.clear();
	}

	switch (pEntity->GetClassID())
	{
	case ETFClassID::CTFAmmoPack:
	case ETFClassID::CFuncAreaPortalWindow:
	case ETFClassID::CFuncRespawnRoomVisualizer:
	case ETFClassID::CTFReviveMarker: return false;
	case ETFClassID::CTFMedigunShield: return pEntity->m_iTeamNum() != iTeam;
	case ETFClassID::CTFPlayer:
	case ETFClassID::CBaseObject:
	case ETFClassID::CObjectSentrygun:
	case ETFClassID::CObjectDispenser:
	case ETFClassID::CObjectTeleporter:
	{
		if (iType != SKIP_CHECK && (iWeapon == WEAPON_INCLUDE ? bWeapon : !bWeapon))
			return iType == FORCE_HIT ? true : false;
		return pEntity->m_iTeamNum() != iTeam;
	}
	}

	return true;
}
TraceType_t CTraceFilterHitscan::GetTraceType() const
{
	return TRACE_EVERYTHING;
}

bool CTraceFilterCollideable::ShouldHitEntity(IHandleEntity* pServerEntity, int nContentsMask)
{
	if (!pServerEntity || pServerEntity == pSkip)
		return false;

	auto pEntity = reinterpret_cast<CBaseEntity*>(pServerEntity);
	if (iTeam == -1) iTeam = pSkip ? pSkip->m_iTeamNum() : 0;
	if (iType != SKIP_CHECK && !vWeapons.empty())
	{
		if (auto pWeapon = pSkip && pSkip->IsPlayer() ? pSkip->As<CTFPlayer>()->m_hActiveWeapon()->As<CTFWeaponBase>() : nullptr)
		{
			int iWeaponID = pWeapon->GetWeaponID();
			bWeapon = std::find(vWeapons.begin(), vWeapons.end(), iWeaponID) != vWeapons.end();
		}
		vWeapons.clear();
	}

	switch (pEntity->GetClassID())
	{
	case ETFClassID::CBaseEntity:
	case ETFClassID::CBaseDoor:
	case ETFClassID::CDynamicProp:
	case ETFClassID::CPhysicsProp:
	case ETFClassID::CPhysicsPropMultiplayer:
	case ETFClassID::CObjectCartDispenser:
	case ETFClassID::CFunc_LOD:
	case ETFClassID::CFuncTrackTrain:
	case ETFClassID::CFuncConveyor: 
	case ETFClassID::CTFGenericBomb:
	case ETFClassID::CTFPumpkinBomb: return true;
	case ETFClassID::CFuncRespawnRoomVisualizer:
		if (nContentsMask & CONTENTS_PLAYERCLIP)
			return pEntity->m_iTeamNum() != iTeam;
		break;
	case ETFClassID::CTFMedigunShield:
		if (!(nContentsMask & CONTENTS_PLAYERCLIP))
			return pEntity->m_iTeamNum() != iTeam;
		break;
	case ETFClassID::CTFPlayer:
	{
		if (iPlayer == PLAYER_ALL)
			return true;
		if (iPlayer == PLAYER_NONE)
			return false;
		if (iType != SKIP_CHECK && (iWeapon == WEAPON_INCLUDE ? bWeapon : !bWeapon))
			return iType == FORCE_HIT ? true : false;
		return pEntity->m_iTeamNum() != iTeam;
	}
	case ETFClassID::CBaseObject:
	case ETFClassID::CObjectSentrygun:
	case ETFClassID::CObjectDispenser: return iObject == OBJECT_ALL ? true : iObject == OBJECT_NONE ? false : pEntity->m_iTeamNum() != iTeam;
	case ETFClassID::CObjectTeleporter: return true;
	//case ETFClassID::CTFBaseBoss:
	//case ETFClassID::CTFTankBoss:
	//case ETFClassID::CMerasmus:
	//case ETFClassID::CEyeballBoss:
	//case ETFClassID::CHeadlessHatman:
	//case ETFClassID::CZombie:
	case ETFClassID::CTFGrenadePipebombProjectile:
		return bMisc ? true : false;
	}

	return false;
}
TraceType_t CTraceFilterCollideable::GetTraceType() const
{
	return TRACE_EVERYTHING;
}

bool CTraceFilterWorldAndPropsOnly::ShouldHitEntity(IHandleEntity* pServerEntity, int nContentsMask)
{
	if (!pServerEntity || pServerEntity == pSkip)
		return false;
	if (pServerEntity->GetRefEHandle().GetSerialNumber() == (1 << 15))
		return pServerEntity->GetRefEHandle().GetEntryIndex() != iTeam; // just use team variable since cliententitylist can give us nullptrs for some props for whatever reason

	auto pEntity = reinterpret_cast<CBaseEntity*>(pServerEntity);
	if (iTeam == -1) iTeam = pSkip ? pSkip->m_iTeamNum() : 0;

	switch (pEntity->GetClassID())
	{
	case ETFClassID::CBaseEntity:
	case ETFClassID::CBaseDoor:
	case ETFClassID::CDynamicProp:
	case ETFClassID::CPhysicsProp:
	case ETFClassID::CPhysicsPropMultiplayer:
	case ETFClassID::CFunc_LOD:
	case ETFClassID::CObjectCartDispenser:
	case ETFClassID::CFuncTrackTrain:
	case ETFClassID::CFuncConveyor: return true;
	case ETFClassID::CFuncRespawnRoomVisualizer:
		if (nContentsMask & CONTENTS_PLAYERCLIP)
			return pEntity->m_iTeamNum() != iTeam;
	}

	return false;
}
TraceType_t CTraceFilterWorldAndPropsOnly::GetTraceType() const
{
	return TRACE_EVERYTHING_FILTER_PROPS;
}

#define MOVEMENT_COLLISION_GROUP 8
#define RED_CONTENTS_MASK 0x800
#define BLU_CONTENTS_MASK 0x1000

bool CTraceFilterNavigation::ShouldHitEntity(IHandleEntity* pServerEntity, int nContentsMask)
{
	if (!pServerEntity)
		return false;

	auto pEntity = reinterpret_cast<CBaseEntity*>(pServerEntity);

	if (pEntity->entindex() != 0 && pEntity->GetClassID() != ETFClassID::CBaseEntity)
		return false;

	if (pEntity->GetClassID() == ETFClassID::CFuncRespawnRoomVisualizer)
	{
		auto pLocal = H::Entities.GetLocal();
		const int iTargetTeam = pEntity->m_iTeamNum();
		const int iLocalTeam = pLocal ? pLocal->m_iTeamNum() : iTargetTeam;

		if (!pEntity->ShouldCollide(MOVEMENT_COLLISION_GROUP, iLocalTeam == TF_TEAM_RED ? RED_CONTENTS_MASK : BLU_CONTENTS_MASK))
			return true;
	}

	return true;
}

TraceType_t CTraceFilterNavigation::GetTraceType() const
{
	return TRACE_EVERYTHING;
}

static bool StandardFilterRules(IHandleEntity* pHandleEntity, int fContentsMask)
{
	CBaseEntity* pCollide = I::ClientEntityList->GetClientEntityFromHandle(pHandleEntity->GetRefEHandle())->As<CBaseEntity>();

	if (!pCollide)
		return true;

	SolidType_t solid = (SolidType_t)pCollide->m_nSolidType();
	const model_t* pModel = pCollide->GetModel();

	if ((I::ModelInfoClient->GetModelType(pModel) != mod_brush) || (solid != SOLID_BSP && solid != SOLID_VPHYSICS))
	{
		if ((fContentsMask & CONTENTS_MONSTER) == 0)
			return false;
	}

	if (!(fContentsMask & CONTENTS_WINDOW) && pCollide->IsTransparent())
		return false;

	if (!(fContentsMask & CONTENTS_MOVEABLE) && (pCollide->m_MoveType() == MOVETYPE_PUSH))
		return false;

	return true;
}

CTraceFilterSimple::CTraceFilterSimple(const IHandleEntity* passedict, int collisionGroup,
	ShouldHitFunc_t pExtraShouldHitFunc)
{
	m_pPassEnt = passedict;
	m_collisionGroup = collisionGroup;
	m_pExtraShouldHitCheckFunction = pExtraShouldHitFunc;
}

bool CTraceFilterSimple::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
{
	if (!StandardFilterRules(pHandleEntity, contentsMask))
		return false;

	CBaseEntity* pEntity = I::ClientEntityList->GetClientEntityFromHandle(pHandleEntity->GetRefEHandle())->As<CBaseEntity>();
	if (!pEntity)
		return false;

	if (m_pPassEnt)
	{
		CBaseEntity* pEntity2 = I::ClientEntityList->GetClientEntityFromHandle(m_pPassEnt->GetRefEHandle())->As<CBaseEntity>();
		if (!pEntity2 || pHandleEntity == m_pPassEnt 
			|| pEntity->m_hOwnerEntity() == pEntity2
			|| pEntity2->m_hOwnerEntity() == pEntity)
			return false;
	}

	if (!pEntity->ShouldCollide(m_collisionGroup, contentsMask))
		return false;
	if (m_pExtraShouldHitCheckFunction &&
		(!(m_pExtraShouldHitCheckFunction(pHandleEntity, contentsMask))))
		return false;

	return true;
}

TraceType_t CTraceFilterSimple::GetTraceType() const
{
	return TRACE_EVERYTHING;
}

CTargetOnlyFilter::CTargetOnlyFilter(CBaseEntity* pShooter, CBaseEntity* pTarget)
	: CTraceFilterSimple(pShooter, COLLISION_GROUP_NONE)
{
	m_pShooter = pShooter;
	m_pTarget = pTarget;
}

static inline bool IsBSPModel(CBaseEntity* pEntity) // im lazy
{
	if (pEntity->m_nSolidType() == SOLID_BSP)
		return true;

	const model_t* pModel = I::ModelInfoClient->GetModel(pEntity->m_nModelIndex());
	if (pEntity->m_nSolidType() == SOLID_VPHYSICS && I::ModelInfoClient->GetModelType(pModel) == mod_brush)
		return true;

	return false;
}

bool CTargetOnlyFilter::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
{
	CBaseEntity* pEnt = static_cast<CBaseEntity*>(pHandleEntity);

	if (pEnt && pEnt == m_pTarget)
		return true;
	else if (!pEnt || pEnt != m_pTarget)
	{
		// If we hit a solid piece of the world, we're done.
		if (IsBSPModel(pEnt) && (pEnt->m_Collision()->m_nSolidType != SOLID_NONE && (pEnt->m_Collision()->m_usSolidFlags & FSOLID_NOT_SOLID) == 0))
			return CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask);
		return false;
	}
	else
		return CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask);
}