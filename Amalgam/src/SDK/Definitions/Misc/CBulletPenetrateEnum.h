#pragma once
#include "../Interfaces/IEngineTrace.h"
#include "../Main/CGameTrace.h"
#include "../Main/CBaseEntity.h"

#include "../Main/UtlSortVector.h"

struct penetrated_target_list
{
	CBaseEntity* pTarget;
	float flDistanceFraction;
};

class CBulletPenetrateEnum : public IEntityEnumerator
{
public:
	CBulletPenetrateEnum(Vector vecStart, Vector vecEnd, CBaseEntity* pShooter, int nCustomDamageType, bool bIgnoreTeammates = true)
	{
		m_vecStart = vecStart;
		m_vecEnd = vecEnd;
		m_pShooter = pShooter;
		m_nCustomDamageType = nCustomDamageType;
		m_bIgnoreTeammates = bIgnoreTeammates;
	}

	// We need to sort the penetrated targets into order, with the closest target first
	class PenetratedTargetLess
	{
	public:
		bool Less(const penetrated_target_list& src1, const penetrated_target_list& src2, void* pCtx)
		{
			return src1.flDistanceFraction < src2.flDistanceFraction;
		}
	};

	virtual bool EnumEntity(IHandleEntity* pHandleEntity)
	{
		trace_t tr;

		CBaseEntity* pEnt = static_cast<CBaseEntity*>(pHandleEntity);

		// Ignore collisions with the shooter
		if (pEnt == m_pShooter)
			return true;

		if (pEnt->IsBaseCombatCharacter() || pEnt->IsBaseObject())
		{
			if (m_bIgnoreTeammates && pEnt->m_iTeamNum() == m_pShooter->m_iTeamNum())
				return true;

			Ray_t ray;
			ray.Init(m_vecStart, m_vecEnd);
			I::EngineTrace->ClipRayToEntity(ray, MASK_SOLID | CONTENTS_HITBOX, pHandleEntity, &tr);

			if (tr.fraction < 1.0f)
			{
				penetrated_target_list newEntry;
				newEntry.pTarget = pEnt;
				newEntry.flDistanceFraction = tr.fraction;
				m_Targets.Insert(newEntry);
				return true;
			}
		}

		return true;
	}

public:
	Vector		 m_vecStart;
	Vector		 m_vecEnd;
	int			 m_nCustomDamageType;
	CBaseEntity* m_pShooter;
	bool		 m_bIgnoreTeammates;
	CUtlSortVector<penetrated_target_list, PenetratedTargetLess> m_Targets;
};