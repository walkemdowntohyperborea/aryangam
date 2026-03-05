#pragma once
#include "../../../SDK/SDK.h"
#include <functional>

Enum(Move, Ground, Air, Swim)

struct MoveStorage
{
	CTFPlayer* m_pPlayer = nullptr;
	CMoveData m_MoveData = {};
	byte* m_pData = nullptr;

	float m_flAverageYaw = 0.f;
	bool m_bBunnyHop = false;

	float m_flSimTime = 0.f;
	float m_flPredictedDelta = 0.f;
	float m_flPredictedSimTime = 0.f;
	bool m_bDirectMove = true;

	bool m_bPredictNetworked = true;
	Vec3 m_vPredictedOrigin = {};

	std::vector<std::tuple<Vec3, Vec3, float>> m_vPath = {};

	bool m_bFailed = false;
	bool m_bInitFailed = false;
};

struct MoveData
{
	Vec3 m_vDirection = {};
	float m_flSimTime = 0.f;
	int m_iMode = 0;
	Vec3 m_vVelocity = {};
	Vec3 m_vOrigin = {};
};

struct StrafeDataState
{
	int iChanges = 0;
	int iStart = 0;
	int iStaticSign = 0;
	bool bStaticZero = false;
};

class CScopedBounds
{
public:
	CScopedBounds(CTFPlayer* pPlayer) : m_pPlayer(pPlayer)
	{
		if (!m_pPlayer || m_pPlayer->entindex() == I::EngineClient->GetLocalPlayer())
			return;

		if (auto pGameRules = I::TFGameRules())
		{
			if (auto pViewVectors = pGameRules->GetViewVectors())
			{
				m_pViewVectors = pViewVectors;
				m_vHullMin = pViewVectors->m_vHullMin;
				m_vHullMax = pViewVectors->m_vHullMax;
				m_vDuckHullMin = pViewVectors->m_vDuckHullMin;
				m_vDuckHullMax = pViewVectors->m_vDuckHullMax;

				pViewVectors->m_vHullMin = Vec3(-24, -24, 0) + PLAYER_ORIGIN_COMPRESSION;
				pViewVectors->m_vHullMax = Vec3(24, 24, 82) - PLAYER_ORIGIN_COMPRESSION;
				pViewVectors->m_vDuckHullMin = Vec3(-24, -24, 0) + PLAYER_ORIGIN_COMPRESSION;
				pViewVectors->m_vDuckHullMax = Vec3(24, 24, 62) - PLAYER_ORIGIN_COMPRESSION;
			}
		}
	}

	~CScopedBounds()
	{
		if (m_pViewVectors)
		{
			m_pViewVectors->m_vHullMin = m_vHullMin;
			m_pViewVectors->m_vHullMax = m_vHullMax;
			m_pViewVectors->m_vDuckHullMin = m_vDuckHullMin;
			m_pViewVectors->m_vDuckHullMax = m_vDuckHullMax;
		}
	}

private:
	CTFPlayer* m_pPlayer = nullptr;
	CViewVectors* m_pViewVectors = nullptr;
	Vec3 m_vHullMin, m_vHullMax, m_vDuckHullMin, m_vDuckHullMax;
};

class CMovementSimulation
{
private:
	void Store(MoveStorage& tMoveStorage);
	void Reset(MoveStorage& tMoveStorage);

	bool SetupMoveData(MoveStorage& tMoveStorage);
	void GetAverageYaw(MoveStorage& tMoveStorage, int iSamples);
	bool StrafePrediction(MoveStorage& tMoveStorage, int iSamples);

	//void SetBounds(CTFPlayer* pPlayer);
	//void RestoreBounds(CTFPlayer* pPlayer);

	bool m_bOldInPrediction = false;
	bool m_bOldFirstTimePredicted = false;
	float m_flOldFrametime = 0.f;

	std::unordered_map<int, std::deque<MoveData>> m_mRecords = {};
	std::unordered_map<int, std::deque<float>> m_mSimTimes = {};

public:
	void Store();

	bool Initialize(CBaseEntity* pEntity, MoveStorage& tMoveStorage, bool bHitchance = true, bool bStrafe = true);
	bool SetDuck(MoveStorage& tMoveStorage, bool bDuck);
	void RunTick(MoveStorage& tMoveStorage, bool bPath = true, std::function<void(CMoveData&)>* pCallback = nullptr);
	void RunTick(MoveStorage& tMoveStorage, bool bPath, std::function<void(CMoveData&)> fCallback);
	void Restore(MoveStorage& tMoveStorage);

	float GetPredictedDelta(CBaseEntity* pEntity);
};

ADD_FEATURE(CMovementSimulation, MoveSim);