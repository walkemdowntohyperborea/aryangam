#include "AutoStickyJump.h"
#include "../../Simulation/MovementSimulation/MovementSimulation.h"

inline Vec3 VelocityToAngles(const Vec3& direction)
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

void CStickyJump::Run(CUserCmd* pCmd)
{
	if (!Vars::Misc::Movement::AutoStickyJump.Value)
		return;

	auto pLocal = H::Entities.GetLocal();
	if (!pLocal || pLocal->deadflag() || pLocal->IsOnGround() || !pLocal->InCond(TF_COND_BLASTJUMPING))
		return;

	auto pWeapon = H::Entities.GetWeapon();
	if (!pWeapon || pWeapon->GetWeaponID() != TF_WEAPON_PIPEBOMBLAUNCHER)
		return;

	MoveStorage tStorage;
	if (!F::MoveSim.Initialize(pLocal, tStorage))
		return;

	int simTicks = TIME_TO_TICKS(SDK::AttribHookValue(0.8f, "sticky_arm_time", pLocal));
	for (int n = 0; n < simTicks; n++)
		F::MoveSim.RunTick(tStorage);

	Vec3 end = tStorage.m_vPredictedOrigin;
	F::MoveSim.Restore(tStorage);

	if (G::Attacking)
	{
		G::SilentAngles = true;

		float pitchOffset = Math::RemapVal(I::EngineClient->GetViewAngles().x, 0.0f, -25.0f, 0.0f, -4.0f);
		if (!pLocal->IsDucking())
		{
			pitchOffset = -3.0f;
		}

		pCmd->viewangles = Math::CalcAngle(pLocal->GetShootPos(), end) + Vec3(pitchOffset, 0.0f, 0.0f);
	}

	// Shoot a sticky
	if (H::Entities.GetGroup(EntityEnum::LocalStickies).empty() && pLocal->m_vecVelocity().z < 500.0f)
	{
		if (pWeapon->As<CTFPipebombLauncher>()->m_flChargeBeginTime() > 0.0f)
			pCmd->buttons &= ~IN_ATTACK;
		else
			pCmd->buttons |= IN_ATTACK;
	}

	for (auto pEntity : H::Entities.GetGroup(EntityEnum::LocalStickies))
	{
		if (!pEntity)
			continue;

		auto sticky = pEntity->As<CTFGrenadePipebombProjectile>();
		if (!sticky || sticky->m_bTouched())
			continue;

		pCmd->forwardmove *= 0.05f;
		pCmd->sidemove *= 0.05f;

		Vec3 stickyVel{};
		sticky->EstimateAbsVelocity(stickyVel);

		// det the sticky
		if (sticky->m_vecOrigin().DistTo(pLocal->m_vecOrigin()) < 350.0f && pLocal->m_vecVelocity().Length2D() > (stickyVel.Length2D() * 1.05f))
			pCmd->buttons |= IN_ATTACK2;

		float slowdownSpeed = Math::RemapVal(I::EngineClient->GetViewAngles().x, 0.0f, -25.0f, 880.0f, 840.0f);
		if (!(pLocal->m_fFlags() & FL_DUCKING))
			slowdownSpeed *= 1.01f;

		// move to sticky
		if (pLocal->m_vecVelocity().Length2D() > slowdownSpeed)
		{
			Vec3 forward{};
			Math::AngleVectors(VelocityToAngles({ pLocal->m_vecVelocity().x, pLocal->m_vecVelocity().y, 0.0f }), &forward);

			Vec3 vTo = pLocal->m_vecOrigin() + ((forward * -1.0f) * 100.0f);
			SDK::WalkTo(pCmd, pLocal, vTo);
		}

		break;
	}
}