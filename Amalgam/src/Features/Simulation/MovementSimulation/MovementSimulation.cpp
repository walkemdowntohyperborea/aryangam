#include "MovementSimulation.h"

#include "../../EnginePrediction/EnginePrediction.h"
#include <numeric>

// cr: kaizer @ https://github.com/kaizer1308/

namespace MoveSimConstants
{
	constexpr float HULL_PADDING = 0.0625f;
	constexpr float MAX_MOVEMENT_SPEED = 450.f;
	constexpr float MAX_RECORD_GAP = 0.35f;
	constexpr float WEIGHT_EXP_DECAY = 1.5f;
	constexpr float VELOCITY_THRESHOLD = 0.015f;
}

static CUserCmd s_tDummyCmd = {};

void CMovementSimulation::Store(MoveStorage& tMoveStorage)
{
	auto pMap = tMoveStorage.m_pPlayer->GetPredDescMap();
	if (!pMap)
		return;

	size_t iSize = tMoveStorage.m_pPlayer->GetIntermediateDataSize();
	tMoveStorage.m_pData = reinterpret_cast<byte*>(I::MemAlloc->Alloc(iSize));

	CPredictionCopy copy = { PC_NETWORKED_ONLY, tMoveStorage.m_pData, PC_DATA_PACKED, tMoveStorage.m_pPlayer, PC_DATA_NORMAL };
	copy.TransferData("MovementSimulationStore", tMoveStorage.m_pPlayer->entindex(), pMap);
}

void CMovementSimulation::Reset(MoveStorage& tMoveStorage)
{
	if (tMoveStorage.m_pData)
	{
		auto pMap = tMoveStorage.m_pPlayer->GetPredDescMap();
		if (!pMap)
			return;

		CPredictionCopy copy = { PC_NETWORKED_ONLY, tMoveStorage.m_pPlayer, PC_DATA_NORMAL, tMoveStorage.m_pData, PC_DATA_PACKED };
		copy.TransferData("MovementSimulationReset", tMoveStorage.m_pPlayer->entindex(), pMap);

		I::MemAlloc->Free(tMoveStorage.m_pData);
		tMoveStorage.m_pData = nullptr;
	}
}

void CMovementSimulation::Store()
{
	for (auto pEntity : H::Entities.GetGroup(EntityEnum::PlayerAll))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		const int iIndex = pPlayer->entindex();
		auto& vRecords = m_mRecords[iIndex];

		if (!pPlayer->IsAlive() || pPlayer->IsAGhost())
		{
			vRecords.clear();
			continue;
		}

		bool bLocal = pPlayer->entindex() == I::EngineClient->GetLocalPlayer() && !I::EngineClient->IsPlayingDemo();
		Vec3 vVelocity = bLocal ? F::EnginePrediction.m_vVelocity : pPlayer->m_vecVelocity();
		Vec3 vOrigin = bLocal ? F::EnginePrediction.m_vOrigin : pPlayer->m_vecOrigin();
		float flMaxSpeed = SDK::MaxSpeed(pPlayer);
		Vec3 vDirection = Vec3();
		if (vVelocity.Length2D() > flMaxSpeed * MoveSimConstants::VELOCITY_THRESHOLD)
			vDirection = vVelocity.Normalized2D() * flMaxSpeed;
		if (pPlayer->IsSwimming())
			vDirection.z = vVelocity.z;

		if (vDirection.IsZero())
		{
			vRecords.clear();
			continue;
		}

		MoveData* pLastRecord = !vRecords.empty() ? &vRecords.front() : nullptr;
		const float flSimTime = pPlayer->m_flSimulationTime();
		const int iMode = pPlayer->IsSwimming() ? 2 : pPlayer->IsOnGround() ? 0 : 1;
		if (pLastRecord && pLastRecord->m_flSimTime == flSimTime)
		{
			pLastRecord->m_vDirection = vDirection;
			pLastRecord->m_iMode = iMode;
			pLastRecord->m_vVelocity = vVelocity;
			pLastRecord->m_vOrigin = vOrigin;
			continue;
		}
		vRecords.emplace_front(
			vDirection,
			flSimTime,
			iMode,
			vVelocity,
			vOrigin
		);
		if (vRecords.size() > 66)
			vRecords.pop_back();

		if (pLastRecord)
		{
			CGameTrace trace = {};
			CTraceFilterWorldAndPropsOnly filter = {};
			SDK::TraceHull(pLastRecord->m_vOrigin, pLastRecord->m_vOrigin + pLastRecord->m_vVelocity * TICK_INTERVAL, pPlayer->m_vecMins() + MoveSimConstants::HULL_PADDING, pPlayer->m_vecMaxs() - MoveSimConstants::HULL_PADDING, pPlayer->SolidMask(), &filter, &trace);
			if (trace.DidHit() && trace.plane.normal.z < 0.5f)
				vRecords.clear();
		}
	}

	for (auto pEntity : H::Entities.GetGroup(EntityEnum::PlayerAll))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		const int iIndex = pPlayer->entindex();
		auto& vSimTimes = m_mSimTimes[iIndex];

		if (pEntity->entindex() == I::EngineClient->GetLocalPlayer() || !pPlayer->IsAlive() || pPlayer->IsAGhost())
		{
			vSimTimes.clear();
			continue;
		}

		float flDeltaTime = H::Entities.GetDeltaTime(iIndex);
		if (!flDeltaTime)
			continue;

		vSimTimes.push_front(flDeltaTime);
		if (vSimTimes.size() > Vars::Aimbot::Projectile::DeltaCount.Value)
			vSimTimes.pop_back();
	}
}

bool CMovementSimulation::Initialize(CBaseEntity* pEntity, MoveStorage& tMoveStorage, bool bHitchance, bool bStrafe)
{
	if (!pEntity || !pEntity->IsPlayer() || !pEntity->As<CTFPlayer>()->IsAlive())
	{
		tMoveStorage.m_bInitFailed = tMoveStorage.m_bFailed = true;
		return false;
	}

	tMoveStorage.m_flAverageYaw = 0.f; // reset any stale strafe estimation

	auto pPlayer = pEntity->As<CTFPlayer>();
	tMoveStorage.m_pPlayer = pPlayer;

	// store vars
	m_bOldInPrediction = I::Prediction->m_bInPrediction;
	m_bOldFirstTimePredicted = I::Prediction->m_bFirstTimePredicted;
	m_flOldFrametime = I::GlobalVars->frametime;

	// store restore data
	Store(tMoveStorage);

	// the hacks that make it work
	I::MoveHelper->SetHost(pPlayer);
	pPlayer->m_pCurrentCommand() = &s_tDummyCmd;

	if (auto pAvgVelocity = H::Entities.GetAvgVelocity(pPlayer->entindex()))
		pPlayer->m_vecVelocity() = *pAvgVelocity; // only use average velocity here

	if (pPlayer->m_bDucked() = pPlayer->IsDucking())
	{
		pPlayer->m_fFlags() &= ~FL_DUCKING; // breaks origin's z if FL_DUCKING is not removed
		pPlayer->m_flDucktime() = 0.f;
		pPlayer->m_flDuckJumpTime() = 0.f;
		pPlayer->m_bDucking() = false;
		pPlayer->m_bInDuckJump() = false;
	}

	if (pPlayer->entindex() != I::EngineClient->GetLocalPlayer())
	{
		pPlayer->m_vecBaseVelocity() = Vec3(); // residual basevelocity causes issues
		if (pPlayer->IsOnGround())
			pPlayer->m_vecVelocity().z = std::min(pPlayer->m_vecVelocity().z, 0.f); // step fix
		else
			pPlayer->m_hGroundEntity() = nullptr; // fix for velocity.z being set to 0 even if in air
	}
	else if (Vars::Misc::Movement::Bunnyhop.Value && G::OriginalCmd.buttons & IN_JUMP)
		tMoveStorage.m_bBunnyHop = true;

	// setup move data
	if (!SetupMoveData(tMoveStorage))
	{
		tMoveStorage.m_bFailed = true;
		return false;
	}

	// reserve path storage to avoid per tick reallocations when recording paths
	tMoveStorage.m_vPath.clear();
	if (tMoveStorage.m_vPath.capacity() < 128)
		tMoveStorage.m_vPath.reserve(128);

	const int iStrafeSamples = tMoveStorage.m_bDirectMove
		? Vars::Aimbot::Projectile::GroundSamples.Value
		: Vars::Aimbot::Projectile::AirSamples.Value;

	bool bCalculated = bStrafe ? StrafePrediction(tMoveStorage, iStrafeSamples) : false;

	if (bHitchance && bCalculated && !pPlayer->m_vecVelocity().IsZero() && Vars::Aimbot::Projectile::HitChance.Value)
	{
		const auto& vRecords = m_mRecords[pPlayer->entindex()];
		const auto iSamples = vRecords.size();
		if (vRecords.empty())
		{
			tMoveStorage.m_bFailed = true;
			return false;
		}

		float flLegacyChance = 1.f;
		double dbAverageYaw = 0.0;
		double dbTotalTime = 0.0;

		float flYaw1 = Math::VectorAngles(vRecords[0].m_vDirection).y;
		for (size_t i = 0; i < iSamples; i++)
		{
			if (vRecords.size() <= i + 2)
				break;

			const auto& pRecord1 = vRecords[i], & pRecord2 = vRecords[i + 1];
			if (pRecord1.m_iMode != pRecord2.m_iMode)
			{
				flYaw1 = Math::VectorAngles(pRecord2.m_vDirection).y;
				continue;
			}

			const float flYaw2 = Math::VectorAngles(pRecord2.m_vDirection).y;
			const float flTime1 = pRecord1.m_flSimTime, flTime2 = pRecord2.m_flSimTime;
			const float flDelta = flTime1 - flTime2;
			if (flDelta <= 0.f)
			{
				flYaw1 = flYaw2;
				continue;
			}
			const int iTicks = std::max(TIME_TO_TICKS(flDelta), 1);

			float flYaw = Math::NormalizeAngle(flYaw1 - flYaw2);
			dbAverageYaw += flYaw;
			dbTotalTime += std::max(flDelta, 0.f);

			flYaw /= iTicks;

			if (tMoveStorage.m_MoveData.m_flMaxSpeed)
				flYaw *= std::clamp(pRecord1.m_vVelocity.Length2D() / tMoveStorage.m_MoveData.m_flMaxSpeed, 0.f, 1.f);

			if ((i + 1) % iStrafeSamples == 0 || i == iSamples - 1)
			{
				float flAverageYaw = dbAverageYaw / std::max(TIME_TO_TICKS(dbTotalTime), 1);
				if (fabsf(tMoveStorage.m_flAverageYaw - flAverageYaw) > 0.5f)
					flLegacyChance -= 1.f / ((iSamples - 1) / float(iStrafeSamples) + 1);

				dbAverageYaw = 0.0;
				dbTotalTime = 0.0;
			}

			flYaw1 = flYaw2;
		}

		double dbTotalWeight = 0.0;
		double dbDevWeight = 0.0;
		const float flYawTolerance = 0.6f;

		flYaw1 = Math::VectorAngles(vRecords[0].m_vDirection).y;
		for (size_t i = 0; i + 1 < iSamples; i++)
		{
			const auto& pRecord1 = vRecords[i];
			const auto& pRecord2 = vRecords[i + 1];
			if (pRecord1.m_iMode != pRecord2.m_iMode)
			{
				flYaw1 = Math::VectorAngles(pRecord2.m_vDirection).y;
				continue;
			}

			const float flYaw2 = Math::VectorAngles(pRecord2.m_vDirection).y;
			const float flDeltaTime = std::max(pRecord1.m_flSimTime - pRecord2.m_flSimTime, 0.f);
			if (flDeltaTime <= 0.f)
			{
				flYaw1 = flYaw2;
				continue;
			}

			const int iTicks = std::max(TIME_TO_TICKS(flDeltaTime), 1);

			float flYawRate = Math::NormalizeAngle(flYaw1 - flYaw2) / iTicks;
			if (tMoveStorage.m_MoveData.m_flMaxSpeed)
				flYawRate *= std::clamp(pRecord1.m_vVelocity.Length2D() / tMoveStorage.m_MoveData.m_flMaxSpeed, 0.f, 1.f);

			const double dbWeight = pRecord1.m_vVelocity.Length2D() * flDeltaTime;
			dbTotalWeight += dbWeight;
			if (fabsf(flYawRate - tMoveStorage.m_flAverageYaw) > flYawTolerance)
				dbDevWeight += dbWeight;

			flYaw1 = flYaw2;
		}

		float flNewChance = 1.f;
		if (dbTotalWeight > 0.0)
			flNewChance = static_cast<float>(1.0 - std::clamp(dbDevWeight / dbTotalWeight, 0.0, 1.0));

		float flCurrentChance = flLegacyChance;
		if (dbTotalWeight > 0.0)
		{
			constexpr float flBlend = 0.35f;
			flCurrentChance = std::clamp(flLegacyChance + (flNewChance - flLegacyChance) * flBlend, 0.f, 1.f);
		}
		if (flCurrentChance < Vars::Aimbot::Projectile::HitChance.Value / 100)
		{
			SDK::Output("MovementSimulation", std::format("Hitchance (current {}% < {}%, legacy {}%, new {}%)", flCurrentChance * 100, Vars::Aimbot::Projectile::HitChance.Value, flLegacyChance * 100, flNewChance * 100).c_str(), { 80, 200, 120 }, Vars::Debug::Logging.Value);

			tMoveStorage.m_bFailed = true;
			return false;
		}
	}

	for (int i = 0; i < H::Entities.GetChoke(pPlayer->entindex()); i++)
		RunTick(tMoveStorage);

	return true;
}

bool CMovementSimulation::SetupMoveData(MoveStorage& tMoveStorage)
{
	if (!tMoveStorage.m_pPlayer)
		return false;

	tMoveStorage.m_MoveData.m_bFirstRunOfFunctions = false;
	tMoveStorage.m_MoveData.m_bGameCodeMovedPlayer = false;
	tMoveStorage.m_MoveData.m_nPlayerHandle = reinterpret_cast<IHandleEntity*>(tMoveStorage.m_pPlayer)->GetRefEHandle();

	tMoveStorage.m_MoveData.m_vecAbsOrigin = tMoveStorage.m_pPlayer->m_vecOrigin();
	tMoveStorage.m_MoveData.m_vecVelocity = tMoveStorage.m_pPlayer->m_vecVelocity();
	tMoveStorage.m_MoveData.m_flMaxSpeed = SDK::MaxSpeed(tMoveStorage.m_pPlayer);
	tMoveStorage.m_MoveData.m_flClientMaxSpeed = tMoveStorage.m_MoveData.m_flMaxSpeed;

	if (!tMoveStorage.m_MoveData.m_vecVelocity.To2D().IsZero())
	{
		int iIndex = tMoveStorage.m_pPlayer->entindex();
		if (iIndex == I::EngineClient->GetLocalPlayer() && G::CurrentUserCmd)
			tMoveStorage.m_MoveData.m_vecViewAngles = G::CurrentUserCmd->viewangles;
		else
		{
			if (!tMoveStorage.m_pPlayer->InCond(TF_COND_SHIELD_CHARGE))
				tMoveStorage.m_MoveData.m_vecViewAngles = { 0.f, Math::VectorAngles(tMoveStorage.m_MoveData.m_vecVelocity).y, 0.f };
			else
				tMoveStorage.m_MoveData.m_vecViewAngles = H::Entities.GetEyeAngles(iIndex);
		}

		const auto& vRecords = m_mRecords[tMoveStorage.m_pPlayer->entindex()];
		if (!vRecords.empty())
		{
			auto& tRecord = vRecords.front();
			if (!tRecord.m_vDirection.IsZero())
			{
				Vec3 vForward = {}, vRight = {};
				Math::AngleVectors(tMoveStorage.m_MoveData.m_vecViewAngles, &vForward, &vRight);
				vForward = vForward.To2D();
				vRight = vRight.To2D();
				const Vec3 vWish = tRecord.m_vDirection;
				tMoveStorage.m_MoveData.m_flForwardMove = vWish.Dot(vForward);
				tMoveStorage.m_MoveData.m_flSideMove = vWish.Dot(vRight);
				tMoveStorage.m_MoveData.m_flUpMove = vWish.z;
			}
		}
	}

	tMoveStorage.m_MoveData.m_vecAngles = tMoveStorage.m_MoveData.m_vecOldAngles = tMoveStorage.m_MoveData.m_vecViewAngles;
	if (auto pConstraintEntity = tMoveStorage.m_pPlayer->m_hConstraintEntity().Get())
		tMoveStorage.m_MoveData.m_vecConstraintCenter = pConstraintEntity->GetAbsOrigin();
	else
		tMoveStorage.m_MoveData.m_vecConstraintCenter = tMoveStorage.m_pPlayer->m_vecConstraintCenter();
	tMoveStorage.m_MoveData.m_flConstraintRadius = tMoveStorage.m_pPlayer->m_flConstraintRadius();
	tMoveStorage.m_MoveData.m_flConstraintWidth = tMoveStorage.m_pPlayer->m_flConstraintWidth();
	tMoveStorage.m_MoveData.m_flConstraintSpeedFactor = tMoveStorage.m_pPlayer->m_flConstraintSpeedFactor();

	tMoveStorage.m_flPredictedDelta = GetPredictedDelta(tMoveStorage.m_pPlayer);
	tMoveStorage.m_flSimTime = tMoveStorage.m_pPlayer->m_flSimulationTime();
	tMoveStorage.m_flPredictedSimTime = tMoveStorage.m_flSimTime + tMoveStorage.m_flPredictedDelta;
	tMoveStorage.m_vPredictedOrigin = tMoveStorage.m_MoveData.m_vecAbsOrigin;
	tMoveStorage.m_bDirectMove = tMoveStorage.m_pPlayer->IsOnGround() || tMoveStorage.m_pPlayer->IsSwimming();

	return true;
}

static inline float GetGravity(CBaseEntity* pEntity = nullptr)
{
	static auto sv_gravity = U::ConVars.FindVar("sv_gravity");
	float flGravity = sv_gravity->GetFloat();

	if (pEntity && pEntity->IsPlayer() && pEntity->As<CTFPlayer>()->InCond(TF_COND_PARACHUTE_DEPLOYED))
		flGravity *= 0.5f;

	return flGravity;
}

static inline float GetFrictionScale(float flVelocityXY, float flTurn, float flVelocityZ, float flMin = 50.f, float flMax = 150.f)
{
	static auto sv_airaccelerate = U::ConVars.FindVar("sv_airaccelerate");
	float flScale = std::max(sv_airaccelerate->GetFloat(), 1.f);
	flMin *= flScale, flMax *= flScale;

	// entity friction will be 0.25f if velocity is between 0.f and 250.f
	return Math::RemapVal(fabsf(flVelocityXY * flTurn), flMin, flMax, 1.f, 0.25f);
}

static inline bool GetYawDifference(CBaseEntity* pEntity, MoveData& tRecord1, MoveData& tRecord2, StrafeDataState& state, bool bStart, float* pYaw, float flYaw1, float flYaw2, float flStraightFuzzyValue, int iMaxChanges = 0, int iMaxChangeTime = 0, float flMaxSpeed = 0.f)
{
	const float flTime1 = tRecord1.m_flSimTime, flTime2 = tRecord2.m_flSimTime;
	const int iTicks = std::max(TIME_TO_TICKS(flTime1 - flTime2), 1);

	*pYaw = Math::NormalizeAngle(flYaw1 - flYaw2);
	if (flMaxSpeed && tRecord1.m_iMode != 1)
		*pYaw *= std::clamp(tRecord1.m_vVelocity.Length2D() / flMaxSpeed, 0.f, 1.f);
	if (Vars::Aimbot::Projectile::MovesimFrictionFlags.Value & Vars::Aimbot::Projectile::MovesimFrictionFlagsEnum::CalculateIncrease && tRecord1.m_iMode == 1)
		*pYaw /= GetFrictionScale(tRecord1.m_vVelocity.Length2D(), *pYaw, tRecord1.m_vVelocity.z + GetGravity(pEntity) * TICK_INTERVAL, 0.f, 56.f);
	if (fabsf(*pYaw) > 45.f)
		return false;

	const bool iLastZero = state.bStaticZero;
	const bool iCurrZero = fabsf(*pYaw) < 0.001f;
	const int iLastSign = state.iStaticSign;
	const int iCurrSign = iCurrZero ? 0 : sign(*pYaw);
	state.bStaticZero = iCurrZero;
	state.iStaticSign = iCurrSign ? iCurrSign : state.iStaticSign;

	const bool bChanged = (!iCurrZero && iLastSign && iCurrSign && iCurrSign != iLastSign) || (iCurrZero != iLastZero);
	const bool bStraight = fabsf(*pYaw) * tRecord1.m_vVelocity.Length2D() * iTicks < flStraightFuzzyValue; // dumb way to get straight bool

	if (bStart)
	{
		state.iChanges = 0; state.iStart = TIME_TO_TICKS(flTime1);
		if (bStraight && ++state.iChanges > iMaxChanges)
			return false;
		return true;
	}
	else
	{
		if ((bChanged || bStraight) && ++state.iChanges > iMaxChanges)
			return false;
		return state.iChanges && state.iStart - TIME_TO_TICKS(flTime2) > iMaxChangeTime ? false : true;
	}
}

void CMovementSimulation::GetAverageYaw(MoveStorage& tMoveStorage, int iSamples)
{
	auto pPlayer = tMoveStorage.m_pPlayer;
	auto& vRecords = m_mRecords[pPlayer->entindex()];
	if (vRecords.empty())
		return;

	tMoveStorage.m_flAverageYaw = 0.f;

	const float flMinSpeed2D = 6.f;
	const float flMaxRecordGap = MoveSimConstants::MAX_RECORD_GAP;

	bool bGround = tMoveStorage.m_bDirectMove; int iMinimumStrafes = 4;
	float flMaxSpeed = SDK::MaxSpeed(tMoveStorage.m_pPlayer, false, true);

	float flLowMinimumDistance = bGround ? Vars::Aimbot::Projectile::GroundLowMinimumDistance.Value : Vars::Aimbot::Projectile::AirLowMinimumDistance.Value;
	float flLowMinimumSamples = bGround ? Vars::Aimbot::Projectile::GroundLowMinimumSamples.Value : Vars::Aimbot::Projectile::AirLowMinimumSamples.Value;
	float flHighMinimumDistance = bGround ? Vars::Aimbot::Projectile::GroundHighMinimumDistance.Value : Vars::Aimbot::Projectile::AirHighMinimumDistance.Value;
	float flHighMinimumSamples = bGround ? Vars::Aimbot::Projectile::GroundHighMinimumSamples.Value : Vars::Aimbot::Projectile::AirHighMinimumSamples.Value;

	double dbWeightedYaw = 0.0;
	double dbTotalWeight = 0.0;
	double dbAccumulatedTime = 0.0;
	StrafeDataState state = {};

	iSamples = std::min(iSamples, int(vRecords.size()));
	size_t i = 1;
	int iValidStrafes = 0;

	float flYaw1 = Math::VectorAngles(vRecords[0].m_vDirection).y;
	for (; i < iSamples; i++)
	{
		auto& tRecord1 = vRecords[i - 1];
		auto& tRecord2 = vRecords[i];
		if (tRecord1.m_iMode != tRecord2.m_iMode)
		{
			state = {};
			flYaw1 = Math::VectorAngles(tRecord2.m_vDirection).y;
			continue;
		}

		const float flDeltaTime = std::max(tRecord1.m_flSimTime - tRecord2.m_flSimTime, 0.f);
		if (flDeltaTime <= 0.f)
			continue;
		if (flDeltaTime > flMaxRecordGap)
		{
			state = {};
			flYaw1 = Math::VectorAngles(tRecord2.m_vDirection).y;
			continue;
		}

		float flYaw2 = Math::VectorAngles(tRecord2.m_vDirection).y;

		bGround = tRecord1.m_iMode != 1;
		float flStraightFuzzyValue = bGround ? Vars::Aimbot::Projectile::GroundStraightFuzzyValue.Value : Vars::Aimbot::Projectile::AirStraightFuzzyValue.Value;
		int iMaxChanges = bGround ? Vars::Aimbot::Projectile::GroundMaxChanges.Value : Vars::Aimbot::Projectile::AirMaxChanges.Value;
		int iMaxChangeTime = bGround ? Vars::Aimbot::Projectile::GroundMaxChangeTime.Value : Vars::Aimbot::Projectile::AirMaxChangeTime.Value;
		if (!bGround && tRecord2.m_iMode == 0 && flDeltaTime <= TICK_INTERVAL * 2)
			bGround = false;
		iMinimumStrafes = 4 + iMaxChanges;

		const float flSpeed = tRecord1.m_vVelocity.Length2D();
		if (flSpeed < flMinSpeed2D)
		{
			flYaw1 = flYaw2;
			continue;
		}

		float flYaw = 0.f;
		bool bResult = GetYawDifference(pPlayer, tRecord1, tRecord2, state, state.iStart == 0, &flYaw, flYaw1, flYaw2, flStraightFuzzyValue, iMaxChanges, iMaxChangeTime, flMaxSpeed);

		flYaw1 = flYaw2; // Cache for next iteration

		if (!bResult)
			break;

		const float flTicks = std::max(flDeltaTime / TICK_INTERVAL, 1.f);

		const float flYawRate = flYaw / flTicks;
		++iValidStrafes;
		const double dbWeight = flSpeed * flDeltaTime * std::exp(-dbAccumulatedTime * MoveSimConstants::WEIGHT_EXP_DECAY); // weight yaw by speed and time
		dbWeightedYaw += flYawRate * dbWeight;
		dbTotalWeight += dbWeight;
		dbAccumulatedTime += flDeltaTime;
	}
#ifdef VISUALIZE_RECORDS
	size_t i2 = i; for (; i2 < iSamples; i2++)
	{
		auto& tRecord1 = vRecords[i2 - 1];
		auto& tRecord2 = vRecords[i2];

		float flStraightFuzzyValue = bGround ? Vars::Aimbot::Projectile::GroundStraightFuzzyValue.Value : Vars::Aimbot::Projectile::AirStraightFuzzyValue.Value;
		VisualizeRecords(tRecord1, tRecord2, { 0, 0, 0, 100 }, flStraightFuzzyValue);
	}
#endif
	if (iValidStrafes < iMinimumStrafes) // valid strafes not high enough
		return;

	int iMinimum = flLowMinimumSamples;
	if (pPlayer->entindex() != I::EngineClient->GetLocalPlayer())
	{
		float flDistance = 0.f;
		if (auto pLocal = H::Entities.GetLocal())
			flDistance = pLocal->m_vecOrigin().DistTo(tMoveStorage.m_pPlayer->m_vecOrigin());
		iMinimum = flDistance < flLowMinimumDistance ? flLowMinimumSamples : Math::RemapVal(flDistance, flLowMinimumDistance, flHighMinimumDistance, flLowMinimumSamples + 1, flHighMinimumSamples);
	}

	// demand both weight and meaningful time covered; clamp minimum by ticks of collected weight
	const int iCollectedTicks = std::max(TIME_TO_TICKS(float(dbTotalWeight) / std::max(flMaxSpeed, 1.f)), 1);
	if (iCollectedTicks < iMinimum || dbTotalWeight <= 0.0)
		return;

	float flAverageYaw = static_cast<float>(dbWeightedYaw / dbTotalWeight);

	if (fabsf(flAverageYaw) < 0.2f)
		return;

	tMoveStorage.m_flAverageYaw = flAverageYaw;
	SDK::Output("MovementSimulation", std::format("flAverageYaw calculated to {} with weight {:.2f} (min {} {})", flAverageYaw, dbTotalWeight, iMinimum, pPlayer->entindex() == I::EngineClient->GetLocalPlayer() ? "local" : "remote").c_str(), { 100, 255, 150 }, Vars::Debug::Logging.Value);
}

bool CMovementSimulation::StrafePrediction(MoveStorage& tMoveStorage, int iSamples)
{
	if (tMoveStorage.m_bDirectMove
		? !(Vars::Aimbot::Projectile::StrafePrediction.Value & Vars::Aimbot::Projectile::StrafePredictionEnum::Ground)
		: !(Vars::Aimbot::Projectile::StrafePrediction.Value & Vars::Aimbot::Projectile::StrafePredictionEnum::Air))
		return false;

	GetAverageYaw(tMoveStorage, iSamples);
	return true;
}

bool CMovementSimulation::SetDuck(MoveStorage& tMoveStorage, bool bDuck) // this only touches origin, bounds
{
	if (bDuck == tMoveStorage.m_pPlayer->m_bDucked())
		return true;

	auto pGameRules = I::TFGameRules();
	auto pViewVectors = pGameRules ? pGameRules->GetViewVectors() : nullptr;
	float flScale = tMoveStorage.m_pPlayer->m_flModelScale();

	if (!tMoveStorage.m_pPlayer->IsOnGround())
	{
		Vec3 vHullMins = (pViewVectors ? pViewVectors->m_vHullMin : Vec3(-24, -24, 0)) * flScale;
		Vec3 vHullMaxs = (pViewVectors ? pViewVectors->m_vHullMax : Vec3(24, 24, 82)) * flScale;
		Vec3 vDuckHullMins = (pViewVectors ? pViewVectors->m_vDuckHullMin : Vec3(-24, -24, 0)) * flScale;
		Vec3 vDuckHullMaxs = (pViewVectors ? pViewVectors->m_vDuckHullMax : Vec3(24, 24, 62)) * flScale;

		if (bDuck)
			tMoveStorage.m_MoveData.m_vecAbsOrigin += (vHullMaxs - vHullMins) - (vDuckHullMaxs - vDuckHullMins);
		else
		{
			Vec3 vOrigin = tMoveStorage.m_MoveData.m_vecAbsOrigin - ((vHullMaxs - vHullMins) - (vDuckHullMaxs - vDuckHullMins));

			CGameTrace trace = {};
			CTraceFilterWorldAndPropsOnly filter = {};
			SDK::TraceHull(vOrigin, vOrigin, vHullMins, vHullMaxs, tMoveStorage.m_pPlayer->SolidMask(), &filter, &trace);
			if (trace.DidHit())
				return false;

			tMoveStorage.m_MoveData.m_vecAbsOrigin = vOrigin;
		}
	}
	tMoveStorage.m_pPlayer->m_bDucked() = bDuck;

	return true;
}


void CMovementSimulation::RunTick(MoveStorage& tMoveStorage, bool bPath, std::function<void(CMoveData&)>* pCallback)
{
	if (tMoveStorage.m_bFailed || !tMoveStorage.m_pPlayer || !tMoveStorage.m_pPlayer->IsPlayer())
		return;

	if (bPath)
		tMoveStorage.m_vPath.emplace_back(tMoveStorage.m_MoveData.m_vecAbsOrigin, tMoveStorage.m_MoveData.m_vecVelocity, tMoveStorage.m_MoveData.m_flMaxSpeed);

	// make sure frametime and prediction vars are right
	I::Prediction->m_bInPrediction = true;
	I::Prediction->m_bFirstTimePredicted = false;
	I::GlobalVars->frametime = I::Prediction->m_bEnginePaused ? 0.f : TICK_INTERVAL;

	CScopedBounds scopedBounds(tMoveStorage.m_pPlayer);

	float flAppliedYaw = 0.f;
	if (tMoveStorage.m_flAverageYaw)
	{
		float flMult = 1.f;
		if (!tMoveStorage.m_bDirectMove && !tMoveStorage.m_pPlayer->InCond(TF_COND_SHIELD_CHARGE)
			&& (Vars::Aimbot::Projectile::MovesimFrictionFlags.Value & Vars::Aimbot::Projectile::MovesimFrictionFlagsEnum::RunReduce))
			flMult = GetFrictionScale(tMoveStorage.m_MoveData.m_vecVelocity.Length2D(), tMoveStorage.m_flAverageYaw, tMoveStorage.m_MoveData.m_vecVelocity.z + GetGravity(tMoveStorage.m_pPlayer) * TICK_INTERVAL);

		flAppliedYaw = tMoveStorage.m_flAverageYaw * flMult;
		tMoveStorage.m_MoveData.m_vecViewAngles.y += flAppliedYaw;

		if (!tMoveStorage.m_bDirectMove && !tMoveStorage.m_pPlayer->InCond(TF_COND_SHIELD_CHARGE))
		{
			tMoveStorage.m_MoveData.m_flForwardMove = 0.f;
			tMoveStorage.m_MoveData.m_flSideMove = (tMoveStorage.m_flAverageYaw > 0 ? -1.f : 1.f) * tMoveStorage.m_MoveData.m_flMaxSpeed;
		}
	}
	else if (!tMoveStorage.m_bDirectMove)
		tMoveStorage.m_MoveData.m_flForwardMove = tMoveStorage.m_MoveData.m_flSideMove = 0.f;

	float flOldSpeed = tMoveStorage.m_MoveData.m_flClientMaxSpeed;
	if (tMoveStorage.m_pPlayer->m_bDucked() && tMoveStorage.m_pPlayer->IsOnGround() && !tMoveStorage.m_pPlayer->IsSwimming())
		tMoveStorage.m_MoveData.m_flClientMaxSpeed /= 3;

	if (tMoveStorage.m_bBunnyHop && tMoveStorage.m_pPlayer->IsOnGround() && !tMoveStorage.m_pPlayer->m_bDucked())
	{
		tMoveStorage.m_MoveData.m_nOldButtons = 0;
		tMoveStorage.m_MoveData.m_nButtons |= IN_JUMP;
	}

	I::GameMovement->ProcessMovement(tMoveStorage.m_pPlayer, &tMoveStorage.m_MoveData);
	if (pCallback)
		(*pCallback)(tMoveStorage.m_MoveData);

	tMoveStorage.m_MoveData.m_flClientMaxSpeed = flOldSpeed;

	tMoveStorage.m_flSimTime += TICK_INTERVAL;
	tMoveStorage.m_bPredictNetworked = tMoveStorage.m_flSimTime >= tMoveStorage.m_flPredictedSimTime;
	if (tMoveStorage.m_bPredictNetworked)
	{
		tMoveStorage.m_vPredictedOrigin = tMoveStorage.m_MoveData.m_vecAbsOrigin;
		tMoveStorage.m_flPredictedSimTime += tMoveStorage.m_flPredictedDelta;
	}
	bool bLastbDirectMove = tMoveStorage.m_bDirectMove;
	tMoveStorage.m_bDirectMove = tMoveStorage.m_pPlayer->IsOnGround() || tMoveStorage.m_pPlayer->IsSwimming();

	if (!tMoveStorage.m_flAverageYaw && tMoveStorage.m_bDirectMove && !bLastbDirectMove
		&& !tMoveStorage.m_MoveData.m_flForwardMove && !tMoveStorage.m_MoveData.m_flSideMove
		&& tMoveStorage.m_MoveData.m_vecVelocity.Length2D() > tMoveStorage.m_MoveData.m_flMaxSpeed * MoveSimConstants::VELOCITY_THRESHOLD)
	{
		Vec3 vDirection = tMoveStorage.m_MoveData.m_vecVelocity.Normalized2D() * MoveSimConstants::MAX_MOVEMENT_SPEED;
		s_tDummyCmd.forwardmove = vDirection.x, s_tDummyCmd.sidemove = -vDirection.y;
		SDK::FixMovement(&s_tDummyCmd, {}, tMoveStorage.m_MoveData.m_vecViewAngles);
		tMoveStorage.m_MoveData.m_flForwardMove = s_tDummyCmd.forwardmove, tMoveStorage.m_MoveData.m_flSideMove = s_tDummyCmd.sidemove;
	}
}

void CMovementSimulation::RunTick(MoveStorage& tMoveStorage, bool bPath, std::function<void(CMoveData&)> fCallback)
{
	RunTick(tMoveStorage, bPath, &fCallback);
}

void CMovementSimulation::Restore(MoveStorage& tMoveStorage)
{
	if (tMoveStorage.m_bInitFailed || !tMoveStorage.m_pPlayer)
		return;

	I::MoveHelper->SetHost(nullptr);
	tMoveStorage.m_pPlayer->m_pCurrentCommand() = nullptr;

	Reset(tMoveStorage);

	I::Prediction->m_bInPrediction = m_bOldInPrediction;
	I::Prediction->m_bFirstTimePredicted = m_bOldFirstTimePredicted;
	I::GlobalVars->frametime = m_flOldFrametime;
}

float CMovementSimulation::GetPredictedDelta(CBaseEntity* pEntity)
{
	auto& vSimTimes = m_mSimTimes[pEntity->entindex()];
	if (!vSimTimes.empty())
	{
		switch (Vars::Aimbot::Projectile::DeltaMode.Value)
		{
		case 0: return std::reduce(vSimTimes.begin(), vSimTimes.end()) / vSimTimes.size();
		case 1: return *std::max_element(vSimTimes.begin(), vSimTimes.end());
		}
	}
	return TICK_INTERVAL;
}