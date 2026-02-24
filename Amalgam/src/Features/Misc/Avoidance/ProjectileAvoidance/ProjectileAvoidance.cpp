#include "ProjectileAvoidance.h"

#include "../../../Simulation/MovementSimulation/MovementSimulation.h"
#include "../../../Simulation/ProjectileSimulation/ProjectileSimulation.h"
#include "../../../Aimbot/AimbotProjectile/AimbotProjectile.h"
#include "../../../Ticks/Ticks.h"

static inline bool ShouldIgnore(CBaseEntity* pEntity)
{
	if(pEntity->GetClassID() == ETFClassID::CTFGrenadePipebombProjectile)
		return pEntity->GetAbsVelocity().IsZero() || pEntity->As<CTFGrenadePipebombProjectile>()->m_bTouched();

	return pEntity->GetAbsVelocity().IsZero();
}

static Vec3 GetRotatedPosition(Vec3 vStart, const float flRotation, const float flDistance)
{
	const auto rad = DEG2RAD(flRotation);
	vStart.x += cosf(rad) * flDistance;
	vStart.y += sinf(rad) * flDistance;

	return vStart;
}

static float ta(float a)
{
	while (a > 180)
		a -= 360;
	while (a < -180)
		a += 360;

	return a;
}

static float DistanceTo(Vec3 s, Vec3 d, Vec3 mins, Vec3 maxs)
{
	CGameTrace tr;
	CTraceFilterWorldAndPropsOnly filter;
	SDK::TraceHull(s, d, mins, maxs, MASK_SOLID, &filter, &tr);
	return tr.endpos.DistTo(s);
}


static Vec3 VelocityToAngles(const Vec3& direction)
{
	auto Magnitude = [&](const Vec3& v) -> float {
		return sqrtf(v.Dot(v));
		};

	float yaw, pitch;

	if (direction.y == 0.0f && direction.x == 0.0f)
	{
		yaw = 0.0f;

		if (direction.z > 0.0f)
			pitch = 270.0f;

		else pitch = 90.0f;
	}
	else
	{
		yaw = RAD2DEG(atan2f(direction.y, direction.x));
		pitch = RAD2DEG(atan2f(-direction.z, Magnitude(Vec3(direction))));

		if (yaw < 0.0f)
			yaw += 360.0f;

if (pitch < 0.0f)
	pitch += 360.0f;
	}

	return { pitch, yaw, 0.0f };
}

static inline bool WithinAABox(Vec3 toCheck, Vec3 boxmin, Vec3 boxmax)
{
	return (toCheck.x >= boxmin.x) && (toCheck.x <= boxmax.x) &&
		(toCheck.y >= boxmin.y) && (toCheck.y <= boxmax.y) &&
		(toCheck.z >= boxmin.z) && (toCheck.z <= boxmax.z);
}

void CProjectileAvoidance::Run(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!Vars::Misc::Movement::ProjectileAvoidance.Value)
	{
		updateIn = 0.0f;
		return;
	}

	auto vProjectiles = H::Entities.GetGroup(EntityEnum::WorldProjectile);

	const int iLocalTeam = pLocal->m_iTeamNum();
	for (auto it = vProjectiles.begin(); it != vProjectiles.end();)
	{
		if ((*it)->m_iTeamNum() == iLocalTeam)
			it = vProjectiles.erase(it);
		else
			++it;
	}
	if (vProjectiles.empty())
		return;

	auto velocity = pLocal->GetAbsVelocity();
	Vector mins = pLocal->m_vecMins();
	Vector maxs = pLocal->m_vecMaxs();
	Vector center = pLocal->GetCenter();

	if (velocity.IsZero()) {
		mins += pLocal->GetAbsOrigin();
		maxs += pLocal->GetAbsOrigin();
	}
	else {
		MoveStorage tStorage{};
		F::MoveSim.Initialize(pLocal, tStorage, false, true);
		F::MoveSim.RunTick(tStorage);
		const auto backup_path = tStorage.m_vPath;
		Vector lastPos;
		for (auto i = 0; i < TIME_TO_TICKS((pLocal->CalculateMaxSpeed() / 1200.0f)); i += TIME_TO_TICKS(0.06f))
		{
			F::MoveSim.RunTick(tStorage);
			lastPos = tStorage.m_vPredictedOrigin;
		}
		vPath = backup_path;

		mins += lastPos;
		maxs += lastPos;
		center = (pLocal->GetCenter() - pLocal->GetAbsOrigin()) + lastPos;
	}

	QAngle optimalAngle = QAngle(0, 0, 0);
	bool bUseWarp = false;
	bool bDiscard = true;

	for (auto pEntity : vProjectiles)
	{
		if (ShouldIgnore(pEntity))
			continue;

		float flOptimalDistance = pEntity->GetAbsVelocity().Length() * (1000.0f - pLocal->CalculateMaxSpeed());
		float flWarpDistance = pEntity->GetAbsVelocity().Length() * ((1000.0f - pLocal->CalculateMaxSpeed()) * (Vars::Misc::Movement::ProjectileAvoidanceWarpDistance.Value / 100.0f));
		float flCurrentDistance = center.DistTo(pEntity->GetAbsOrigin());
		float splashRadius = F::AimbotProjectile.GetSplashRadius(pEntity);
		bool bIgnore = true;

		if (flCurrentDistance < flOptimalDistance)
		{
			if (updateIn > I::GlobalVars->curtime)
				break;

			ProjectileInfo projInfo;
			F::ProjSim.GetInfo(pEntity, projInfo);
			if (F::ProjSim.Initialize(projInfo, true, true))
			{
				Vec3 finalPos = {};
				for (int n = 1; n <= TIME_TO_TICKS(projInfo.m_flLifetime); n++)
				{
					Vec3 Old = F::ProjSim.GetOrigin();
					F::ProjSim.RunTick(projInfo);
					Vec3 New = F::ProjSim.GetOrigin();

					if (WithinAABox(projInfo.m_vPath.back(), mins, maxs))
					{
						CGameTrace trace{};
						CTraceFilterCollideable filter{}; filter.pSkip = pEntity->As<CBaseProjectile>()->m_hOriginalLauncher().Get();
						int nMask = MASK_SOLID;
						SDK::TraceHull(Old, New, projInfo.m_vHull * -1, projInfo.m_vHull, nMask, &filter, &trace);
						F::ProjSim.SetupTrace(filter, nMask, pEntity);

						if (!trace.DidHit())
						{
							bIgnore = false;
							bDiscard = false;
							break;
						}
					}
				}
			}

			if (projInfo.m_vPath.empty())
				return;

			if (bIgnore)
				continue;

			float CheckOne = GetRotatedPosition(pEntity->GetAbsOrigin(), VelocityToAngles(pEntity->GetAbsVelocity()).y + 90.0f, 50.0f).DistTo(center);
			float CheckTwo = GetRotatedPosition(pEntity->GetAbsOrigin(), VelocityToAngles(pEntity->GetAbsVelocity()).y - 90.0f, 50.0f).DistTo(center);
			float CheckOneDist = DistanceTo(pLocal->GetAbsOrigin(), GetRotatedPosition(pLocal->GetAbsOrigin(), VelocityToAngles(pEntity->GetAbsVelocity()).y + 90.0f, 50.0f), pLocal->m_vecMins(), pLocal->m_vecMaxs());
			float CheckTwoDist = DistanceTo(pLocal->GetAbsOrigin(), GetRotatedPosition(pLocal->GetAbsOrigin(), VelocityToAngles(pEntity->GetAbsVelocity()).y - 90.0f, 50.0f), pLocal->m_vecMins(), pLocal->m_vecMaxs());
			float angle = VelocityToAngles(pEntity->GetAbsVelocity()).y + (CheckOne <= CheckTwo ? CheckOneDist <= CheckTwoDist ? -90.0f : 90.0f : CheckOneDist <= CheckTwoDist ? 90.0f : -90.0f);

			angle = ta(angle);

			optimalAngle.x += cos(DEG2RAD(angle));
			optimalAngle.y += sin(DEG2RAD(angle));
		}

		if (flCurrentDistance < flWarpDistance)
			bUseWarp = true;
	}

	if (updateIn > I::GlobalVars->curtime)
	{
		pCmd->forwardmove = -cos(DEG2RAD(ta(optimal - pLocal->EyeAngles().y))) * 450.0f;
		pCmd->sidemove = sin(DEG2RAD(ta(optimal - pLocal->EyeAngles().y))) * 450.0f;
		return;
	}

	if (bDiscard)
	{
		if (Vars::Misc::Movement::ProjectileAvoidanceUseWarp.Value)
		{
			if (!F::Ticks.m_iShiftedTicks || F::Ticks.m_bDoubletap || F::Ticks.m_bRecharge || F::Ticks.m_bSpeedhack)
				return;

			F::Ticks.m_bWarp = true;
		}
		return;
	}

	optimal = RAD2DEG(atan2(optimalAngle.y, optimalAngle.x));
	updateIn = I::GlobalVars->curtime + 0.09f;

	pCmd->forwardmove = -cos(DEG2RAD(ta(optimal - pLocal->EyeAngles().y))) * 450.0f;
	pCmd->sidemove = sin(DEG2RAD(ta(optimal - pLocal->EyeAngles().y))) * 450.0f;

	if (bUseWarp && Vars::Misc::Movement::ProjectileAvoidanceUseWarp.Value)
	{
		if (!F::Ticks.m_iShiftedTicks || F::Ticks.m_bDoubletap || F::Ticks.m_bRecharge || F::Ticks.m_bSpeedhack)
			return;

		F::Ticks.m_bWarp = true;
	}
}