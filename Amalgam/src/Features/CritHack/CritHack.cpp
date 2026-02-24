#include "CritHack.h"

#include "../Ticks/Ticks.h"

#define WEAPON_RANDOM_RANGE				10000
#define TF_DAMAGE_CRIT_MULTIPLIER		3.0f
#define TF_DAMAGE_CRIT_CHANCE			0.02f
#define TF_DAMAGE_CRIT_CHANCE_RAPID		0.02f
#define TF_DAMAGE_CRIT_CHANCE_MELEE		0.15f
#define TF_DAMAGE_CRIT_DURATION_RAPID	2.0f

#define STATS_SEND_FREQUENCY 1.f

#define SEED_ATTEMPTS 4096
#define BUCKET_ATTEMPTS 1000

//#define SERVER_CRIT_DATA

int CCritHack::GetCritCommand(CTFWeaponBase* pWeapon, int iCommandNumber, bool bCrit, bool bSafe)
{
	for (int i = iCommandNumber; i < iCommandNumber + SEED_ATTEMPTS; i++)
	{
		if (IsCritCommand(i, pWeapon, bCrit, bSafe))
			return i;
	}
	return 0;
}

bool CCritHack::IsCritCommand(int iCommandNumber, CTFWeaponBase* pWeapon, bool bCrit, bool bSafe)
{
	int iSeed = CommandToSeed(iCommandNumber);
	return IsCritSeed(iSeed, pWeapon, bCrit, bSafe);
}

bool CCritHack::IsCritSeed(int iSeed, CTFWeaponBase* pWeapon, bool bCrit, bool bSafe)
{
	if (iSeed == pWeapon->m_iCurrentSeed())
		return false;

	SDK::RandomSeed(iSeed);
	int iRandom = SDK::RandomInt(0, WEAPON_RANDOM_RANGE - 1);

	if (bSafe)
	{
		int iLower, iUpper;
		if (m_bMelee)
			iLower = 1500, iUpper = 6000;
		else
			iLower = 100, iUpper = 800;
		iLower *= m_flMultCritChance, iUpper *= m_flMultCritChance;

		if (bCrit ? iLower >= 0 : iUpper < WEAPON_RANDOM_RANGE)
			return bCrit ? iRandom < iLower : !(iRandom < iUpper);
	}

	int iRange = m_flCritChance * WEAPON_RANDOM_RANGE;
	return bCrit ? iRandom < iRange : !(iRandom < iRange);
}

int CCritHack::CommandToSeed(int iCommandNumber)
{
	int iSeed = MD5_PseudoRandom(iCommandNumber) & std::numeric_limits<int>::max();
	int iMask = m_bMelee
		? m_iEntIndex << 16 | I::EngineClient->GetLocalPlayer() << 8
		: m_iEntIndex << 8 | I::EngineClient->GetLocalPlayer();
	return iSeed ^ iMask;
}



void CCritHack::UpdateWeaponInfo(CTFPlayer* pLocal, CTFWeaponBase* pWeapon)
{
	m_iEntIndex = pWeapon->entindex();
	m_bMelee = pWeapon->GetSlot() == SLOT_MELEE;
	if (m_bMelee)
		m_flCritChance = TF_DAMAGE_CRIT_CHANCE_MELEE * pLocal->GetCritMult();
	else if (pWeapon->IsRapidFire())
	{
		m_flCritChance = TF_DAMAGE_CRIT_CHANCE_RAPID * pLocal->GetCritMult();
		float flNonCritDuration = (TF_DAMAGE_CRIT_DURATION_RAPID / m_flCritChance) - TF_DAMAGE_CRIT_DURATION_RAPID;
		m_flCritChance = 1.f / flNonCritDuration;
	}
	else
		m_flCritChance = TF_DAMAGE_CRIT_CHANCE * pLocal->GetCritMult();
	m_flMultCritChance = SDK::AttribHookValue(1.f, "mult_crit_chance", pWeapon);
	m_flCritChance *= m_flMultCritChance;



	static CTFWeaponBase* pStaticWeapon = nullptr;
	const CTFWeaponBase* pOldWeapon = pStaticWeapon;
	pStaticWeapon = pWeapon;

	static float flStaticBucket = 0.f;
	const float flLastBucket = flStaticBucket;
	const float flBucket = flStaticBucket = pWeapon->m_flCritTokenBucket();

	static int iStaticCritChecks = 0.f;
	const int iLastCritChecks = iStaticCritChecks;
	const int iCritChecks = iStaticCritChecks = pWeapon->m_nCritChecks();

	static int iStaticCritSeedRequests = 0.f;
	const int iLastCritSeedRequests = iStaticCritSeedRequests;
	const int iCritSeedRequests = iStaticCritSeedRequests = pWeapon->m_nCritSeedRequests();

	if (pWeapon == pOldWeapon && flBucket == flLastBucket && iCritChecks == iLastCritChecks && iCritSeedRequests == iLastCritSeedRequests)
		return;

	static auto tf_weapon_criticals_bucket_cap = U::ConVars.FindVar("tf_weapon_criticals_bucket_cap");
	const float flBucketCap = tf_weapon_criticals_bucket_cap->GetFloat();
	bool bRapidFire = pWeapon->IsRapidFire();
	float flFireRate = pWeapon->GetFireRate();

	float flDamage = pWeapon->GetDamage();
	int nProjectilesPerShot = pWeapon->GetBulletsPerShot(false);
	if (!m_bMelee && nProjectilesPerShot > 0)
		nProjectilesPerShot = SDK::AttribHookValue(nProjectilesPerShot, "mult_bullets_per_shot", pWeapon);
	else
		nProjectilesPerShot = 1;
	float flBaseDamage = flDamage *= nProjectilesPerShot;
	if (bRapidFire)
	{
		flDamage *= TF_DAMAGE_CRIT_DURATION_RAPID / flFireRate;
		if (flDamage * TF_DAMAGE_CRIT_MULTIPLIER > flBucketCap)
			flDamage = flBucketCap / TF_DAMAGE_CRIT_MULTIPLIER;
	}

	float flMult = m_bMelee ? 0.5f : Math::RemapVal(float(iCritSeedRequests + 1) / (iCritChecks + 1), 0.1f, 1.f, 1.f, 3.f);
	float flCost = flDamage * TF_DAMAGE_CRIT_MULTIPLIER;

	int iPotentialCrits = (std::max(flBucketCap, flBucket) - flBaseDamage) / (TF_DAMAGE_CRIT_MULTIPLIER * flDamage / (m_bMelee ? 2 : 1) - flBaseDamage);
	int iAvailableCrits = 0;
	{
		int iTestShots = iCritChecks, iTestCrits = iCritSeedRequests;
		float flTestBucket = flBucket;
		for (int i = 0; i < BUCKET_ATTEMPTS; i++)
		{
			iTestShots++; iTestCrits++;

			float flTestMult = m_bMelee ? 0.5f : Math::RemapVal(float(iTestCrits) / iTestShots, 0.1f, 1.f, 1.f, 3.f);
			if (flTestBucket < flBucketCap)
				flTestBucket = std::min(flTestBucket + flBaseDamage, flBucketCap);
			flTestBucket -= flCost * flTestMult;
			if (flTestBucket < 0.f)
				break;

			iAvailableCrits++;
		}
	}

	int iNextCrit = 0;
	if (iAvailableCrits != iPotentialCrits)
	{
		int iTestShots = iCritChecks, iTestCrits = iCritSeedRequests;
		float flTestBucket = flBucket;
		float flTickBase = I::GlobalVars->curtime;
		float flLastRapidFireCritCheckTime = pWeapon->m_flLastRapidFireCritCheckTime();
		for (int i = 0; i < BUCKET_ATTEMPTS; i++)
		{
			int iCrits = 0;
			{
				int iTestShots2 = iTestShots, iTestCrits2 = iTestCrits;
				float flTestBucket2 = flTestBucket;
				for (int j = 0; j < BUCKET_ATTEMPTS; j++)
				{
					iTestShots2++; iTestCrits2++;

					float flTestMult = m_bMelee ? 0.5f : Math::RemapVal(float(iTestCrits2) / iTestShots2, 0.1f, 1.f, 1.f, 3.f);
					if (flTestBucket2 < flBucketCap)
						flTestBucket2 = std::min(flTestBucket2 + flBaseDamage, flBucketCap);
					flTestBucket2 -= flCost * flTestMult;
					if (flTestBucket2 < 0.f)
						break;

					iCrits++;
				}
			}
			if (iAvailableCrits < iCrits)
				break;

			if (!bRapidFire)
				iTestShots++;
			else
			{
				flTickBase += std::ceilf(flFireRate / TICK_INTERVAL) * TICK_INTERVAL;
				if (flTickBase >= flLastRapidFireCritCheckTime + 1.f || !i && flTestBucket == flBucketCap)
				{
					iTestShots++;
					flLastRapidFireCritCheckTime = flTickBase;
				}
			}

			if (flTestBucket < flBucketCap)
				flTestBucket = std::min(flTestBucket + flBaseDamage, flBucketCap);

			iNextCrit++;
		}
	}

	m_flDamage = flBaseDamage;
	m_flCost = flCost * flMult;
	m_iPotentialCrits = iPotentialCrits;
	m_iAvailableCrits = iAvailableCrits;
	m_iNextCrit = iNextCrit;
}

bool CCritHack::IsForcingCrits(CUserCmd* pCmd)
{
	if (pCmd)
	{
		bool bPressed = Vars::CritHack::ForceCrits.Value || m_bForce;
		if (Vars::CritHack::AlwaysMeleeCrit.Value && m_bMelee
			&& (Vars::Aimbot::General::AutoShoot.Value ? pCmd->buttons & IN_ATTACK && !(G::OriginalCmd.buttons & IN_ATTACK) : Vars::Aimbot::General::AimType.Value)
			&& G::AimTarget.m_iEntIndex)
		{
			auto pEntity = I::ClientEntityList->GetClientEntity(G::AimTarget.m_iEntIndex)->As<CBaseEntity>();
			if (pEntity && pEntity->IsPlayer())
				bPressed = true;
		}
		return bPressed;
	}
	else
		return Vars::CritHack::ForceCrits.Value || 
			Vars::CritHack::AlwaysMeleeCrit.Value && m_bMelee ||
			m_bForce;
}

void CCritHack::UpdateInfo(CTFPlayer* pLocal, CTFWeaponBase* pWeapon)
{
	UpdateWeaponInfo(pLocal, pWeapon);

	m_bCritBanned = false;
	m_flDamageTilFlip = 0;
	if (!m_bMelee)
	{
		const float flNormalizedDamage = m_iCritDamage / TF_DAMAGE_CRIT_MULTIPLIER;
		float flCritChance = m_flCritChance + 0.1f;
		if (m_iRangedDamage && m_iCritDamage)
		{
			const float flObservedCritChance = flNormalizedDamage / (flNormalizedDamage + m_iRangedDamage - m_iCritDamage);
			m_bCritBanned = flObservedCritChance > flCritChance;
		}

		if (m_bCritBanned)
			m_flDamageTilFlip = flNormalizedDamage / flCritChance + flNormalizedDamage * 2 - m_iRangedDamage;
		else
			m_flDamageTilFlip = TF_DAMAGE_CRIT_MULTIPLIER * (flNormalizedDamage - flCritChance * (flNormalizedDamage + m_iRangedDamage - m_iCritDamage)) / (flCritChance - 1);
	}

	if (auto pResource = H::Entities.GetResource())
	{
		m_iResourceDamage = pResource->m_iDamage(I::EngineClient->GetLocalPlayer());
		/* // more of a proof of concept for resyncing crit damage
		{	// attempt to resync damages
			m_iRangedDamage = m_iResourceDamage - m_iMeleeDamage;

			float flObservedCritChance = pWeapon->m_flObservedCritChance();
			m_iCritDamage = (TF_DAMAGE_CRIT_MULTIPLIER * flObservedCritChance * m_iResourceDamage) / (1 + 2 * flObservedCritChance);
			SDK::Output("Info", std::format("{}, {}", m_iRangedDamage, m_iCritDamage).c_str());
		}
		 */
		m_iDesyncDamage = m_iRangedDamage + m_iMeleeDamage - m_iResourceDamage;
	}
}

bool CCritHack::WeaponCanCrit(CTFWeaponBase* pWeapon, bool bWeaponOnly)
{
	if (!bWeaponOnly && !pWeapon->AreRandomCritsEnabled() || SDK::AttribHookValue(1.f, "mult_crit_chance", pWeapon) <= 0.f)
		return false;

	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_PDA:
	case TF_WEAPON_PDA_ENGINEER_BUILD:
	case TF_WEAPON_PDA_ENGINEER_DESTROY:
	case TF_WEAPON_PDA_SPY:
	case TF_WEAPON_PDA_SPY_BUILD:
	case TF_WEAPON_BUILDER:
	case TF_WEAPON_INVIS:
	case TF_WEAPON_JAR_MILK:
	case TF_WEAPON_LUNCHBOX:
	case TF_WEAPON_BUFF_ITEM:
	case TF_WEAPON_FLAME_BALL:
	case TF_WEAPON_ROCKETPACK:
	case TF_WEAPON_JAR_GAS:
	case TF_WEAPON_LASER_POINTER:
	case TF_WEAPON_MEDIGUN:
	case TF_WEAPON_SNIPERRIFLE:
	case TF_WEAPON_SNIPERRIFLE_DECAP:
	case TF_WEAPON_SNIPERRIFLE_CLASSIC:
	case TF_WEAPON_COMPOUND_BOW:
	case TF_WEAPON_JAR:
	case TF_WEAPON_KNIFE:
	case TF_WEAPON_PASSTIME_GUN:
		return false;
	}

	return true;
}

void CCritHack::Reset()
{
	m_iCritDamage = 0;
	m_iRangedDamage = 0;
	m_iMeleeDamage = 0;
	m_iResourceDamage = 0;
	m_iDesyncDamage = 0;

	m_bCritBanned = false;
	m_flDamageTilFlip = 0;

	m_mHealthHistory.clear();


	//m_flLastDamageTime = 0.f;
}



int CCritHack::GetCritRequest(CUserCmd* pCmd, CTFWeaponBase* pWeapon)
{
	bool bCanCrit = m_iAvailableCrits > 0 && !m_bCritBanned;

	bool bSkip = Vars::CritHack::AvoidRandomCrits.Value;
	bool bDesync = CommandToSeed(pCmd->command_number) == pWeapon->m_iCurrentSeed();

	return bCanCrit && IsForcingCrits(pCmd) ? CritRequestEnum::Crit : bSkip || bDesync ? CritRequestEnum::Skip : CritRequestEnum::Any;
}

void CCritHack::Run(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	if (!pWeapon || !pLocal->IsAlive() || pLocal->IsAGhost() || !I::EngineClient->IsInGame())
		return;

	UpdateInfo(pLocal, pWeapon);
	if (pLocal->IsCritBoosted() || pWeapon->m_flCritTime() > I::GlobalVars->curtime || !WeaponCanCrit(pWeapon))
		return;

	if (pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN && pCmd->buttons & IN_ATTACK)
		pCmd->buttons &= ~IN_ATTACK2;

	bool bAttacking = G::Attacking /*== 1*/ || F::Ticks.m_bDoubletap || F::Ticks.m_bSpeedhack;
	if (m_bMelee)
	{
		bAttacking = G::CanPrimaryAttack && pCmd->buttons & IN_ATTACK;
		if (!bAttacking && pWeapon->GetWeaponID() == TF_WEAPON_FISTS)
			bAttacking = G::CanPrimaryAttack && pCmd->buttons & IN_ATTACK2;
	}
	else if (pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN && !(G::LastUserCmd->buttons & IN_ATTACK))
		bAttacking = false;
	else if (!bAttacking)
	{
		switch (pWeapon->GetWeaponID())
		{
		case TF_WEAPON_ROCKETLAUNCHER:
		case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
			if (pWeapon->IsInReload() && G::CanPrimaryAttack && SDK::AttribHookValue(0, "can_overload", pWeapon))
			{
				int iClip1 = pWeapon->m_iClip1();
				if (pWeapon->m_bRemoveable() && iClip1 > 0)
					bAttacking = true;
				else if (iClip1 >= pWeapon->GetMaxClip1() || iClip1 > 0 && pLocal->GetAmmoCount(pWeapon->m_iPrimaryAmmoType()) == 0)
					bAttacking = true;
			}
		}
	}
	if (!bAttacking || pWeapon->IsRapidFire() && I::GlobalVars->curtime < pWeapon->m_flLastRapidFireCritCheckTime() + 1.f)
		return;

	int iRequest = GetCritRequest(pCmd, pWeapon);
	if (iRequest == CritRequestEnum::Any)
		return;

	if (!Vars::Misc::Game::AntiCheatCompatibility.Value)
	{
		if (int iCommand = GetCritCommand(pWeapon, pCmd->command_number, iRequest == CritRequestEnum::Crit))
		{
			pCmd->command_number = iCommand;
			pCmd->random_seed = MD5_PseudoRandom(iCommand) & std::numeric_limits<int>::max();
		}
	}
	else if (Vars::Misc::Game::AntiCheatCritHack.Value)
	{
		if (!IsCritCommand(pCmd->command_number, pWeapon, iRequest == CritRequestEnum::Crit, false))
		{
			pCmd->buttons &= ~IN_ATTACK;
			pCmd->viewangles = G::OriginalCmd.viewangles;
			G::PSilentAngles = false;
		}
	}
}

int CCritHack::PredictCmdNum(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	auto getCmdNum = [&](int iCommandNumber)
		{
			if (!pWeapon || !pLocal->IsAlive() || !I::EngineClient->IsInGame() || Vars::Misc::Game::AntiCheatCompatibility.Value
				|| pLocal->IsCritBoosted() || pWeapon->m_flCritTime() > I::GlobalVars->curtime || !WeaponCanCrit(pWeapon))
				return iCommandNumber;

			UpdateInfo(pLocal, pWeapon);
			if (pWeapon->IsRapidFire() && I::GlobalVars->curtime < pWeapon->m_flLastRapidFireCritCheckTime() + 1.f)
				return iCommandNumber;

			int iRequest = GetCritRequest(pCmd, pWeapon);
			if (iRequest == CritRequestEnum::Any)
				return iCommandNumber;

			if (int iCommand = GetCritCommand(pWeapon, iCommandNumber, iRequest == CritRequestEnum::Crit))
				return iCommand;
			return iCommandNumber;
		};

	static int iCommandNumber = 0; // cache, don't constantly test

	static int iStaticCommand = 0;
	if (pCmd->command_number != iStaticCommand)
	{
		iCommandNumber = getCmdNum(pCmd->command_number);
		iStaticCommand = pCmd->command_number;
	}

	return iCommandNumber;
}

void CCritHack::Event(IGameEvent* pEvent, uint32_t uHash, CTFPlayer* pLocal)
{
	switch (uHash)
	{
	case FNV1A::Hash32Const("player_hurt"):
	{
		if (!pLocal)
			break;

		int iVictim = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid"));
		int iAttacker = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("attacker"));
		bool bCrit = pEvent->GetBool("crit") || pEvent->GetBool("minicrit");
		int iDamage = pEvent->GetInt("damageamount");
		int iHealth = pEvent->GetInt("health");
		int iWeaponID = pEvent->GetInt("weaponid");

		if (m_mHealthHistory.contains(iVictim))
		{
			auto& tHistory = m_mHealthHistory[iVictim];
			auto pVictim = I::ClientEntityList->GetClientEntity(iVictim)->As<CTFPlayer>();

			if (!iHealth)
			{
				iDamage = std::clamp(iDamage, 0, tHistory.m_iNewHealth);
				tHistory.m_iSpawnCounter = -1;
			}
			else if (pVictim && (pVictim->m_bFeignDeathReady() || pVictim->InCond(TF_COND_FEIGN_DEATH))) // damage number is spoofed upon sending, correct it
			{
				int iOldHealth = (tHistory.m_mHistory.contains(iHealth) ? tHistory.m_mHistory[iHealth].m_iOldHealth : tHistory.m_iNewHealth) % 32768;
				if (iHealth > iOldHealth)
				{
					for (auto& [_, tOldHealth] : tHistory.m_mHistory)
					{
						int iOldHealth2 = tOldHealth.m_iOldHealth % 32768;
						if (iOldHealth2 > iHealth)
							iOldHealth = iHealth > iOldHealth ? iOldHealth2 : std::min(iOldHealth, iOldHealth2);
					}
				}
				iDamage = std::clamp(iOldHealth - iHealth, 0, iDamage);
			}
		}
		if (iHealth)
			StoreHealthHistory(iVictim, iHealth);

		if (iVictim == iAttacker || iAttacker != I::EngineClient->GetLocalPlayer())
			break;

		if (auto pGameRules = I::TFGameRules())
		{
			auto pMatchDesc = pGameRules->GetMatchGroupDescription();
			if (pMatchDesc && pGameRules->m_iRoundState() != GR_STATE_RND_RUNNING)
			{
				switch (pMatchDesc->m_eMatchType)
				{
				case MATCH_TYPE_COMPETITIVE:
				case MATCH_TYPE_CASUAL:
					return;
				}
			}

			const int iInsanePlayerDamage = pGameRules->m_bPlayingMannVsMachine() ? 5000 : 1500;
			if (iDamage > iInsanePlayerDamage)
				return;
		}

		//m_flLastDamageTime = I::GlobalVars->curtime;

		CTFWeaponBase* pWeapon = nullptr;
		for (int i = 0; i < MAX_WEAPONS; i++)
		{
			auto pWeapon2 = pLocal->GetWeaponFromSlot(i);
			if (!pWeapon2 || pWeapon2->GetWeaponID() != iWeaponID)
				continue;

			pWeapon = pWeapon2;
			break;
		}

		if (!pWeapon || pWeapon->GetSlot() != SLOT_MELEE)
		{
			m_iRangedDamage += iDamage;
			if (bCrit && !pLocal->IsCritBoosted())
				m_iCritDamage += iDamage;
		}
		else
			m_iMeleeDamage += iDamage;

		break;
	}
	case FNV1A::Hash32Const("scorestats_accumulated_update"):
	case FNV1A::Hash32Const("mvm_reset_stats"):
		m_iRangedDamage = m_iCritDamage = m_iMeleeDamage = 0;
		break;
	case FNV1A::Hash32Const("client_beginconnect"):
	case FNV1A::Hash32Const("client_disconnect"):
	case FNV1A::Hash32Const("game_newmap"):
		Reset();
	}
}

void CCritHack::Store()
{
	for (int n = 1; n <= I::EngineClient->GetMaxClients(); n++)
	{
		auto pPlayer = I::ClientEntityList->GetClientEntity(n)->As<CTFPlayer>();
		if (pPlayer && pPlayer->IsAlive() && !pPlayer->IsAGhost())
			StoreHealthHistory(n, pPlayer->m_iHealth(), pPlayer);
	}
}

void CCritHack::StoreHealthHistory(int iIndex, int iHealth, CTFPlayer* pPlayer)
{
	bool bContains = m_mHealthHistory.contains(iIndex);
	auto& tHistory = m_mHealthHistory[iIndex];

	if (bContains && pPlayer)
	{	// deal with instant respawn damage desync better
		if (pPlayer->IsDormant())
			tHistory.m_iSpawnCounter = -1;
		else if (tHistory.m_iSpawnCounter == -1)
			tHistory.m_iSpawnCounter = pPlayer->m_iSpawnCounter();
		else if (tHistory.m_iSpawnCounter != pPlayer->m_iSpawnCounter())
			return; // wait for event
	}

	if (!bContains)
		tHistory = { iHealth, iHealth };
	else if (iHealth != tHistory.m_iNewHealth)
	{
		tHistory.m_iOldHealth = std::max(tHistory.m_iNewHealth, iHealth);
		tHistory.m_iNewHealth = iHealth;
	}

	tHistory.m_mHistory[iHealth % 32768] = { tHistory.m_iOldHealth, float(SDK::PlatFloatTime()) };
	while (tHistory.m_mHistory.size() > 3)
	{
		int iIndex2; float flMin = std::numeric_limits<float>::max();
		for (auto& [i, tStorage] : tHistory.m_mHistory)
		{
			if (tStorage.m_flTime < flMin)
				flMin = tStorage.m_flTime, iIndex2 = i;
		}
		tHistory.m_mHistory.erase(iIndex2);
	}
}

#ifdef SERVER_CRIT_DATA
MAKE_SIGNATURE(CTFGameStats_FindPlayerStats, "server.dll", "4C 8B C1 48 85 D2 75", 0x0);
MAKE_SIGNATURE(UTIL_PlayerByIndex, "server.dll", "48 83 EC ? 8B D1 85 C9 7E ? 48 8B 05", 0x0);

static void* s_pCTFGameStats = nullptr;
MAKE_HOOK(CTFGameStats_FindPlayerStats, S::CTFGameStats_FindPlayerStats(), void*,
	void* rcx, CBasePlayer* pPlayer)
{
	DEBUG_RETURN(CTFGameStats_FindPlayerStats, rcx, pPlayer);

	s_pCTFGameStats = rcx;
	return CALL_ORIGINAL(rcx, pPlayer);
}
#endif


static inline Color_t ColorFade(Color_t tColorA, Color_t tColorB, float t, float flScale)
{
	float percentage = fabsf(sin(t * flScale));
	Color_t newColor;

	newColor.r = (tColorB.r - tColorA.r) * percentage + tColorA.r;
	newColor.g = (tColorB.g - tColorA.g) * percentage + tColorA.g;
	newColor.b = (tColorB.b - tColorA.b) * percentage + tColorA.b;
	newColor.a = (tColorB.a - tColorA.a) * percentage + tColorA.a;
	return newColor;
}

void CCritHack::Draw(CTFPlayer* pLocal)
{
	if (!(Vars::Menu::Indicators.Value & Vars::Menu::IndicatorsEnum::CritHack) || !I::EngineClient->IsInGame())
		return;

	auto pWeapon = H::Entities.GetWeapon();
	if (!pWeapon || !pLocal->IsAlive() || pLocal->IsAGhost() || pWeapon->GetWeaponID() == TF_WEAPON_PASSTIME_GUN)
		return;

	int x = Vars::Menu::CritsDisplay.Value.x;
	int y = Vars::Menu::CritsDisplay.Value.y + 8;
	const auto& fFont = H::Fonts.GetFont(FONT_INDICATORS);
	const int nTall = fFont.m_nTall + H::Draw.Scale(1);
	constexpr EAlign align = ALIGN_TOP;

	auto iSlot = pWeapon->GetSlot();
	float flTickBase = TICKS_TO_TIME(pLocal->m_nTickBase());

	switch (Vars::Menu::CritDisplayStyle.Value)
	{
	case Vars::Menu::CritDisplayStyleEnum::RijiNv1:
	{
		constexpr Color_t tOutline = { 0,0,0,255 };
		constexpr Color_t tGreen = { 115, 221, 88, 255 };
		constexpr Color_t tRed = { 255,0,0,255 };

		const auto& fIndicatorFont = H::Fonts.GetFont(FONT_RIJIN_OLD);

		if (pLocal->IsCritBoosted())
		{
			H::Draw.StringOutlined(fIndicatorFont, x, y += nTall, tGreen, tOutline, align, "crit boosted");
			return;
		}
		else if (pWeapon->m_flCritTime() > flTickBase)
		{
			constexpr Color_t tBlue = { 138, 154, 209, 255 };
			H::Draw.StringOutlined(fIndicatorFont, x, y += nTall, tBlue, tOutline, align, "stream crits");
		}
		else if (m_bCritBanned && iSlot != SLOT_MELEE)
		{
			H::Draw.StringOutlined(fIndicatorFont, x, y += nTall, tRed, tOutline, align, "crit banned");
			const float flCritBanDamage = ceilf(m_flDamageTilFlip);
			if (flCritBanDamage >= 2000.f)
				H::Draw.StringOutlined(fIndicatorFont, x, y += nTall, tRed, tOutline, align, "2000+ damage until unban");
			else
				H::Draw.StringOutlined(fIndicatorFont, x, y += nTall, tRed, tOutline, align, std::format("{} damage until unban", flCritBanDamage).c_str());
			return;
		}
		else
		{
			H::Draw.StringOutlined(fIndicatorFont, x, y += nTall, m_iAvailableCrits == 0 ? tRed : tGreen, tOutline, align, std::format("{}/{} crits", m_iAvailableCrits, m_iPotentialCrits).c_str());
			static const auto tf_weapon_criticals_bucket_cap = U::ConVars.FindVar("tf_weapon_criticals_bucket_cap");
			if (pWeapon->m_flCritTokenBucket() == tf_weapon_criticals_bucket_cap->GetInt())
			{
				constexpr Color_t tPink = { 221, 146, 218, 255 };
				H::Draw.StringOutlined(fIndicatorFont, x, y += nTall, tPink, tOutline, align, "bucket full");
			}
		}

		const float flNormalizedDamage = m_iCritDamage / TF_DAMAGE_CRIT_MULTIPLIER;
		const float flObservedCritChance = flNormalizedDamage / (flNormalizedDamage + m_iRangedDamage - m_iCritDamage);
		if (flObservedCritChance > m_flCritChance)
		{
			constexpr Color_t tOrange = { 232, 158, 97, 255 };
			H::Draw.StringOutlined(fIndicatorFont, x, y += nTall, tOrange, tOutline, align, "high crit/shot ratio");
		}

		break;
	}
	case Vars::Menu::CritDisplayStyleEnum::Cathook:
	{
		bool shouldDrawBar = true;
		if (U::ConVars.FindVar("tf_weapon_criticals")->GetInt() == 0)
			shouldDrawBar = false;

		// draw info strings in corner
		bool continueStrings = true;
		std::vector<std::pair<std::string, Color_t>> critStrings = {};

		Color_t bucketColor = { 83, 188, 49, 255 };
		constexpr Color_t red = { 237, 42, 42, 255 };
		constexpr Color_t orange = { 255, 120, 0, 255 };
		if (!WeaponCanCrit(pWeapon, false))
		{
			bucketColor = red;
			critStrings.push_back({ "Weapon cannot randomly crit.", red });
			continueStrings = false;
		}
		else if (IsForcingCrits())
		{
			bucketColor = { 52, 235, 174, 255 };
			if (m_iAvailableCrits && !m_bCritBanned)
				critStrings.push_back({ "Forcing Crits!", red });
			else
				critStrings.push_back({ "Weapon can currently not crit!", red });
		}

		if (continueStrings)
		{
			if (m_iDesyncDamage)
				critStrings.push_back({ "Out of sync.", red });
			else if (m_bCritBanned && iSlot != SLOT_MELEE)
				critStrings.push_back({ std::format("Damage Until crit: {}", std::ceil(m_flDamageTilFlip)), orange });
			else if (!(m_iAvailableCrits && !m_bCritBanned))
			{
				if (pWeapon->IsRapidFire())
				{
					int iShots = m_iNextCrit;
					std::string critString = std::format("Shot{} until crit: {}", iShots == 1 ? "" : "s", iShots).c_str();
					if (pWeapon->m_flCritTime() >= I::GlobalVars->curtime)
					{
						critString += std::format(", {}s", pWeapon->m_flCritTime() + 1.0f - I::GlobalVars->curtime).c_str();
						critStrings.push_back({ critString, red });
					}
					else
						critStrings.push_back({ critString, orange });
				}
				else
					critStrings.push_back({ std::format("Shots until crit: {}", m_iNextCrit).c_str(), orange });
			}
			Color_t color = red;
			if (WeaponCanCrit(pWeapon, false) && (!m_bCritBanned || iSlot != SLOT_MELEE))
				color = { 0, 255, 0, 255 };
			critStrings.push_back({ std::format("Crit Bucket: {}", pWeapon->m_flCritTokenBucket()), color });
		}

		if (shouldDrawBar)
		{
			const float flBucketCap = U::ConVars.FindVar("tf_weapon_criticals_bucket_cap")->GetFloat();
			const float flBucketPercentage = pWeapon->m_flCritTokenBucket() / flBucketCap;
			float flBucketPercentagePostCrit = pWeapon->m_flCritTokenBucket();

			bool bRapidFire = pWeapon->IsRapidFire();
			float flFireRate = pWeapon->GetFireRate();

			float flBaseDamage = 0;
			float flDamage = pWeapon->GetDamage();
			{
				int nProjectilesPerShot = pWeapon->GetBulletsPerShot(true);
				if (iSlot != SLOT_MELEE && nProjectilesPerShot > 0)
					nProjectilesPerShot = SDK::AttribHookValue(nProjectilesPerShot, "mult_bullets_per_shot", pWeapon);
				else
					nProjectilesPerShot = 1;

				flBaseDamage = flDamage *= nProjectilesPerShot;
				if (pWeapon->IsRapidFire())
				{
					flDamage *= TF_DAMAGE_CRIT_DURATION_RAPID / pWeapon->GetFireRate();
					if (flDamage * TF_DAMAGE_CRIT_MULTIPLIER > flBucketCap)
						flDamage = flBucketCap / TF_DAMAGE_CRIT_MULTIPLIER;
				}
				flDamage *= TF_DAMAGE_CRIT_MULTIPLIER;
			}

			flBucketPercentagePostCrit = std::max(0.f, std::clamp(flBucketPercentagePostCrit + flBaseDamage, 0.f, flBucketCap) - flDamage) / flBucketCap;

			Color_t reductionColor = ColorFade(bucketColor, { 255, 255, 255, 255 }, I::GlobalVars->curtime, 2.0f);

			int iSize = Vars::Menu::CathookBarSize.Value;
			Color_t backgroundColor = { 96, 96, 96, 150 };
			float barBGSizeX = iSize * 2.0f;
			float barBGSizeY = iSize / 5.0f;

			x -= barBGSizeX / 2.f; y += barBGSizeY / 2.f + 8;

			H::Draw.FillRect(x - 5.0f, y - 5.0f, barBGSizeX + 10.0f, barBGSizeY + 10.0f, backgroundColor);

			if (m_iDesyncDamage || m_bCritBanned && iSlot != SLOT_MELEE || !WeaponCanCrit(pWeapon, false))
			{
				H::Draw.StartClipping(x, y, barBGSizeX * flBucketPercentage, barBGSizeY);
				H::Draw.FillRect(x, y, barBGSizeX * flBucketPercentage, barBGSizeY, bucketColor);
				H::Draw.EndClipping();

				std::string barString;
				if (m_iDesyncDamage)
					barString = "Out of sync.";
				else if (m_bCritBanned && iSlot != SLOT_MELEE)
					barString = std::format("{} Damage until Crit!", ceilf(m_flDamageTilFlip));
				else
				{
					int iShots = m_iNextCrit;
					barString = std::format("{} Shot{} until Crit!", iShots, iShots == 1 ? "" : "s");
				}

				if (!barString.empty())
				{
					float textX = x + iSize;
					float textY = y + iSize / 5.0f;
					Vec2 vTextSize = H::Draw.GetTextSize(barString.c_str(), fFont);
					float sx = vTextSize.x;
					float sy = vTextSize.y;
					H::Draw.StringOutlined(fFont, textX - sx / 2, textY + sy, red, { 0, 0, 0, 255 }, ALIGN_LEFT, barString.c_str());
				}
			}
			if (!((m_bCritBanned && iSlot != SLOT_MELEE) || !WeaponCanCrit(pWeapon, false)))
			{
				float flBucketDrawPercentage = flBucketPercentagePostCrit;

				float xOffsetBucket = flBucketDrawPercentage * barBGSizeX;
				if (xOffsetBucket > 0.0f)
				{
					H::Draw.StartClipping(x, y, xOffsetBucket, barBGSizeY);
					H::Draw.FillRect(x, y, xOffsetBucket, barBGSizeY, bucketColor);
					H::Draw.EndClipping();
				}
				else
					xOffsetBucket = 0.0f;

				float flReductionDrawPercentage = flBucketPercentage - flBucketPercentagePostCrit;
				if (flBucketDrawPercentage < 0.0f)
					flBucketDrawPercentage = 0.0f;
				H::Draw.StartClipping(x + xOffsetBucket, y, barBGSizeX * flReductionDrawPercentage, barBGSizeY);
				H::Draw.FillRect(x + xOffsetBucket, y, barBGSizeX * flReductionDrawPercentage, barBGSizeY, reductionColor);
				H::Draw.EndClipping();
				y += nTall + H::Draw.Scale(1);
			}

			int critStringY = Vars::Menu::CathookTextYPos.Value + H::Draw.GetTextSize("Shiftable Ticks:", fFont).y;
			for (const std::pair<std::string, Color_t>& critString : critStrings)
			{
				H::Draw.StringOutlined(fFont, 10, critStringY, critString.second, { 0, 0, 0, 255 }, ALIGN_LEFT, critString.first.c_str());
				critStringY += nTall + H::Draw.Scale(1);
			}
		}
		break;
	}
	case Vars::Menu::CritDisplayStyleEnum::RijiNv2:
	{
		const auto& fIndicatorFont = H::Fonts.GetFont(FONT_RIJIN);
		const auto& fOtherFont = H::Fonts.GetFont(FONT_RIJIN_MISC);

		constexpr Color_t tRed = Color_t(255, 100, 100, 255);
		constexpr Color_t tYellow = Color_t(234, 196, 42, 255);
		constexpr Color_t tGray = Color_t(200, 200, 200, 255);
		constexpr Color_t tWhite = Color_t(255, 255, 255, 255);
		constexpr Color_t tBlack = Color_t(0, 0, 0, 255);

		if (U::ConVars.FindVar("tf_weapon_criticals")->GetInt() == 0)
		{
			H::Draw.String(fFont, x, y, tRed, ALIGN_CENTER, "CRITS DISABLED");
			return;
		}
		else if (Vars::Misc::Game::AntiCheatCompatibility.Value)
		{
			H::Draw.StringOutlined(fOtherFont, x, y, tGray, tBlack, ALIGN_CENTER, "COMMAND NUMBER CHANGE NOT ALLOWED");
			return;
		}

		y -= fIndicatorFont.m_nTall;
		constexpr int iBarW = 50;
		constexpr int iBarH = 3;

		if (m_flDamage >= 0.f)
		{
			if (pLocal->IsCritBoosted())
			{
				const bool bFlashOn = int(I::GlobalVars->curtime * 2) % 2 == 0;
				H::Draw.StringOutlined(fOtherFont, x, y += fOtherFont.m_nTall, bFlashOn ? tWhite : tGray, tBlack, ALIGN_CENTER, "CRIT BOOST");
				return;
			}
			else if (pWeapon->m_flCritTime() > flTickBase)
			{
				const float flTime = pWeapon->m_flCritTime() - flTickBase;
				H::Draw.StringOutlined(fOtherFont, x, y += fOtherFont.m_nTall, Vars::Menu::Theme::Accent.Value, tBlack, ALIGN_CENTER, "STREAM CRITS");

				const float flRatio = std::clamp(flTime / TF_DAMAGE_CRIT_DURATION_RAPID, 0.0f, 1.0f);
				H::Draw.FillRectOutline(x - 23, y + fOtherFont.m_nTall, iBarW, iBarH, Vars::Menu::Theme::Background.Value, tGray);
				H::Draw.FillRect(x - 23, y + fOtherFont.m_nTall, int(iBarW * flRatio), iBarH, tWhite);
				return;
			}
			else if (m_bCritBanned && iSlot != SLOT_MELEE)
			{
				H::Draw.StringOutlined(fOtherFont, x, y += fOtherFont.m_nTall, tRed, tBlack, ALIGN_CENTER, "CRIT BANNED");
				H::Draw.StringOutlined(fOtherFont, x, y += fOtherFont.m_nTall, tYellow, tBlack, ALIGN_CENTER, std::format("DEAL {} DAMAGE", ceilf(m_flDamageTilFlip)).c_str());
				return;
			}

			// crit text
			Color_t tTextColor = Vars::Menu::Theme::Accent.Value;
			if (m_iAvailableCrits == 0)
				tTextColor = tGray;
			H::Draw.StringOutlined(fIndicatorFont, x, y += fIndicatorFont.m_nTall, tTextColor, tBlack, ALIGN_CENTER, std::format("{}/{} crits", m_iAvailableCrits, m_iPotentialCrits).c_str());

			// shots until crit bar
			if (m_iAvailableCrits < m_iPotentialCrits && iSlot != SLOT_MELEE)
			{
				const float flTotalShots = ceilf(m_flCost / std::max(0.f, m_flDamage));
				const float flRatio = std::clamp(float(flTotalShots - m_iNextCrit) / flTotalShots, 0.f, 1.f);
				H::Draw.FillRectOutline(x - 23, y + fIndicatorFont.m_nTall, iBarW, iBarH, Vars::Menu::Theme::Background.Value, tGray);
				H::Draw.FillRect(x - 23, y + fIndicatorFont.m_nTall, int(iBarW * flRatio), iBarH, Vars::Menu::Theme::Accent.Value);
			}
		}
		else
			H::Draw.StringOutlined(fOtherFont, x, y += fOtherFont.m_nTall, tYellow, tBlack, align, "CALCULATING");
		break;
	}
	case Vars::Menu::CritDisplayStyleEnum::LMAOBox:
	{
		constexpr Color_t tBlack = { 0, 0, 0, 255 };
		const int iSize = Vars::Menu::LBoxCritBarSize.Value;
		const float barBGSizeX = 70 * iSize;
		const float barBGSizeY = 3 * iSize;

		x -= barBGSizeX / 2.f; y += barBGSizeY / 2.f + 8;

		if (U::ConVars.FindVar("tf_weapon_criticals")->GetInt() == 0 || !WeaponCanCrit(pWeapon))
			return;

		// draw background & outline
		H::Draw.FillRectOutline(x - 1, y - 1, barBGSizeX, barBGSizeY + 2, tBlack, Vars::Menu::Theme::Accent.Value);

		if (pWeapon->m_flCritTime() > flTickBase)
		{
			const float flTime = pWeapon->m_flCritTime() - flTickBase;
			const float flRatio = std::clamp(flTime / TF_DAMAGE_CRIT_DURATION_RAPID, 0.0f, 1.0f);

			H::Draw.StringOutlined(fFont, x + barBGSizeX / 2, y - fFont.m_nTall - 1, Vars::Menu::Theme::Accent.Value, tBlack, ALIGN_CENTER, "CRIT!");
			H::Draw.FillRectOutline(x, y, barBGSizeX * flRatio, barBGSizeY, { 0, 255, 0, 255 });
			return;
		}

		const float flBucket = pWeapon->m_flCritTokenBucket();
		Color_t tBarColor = { 0, 255, 0, 255 };
		if (m_bCritBanned && iSlot != SLOT_MELEE || !m_iAvailableCrits)
			tBarColor = { 255, 0, 0, 255 };

		const float flBucketCap = U::ConVars.FindVar("tf_weapon_criticals_bucket_cap")->GetFloat();
		const float flBucketBottom = U::ConVars.FindVar("tf_weapon_criticals_bucket_bottom")->GetFloat();

		// calculate damage our crit will do
		float flDamage = pWeapon->GetDamage();
		{
			int nProjectilesPerShot = pWeapon->GetBulletsPerShot(true);
			if (iSlot != SLOT_MELEE && nProjectilesPerShot > 0)
				nProjectilesPerShot = SDK::AttribHookValue(nProjectilesPerShot, "mult_bullets_per_shot", pWeapon);
			else
				nProjectilesPerShot = 1;

			const float flBaseDamage = flDamage *= nProjectilesPerShot;
			if (pWeapon->IsRapidFire())
			{
				flDamage *= TF_DAMAGE_CRIT_DURATION_RAPID / pWeapon->GetFireRate();
				if (flDamage * TF_DAMAGE_CRIT_MULTIPLIER > flBucketCap)
					flDamage = flBucketCap / TF_DAMAGE_CRIT_MULTIPLIER;
			}
			flDamage *= TF_DAMAGE_CRIT_MULTIPLIER;
		}
		const float flPostCritBucket = std::clamp(flBucket - flDamage, flBucketBottom, flBucketCap);

		const float flBucketPct = std::clamp(flBucket / flBucketCap, 0.f, 1.f);
		float flRemainingPct = std::clamp(flPostCritBucket / flBucketCap, 0.f, 1.f);

		if (iSlot != SLOT_MELEE) // lbox does this for god knows what reason
			H::Draw.FillRectOutline(x, y, std::min(barBGSizeX * flBucketPct, barBGSizeX - 2), barBGSizeY, tBarColor);
		else
			flRemainingPct = flBucketPct;
		H::Draw.FillRectOutline(x, y, std::min(barBGSizeX * flRemainingPct, barBGSizeX - 2), barBGSizeY, Vars::Menu::Theme::Accent.Value);

		break;
	}
	case Vars::Menu::CritDisplayStyleEnum::Ateris:
	{
		if (U::ConVars.FindVar("tf_weapon_criticals")->GetInt() == 0 || !WeaponCanCrit(pWeapon))
			return;

		const auto pWeapon = H::Entities.GetWeapon();
		if (!pWeapon)
			return;

		const auto& fIndicatorFont = H::Fonts.GetFont(FONT_ATERIS);
		
		constexpr int iBarW = 180;
		constexpr int iBarH = 15;
		const int iPosX = x - iBarW / 2, iPosY = y + fFont.m_nTall + 6;

		H::Draw.FillRectOutline(iPosX, iPosY, iBarW, iBarH, Vars::Menu::Theme::Background.Value);

		Color_t tGradientStart = Vars::Menu::Theme::Accent.Value;
		constexpr Color_t tGradientEnd = { 20, 20, 20, 50 };

		const bool bStreaming = pWeapon->m_flCritTime() > flTickBase;
		float flTargetRatio = float(m_iAvailableCrits) / std::max(m_iPotentialCrits, 1);
		if (bStreaming)
		{
			const float flTime = pWeapon->m_flCritTime() - flTickBase;
			flTargetRatio = std::clamp(flTime / TF_DAMAGE_CRIT_DURATION_RAPID, 0.0f, 1.0f);
		}
		else if (m_iPotentialCrits == 1)
		{
			const float flTotalShots = ceilf(m_flCost / std::max(0.f, m_flDamage));
			flTargetRatio = std::clamp(float(flTotalShots - m_iNextCrit) / flTotalShots, 0.f, 1.f);
		}

		static float flInterpRatio = 0.f;
		flInterpRatio += (flTargetRatio - flInterpRatio) * 0.03f;
		if (flInterpRatio < 0.01f)
			flInterpRatio = 0.f;

		if (m_bCritBanned && iSlot != SLOT_MELEE && !bStreaming)
			tGradientStart = { 50, 50, 50, 255 };
		else
			tGradientStart.a = std::lerp(20.f, 255.f, flInterpRatio);

		const int iFillW = std::min(int(flInterpRatio * iBarW) + 10, iBarW);
		H::Draw.GradientRect(iPosX, iPosY, iFillW, iBarH / 2, tGradientStart, tGradientEnd, false, true);
		H::Draw.GradientRect(iPosX, iPosY + iBarH / 2, iFillW, iBarH / 2, tGradientEnd, tGradientStart, false, true);
		H::Draw.LineRect(iPosX, iPosY, iFillW, iBarH, Vars::Menu::Theme::Accent.Value);

		constexpr Color_t tOutline = Color_t(0, 0, 0, 255);
		static Color_t tStatusColor = Color_t(255, 255, 255, 255);
		std::string sStatusText;
		if (pLocal->IsCritBoosted() || bStreaming)
		{
			tStatusColor = { 0, 168, 244, 255 };
			sStatusText = "BOOSTED";
		}
		else if (m_bCritBanned && iSlot != SLOT_MELEE)
		{
			tStatusColor = { 255, 100, 100, 255 };
			sStatusText = std::format("DEAL {} DAMAGE", ceilf(m_flDamageTilFlip));
		}
		else if (m_iAvailableCrits > 0 && pWeapon->IsRapidFire() && flTickBase < pWeapon->m_flLastRapidFireCritCheckTime() + 1.f)
		{
			tStatusColor = { 255, 168, 29, 255 };

			float flTime = pWeapon->m_flLastRapidFireCritCheckTime() + 1.f - flTickBase;
			sStatusText = std::format("WAIT: {:.1f}", flTime);
		}
		else if (!m_iAvailableCrits)
		{
			tStatusColor = { 255, 100, 100, 255 };
			sStatusText = "NO CRITS";
		}
		else
		{
			tStatusColor = { 10, 188, 105, 255 };
			sStatusText = std::format("DAMAGE: {}", m_iResourceDamage);

			if(iSlot != SLOT_MELEE)
			{
				std::string sTotalDamage = std::format("TOTAL DMG: {} / {}", m_iCritDamage, m_iRangedDamage);
				H::Draw.StringOutlined(fIndicatorFont, iPosX + 1, iPosY + iBarH + fIndicatorFont.m_nTall, { 255, 255, 255, 255 }, tOutline, ALIGN_LEFT, sTotalDamage.c_str());

				if (m_flDamageTilFlip)
				{
					std::string sSafeDamage = std::format("SAFE: {}", floor(m_flDamageTilFlip));
					const int iTextWidth = H::Draw.GetTextSize(sSafeDamage.c_str(), fIndicatorFont).x;
					H::Draw.StringOutlined(fIndicatorFont, iPosX + iBarW - iTextWidth - 1, iPosY + iBarH + fIndicatorFont.m_nTall, tStatusColor, tOutline, ALIGN_LEFT, sSafeDamage.c_str());
				}
			}
		}

		const int iLabelY = iPosY - fIndicatorFont.m_nTall + 3;
		const int iTextWidth = H::Draw.GetTextSize(sStatusText.c_str(), fIndicatorFont).x;

		H::Draw.StringOutlined(fIndicatorFont, iPosX + 1, iLabelY, { 255,255,255,255 }, tOutline, ALIGN_LEFT, std::format("CRITS: {} / {}", m_iAvailableCrits, m_iPotentialCrits).c_str());
		H::Draw.StringOutlined(fIndicatorFont, iPosX + iBarW - iTextWidth - 1, iLabelY, tStatusColor, tOutline, ALIGN_LEFT, sStatusText.c_str());
		break;
	}
	case Vars::Menu::CritDisplayStyleEnum::Nitro:
	{
		x -= H::Draw.Scale(45);

		if (!WeaponCanCrit(pWeapon) || U::ConVars.FindVar("tf_weapon_criticals")->GetInt() == 0)
		{
			H::Draw.StringOutlined(fFont, x, y, Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, "Weapon can't crit");
			return;
		}

		auto bRapidFire = pWeapon->IsRapidFire();
		y -= nTall;

		if (Vars::Misc::Game::AntiCheatCompatibility.Value)
			H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, "Anticheat compatibility");

		if (m_flDamage >= 0.f)
		{
			if (pWeapon->m_flCritTime() > flTickBase)
			{
				float flTime = pWeapon->m_flCritTime() - flTickBase;
				H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Colors::IndicatorTextMisc.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, std::format("Streaming crits {:.1f}s", flTime).c_str());
			}
			else if (m_bCritBanned && iSlot != SLOT_MELEE)
				H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, std::format("deal {} damage", ceilf(m_flDamageTilFlip)).c_str());
			//H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, "Crit banned");
			else
			{
				if (m_iAvailableCrits > 0)
				{
					if (!bRapidFire || flTickBase > pWeapon->m_flLastRapidFireCritCheckTime() + 1.f)
						H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Colors::IndicatorTextGood.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, "Crit Ready");
					else
					{
						float flTime = pWeapon->m_flLastRapidFireCritCheckTime() + 1.f - flTickBase;
						H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, std::format("Wait {:.1f}s", flTime).c_str());
					}
				}
				else
				{
					int iShots = m_iNextCrit;
					H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, std::format("Crit in {}{} shot{}", iShots, iShots == 1000 ? "+" : "", iShots == 1 ? "" : "s").c_str());
					//H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, "No crits");
				}
			}

			int iCrits = m_iAvailableCrits;
			H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, std::format("{}{} / {} potential crits", iCrits, iCrits == 1000 ? "+" : "", m_iPotentialCrits).c_str());

			if (m_iNextCrit && iCrits)
			{
				int iShots = m_iNextCrit;
				H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, std::format("Next in {}{} shot{}", iShots, iShots == 1000 ? "+" : "", iShots == 1 ? "" : "s").c_str());
			}
		}
		else
			H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, "Calculating");

		if (m_flDamageTilFlip && iSlot != SLOT_MELEE)
		{
			if (!m_bCritBanned)
				H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Colors::IndicatorTextGood.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, std::format("{} damage", floor(m_flDamageTilFlip)).c_str());
			//else
			//	H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Colors::IndicatorTextBad.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, std::format("Deal {} damage", ceilf(m_flDamageTilFlip)).c_str());
		}

		if (m_iDesyncDamage)
			H::Draw.StringOutlined(fFont, x, y += nTall, m_iDesyncDamage > 0 ? Vars::Colors::IndicatorTextBad.Value : Vars::Colors::IndicatorTextGood.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, std::format("Damage desync {}{}", m_iDesyncDamage > 0 ? "+" : "", m_iDesyncDamage).c_str());
	}
	}

	if (Vars::Debug::Info.Value)
	{
		H::Draw.StringOutlined(fFont, x, y += nTall * 2, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, align, std::format("RangedDamage: {}, CritDamage: {}", m_iRangedDamage, m_iCritDamage).c_str());

#ifdef SERVER_CRIT_DATA
		H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, align, std::format("AllDamage: {} ({})", m_iRangedDamage + m_iMeleeDamage, m_iMeleeDamage).c_str());

		if (s_pCTFGameStats)
		{
			if (auto pPlayer2 = S::UTIL_PlayerByIndex.Call<void*>(I::EngineClient->GetLocalPlayer()))
			{
				if (auto pPlayerStats = S::CTFGameStats_FindPlayerStats.Call<PlayerStats_t*>(s_pCTFGameStats, pPlayer2))
				{
					int& iRangedDamage = pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_RANGED];
					int& iCritDamage = pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_RANGED_CRIT_RANDOM];
					int& iDamage = pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE];

					//iRangedDamage = m_iRangedDamage;
					//iCritDamage = m_iCritDamage = 0;
					//iDamage = m_iRangedDamage + m_iMeleeDamage;

					H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Valu, Vars::Menu::Theme::Background.Value, align, std::format("RangedDamage: {}. CritDamage: {}", iRangedDamage, iCritDamage.c_str());
					H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, align, std::format("AllDamage: {} ({})", iDamage, iDamage - iRangedDamage).c_str());
			}
		}

		H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, align, std::format("ResourceDamage: {} ({})", m_iResourceDamage, m_iMeleeDamage).c_str());
#endif

		H::Draw.StringOutlined(fFont, x, y += nTall * 2, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, align, std::format("Bucket: {}, Shots: {}, Crits: {}", pWeapon->m_flCritTokenBucket(), pWeapon->m_nCritChecks(), pWeapon->m_nCritSeedRequests()).c_str());
		H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, align, std::format("Damage: {}, Cost: {}", m_flDamage, m_flCost).c_str());
		H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, align, std::format("CritChance: {:.2f} ({:.2f})", m_flCritChance, m_flCritChance + 0.1f).c_str());
	}
}