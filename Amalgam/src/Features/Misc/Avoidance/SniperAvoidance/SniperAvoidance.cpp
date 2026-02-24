#include "SniperAvoidance.h"

#include "../../../Backtrack/Backtrack.h"
#include "../../../Players/PlayerUtils.h"

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

static inline bool IsLooking(CTFPlayer* pSniper, Vec3 laserStart, Vec3 laserDir, CTFPlayer* pLocal)
{
	matrix3x4 aBones[MAXSTUDIOBONES];
	if (!pLocal->SetupBones(aBones, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, I::GlobalVars->curtime))
		return false;

	auto pModel = pLocal->GetModel();
	if (!pModel) return false;

	auto pHDR = I::ModelInfoClient->GetStudiomodel(pModel);
	if (!pHDR) return false;

	auto pSet = pHDR->pHitboxSet(pLocal->m_nHitboxSet());
	if (!pSet) return false;

	for (int i = 0; i < pSet->numhitboxes; i++)
	{
		const auto pHitbox = pSet->pHitbox(i);
		if (!pHitbox) continue;

		const auto bone = pHitbox->bone;
		const auto& matrix = aBones[bone];

		if (Math::RayToOBB(laserStart, laserDir, pHitbox->bbmin * 1.2f, pHitbox->bbmax * 1.2f, matrix))
			return true;
	}

	return false;
}

void CSniperAvoidance::Run(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!Vars::Misc::Movement::SniperAvoidance.Value)
	{
		updateIn = 0.0f;
		return;
	}

	const Vec3 vMins = pLocal->m_vecMins();
	const Vec3 vMaxs = pLocal->m_vecMaxs();
	const Vec3 vCenter = pLocal->GetCenter();

	for (auto& pEntity : H::Entities.GetGroup(EntityEnum::PlayerEnemy))
	{
		const auto pSniper = pEntity->As<CTFPlayer>();

		if (!pSniper || pSniper->m_iClass() != TF_CLASS_SNIPER || pSniper == pLocal)
			continue;

		int iIndex = pSniper->entindex();
		auto pWeapon = pSniper->m_hActiveWeapon()->As<CTFWeaponBase>();
		if (pSniper->IsDormant() || !pSniper->IsAlive() || pSniper->IsAGhost() || !pSniper->InCond(TF_COND_AIMING) ||
			!pWeapon || pWeapon->GetWeaponID() == TF_WEAPON_COMPOUND_BOW || F::PlayerUtils.HasTag(iIndex, CHEATER_TAG)
			|| H::Entities.IsFriend(iIndex) || H::Entities.InParty(iIndex))
			continue;

		Vec3 vShootPos = pSniper->m_vecOrigin() + pSniper->GetViewOffset();
		Vec3 vEyeAngles = H::Entities.GetEyeAngles(iIndex);

		Vec3 vForward; Math::AngleVectors(vEyeAngles, &vForward);
		Vec3 vShootEnd = vShootPos + (vForward * 8192.f);

		if (!SDK::VisPosWorld(pLocal, pSniper, pLocal->m_vecOrigin() + pLocal->GetViewOffset(), vShootPos))
			continue;

		if (!IsLooking(pSniper, vShootPos, vForward, pLocal) 
			&& !Math::RayToOBB(vShootPos, vForward * 8192.f, vMins * 1.2f, vMaxs * 1.2f, pLocal->RenderableToWorldTransform()))
			continue;

		const float flSniperYaw = vEyeAngles.y;
		const Vec3 vLeftOffset = GetRotatedPosition(pLocal->GetAbsOrigin(), flSniperYaw + 90.0f, 50.0f);
		const Vec3 vRightOffset = GetRotatedPosition(pLocal->GetAbsOrigin(), flSniperYaw - 90.0f, 50.0f);

		bool canLeft = false, canRight = false;
		// wall checks
		{
			CGameTrace trLeft;
			CTraceFilterWorldAndPropsOnly leftFilter{};
			SDK::TraceHull(vCenter, vLeftOffset, vMins, vMaxs, MASK_SOLID, &leftFilter, &trLeft);
			if (!trLeft.DidHit())
				canLeft = true;
		}
		{
			CGameTrace trRight;
			CTraceFilterWorldAndPropsOnly rightFilter{};
			SDK::TraceHull(vCenter, vRightOffset, vMins, vMaxs, MASK_SOLID, &rightFilter, &trRight);
			if (!trRight.DidHit())
				canRight = true;
		}
		// sniper checks
		auto DistancePointToLine = [](const Vec3& point, const Vec3& rayOrigin, const Vec3& rayDir) -> float
			{
				Vec3 toPoint = point - rayOrigin;
				float projLength = toPoint.Dot(rayDir);
				if (projLength < 0)
					return toPoint.Length();

				Vec3 projPoint = rayOrigin + rayDir * projLength;
				return (point - projPoint).Length();
			};
		bool canGoRight, canGoLeft;

		float distLeft = DistancePointToLine(vLeftOffset, vShootPos, vForward);
		canGoLeft = canLeft && distLeft > 50.f;

		float distRight = DistancePointToLine(vRightOffset, vShootPos, vForward);
		canGoRight = canRight && distRight > 50.f;

		const float speed = 450.f;
		Vec3 vEscapeTarget = {};
		bool bHasEscape = false;

		if (!canRight && canGoLeft)
		{
			vEscapeTarget = vLeftOffset;
			bHasEscape = true;
		}
		else if (!canLeft && canGoRight)
		{
			vEscapeTarget = vRightOffset;
			bHasEscape = true;
		}
		else if (canGoLeft && canGoRight)
		{
			vEscapeTarget = (distLeft > distRight) ? vLeftOffset : vRightOffset;
			bHasEscape = true;
		}

		if (bHasEscape)
			SDK::WalkTo(pCmd, pLocal, vEscapeTarget);
	}
}