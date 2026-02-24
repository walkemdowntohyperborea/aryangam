#pragma once
#include "../../SDK/SDK.h"
#include <unordered_set>

struct AngleHistory_t
{
	Vec3 m_vAngle;
	bool m_bAttacking;
};

struct PlayerInfo
{
	uint32_t m_uAccountID = 0;
	const char* m_sName = "";
	bool m_bListChecked = false;

	int m_iDetections = 0;

	struct PacketChoking_t
	{
		std::deque<int> m_vChokes = {}; // store last 3 choke counts
		bool m_bInfract = false; // infract the user for choking?
	} m_PacketChoking;

	struct AimFlicking_t
	{
		std::deque<AngleHistory_t> m_vAngles = {}; // store last 3 angles & if damage was dealt
	} m_AimFlicking;
					
	struct DuckSpeed_t
	{
		int m_iStartTick = 0;
	} m_DuckSpeed;

	struct CritTracker_t
	{
		struct WeaponHistory_t
		{
			std::deque<bool> m_vHistory = {};
			int m_iCrits = 0;
		};

		std::unordered_map<int, WeaponHistory_t> m_mWeaponHistory = {};
		bool m_bInfract = false;
	} m_CritTracker;
};

class CCheaterDetection
{
private:
	bool ShouldScan();

	bool InvalidPitch(CTFPlayer* pEntity);
	bool IsChoking(CTFPlayer* pEntity);
	bool IsFlicking(CTFPlayer* pEntity);
	bool IsDuckSpeed(CTFPlayer* pEntity);
	bool LowPing(CTFPlayer* pEntity, CTFPlayerResource* pResource);
	bool IsInList(CTFPlayer* pEntity);
	bool IsCritManipulating(CTFPlayer* pEntity);
	void TrackCritEvent(CTFPlayer* pEntity, CTFWeaponBase* pWeapon, bool bCrit);

	void Infract(CTFPlayer* pEntity, const char* sReason);
	std::unordered_map<CTFPlayer*, PlayerInfo> mData;
	std::unordered_set<uint32_t> mCheaters;
public:
	void Run();

	void ReportChoke(CTFPlayer* pEntity, int iChoke);
	void ReportDamage(IGameEvent* pEvent);
	void Reset();

	void InitList();
};

ADD_FEATURE(CCheaterDetection, CheaterDetection);