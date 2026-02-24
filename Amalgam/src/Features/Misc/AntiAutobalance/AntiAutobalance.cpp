#include "AntiAutobalance.h"

//#ifdef ANTIAUTOBALANCETESTING
#define FOR_EACH_VEC( vecName, iteratorName ) \
	for ( int iteratorName = 0; iteratorName < (vecName).Count(); iteratorName++ )
#define FOR_EACH_VEC_BACK( vecName, iteratorName ) \
	for ( int iteratorName = (vecName).Count()-1; iteratorName >= 0; iteratorName-- )

void CAntiAutobalance::Reset()
{
	m_eCurrentState = AB_STATE_INACTIVE;
	m_iLightestTeam = m_iHeaviestTeam = TEAM_INVALID;
	m_nNeeded = 0;
	m_flNextStateChange = -1.f;

	m_vecCandidates.Purge();
}

bool CAntiAutobalance::ShouldBeActive() const
{
	const auto pGameRules = I::TFGameRules();
	if (!pGameRules)
		return false;

	if (pGameRules->m_bIsInTraining() || pGameRules->m_bIsInItemTestingMode())
		return false;

	static auto tf_arena_use_queue = U::ConVars.FindVar("tf_arena_use_queue");
	if (pGameRules->m_nGameType() == TF_GAMETYPE_ARENA && tf_arena_use_queue && tf_arena_use_queue->GetBool())
		return false;

	static auto tf_gamemode_community = U::ConVars.FindVar("tf_gamemode_community");
	if (tf_gamemode_community && tf_gamemode_community->GetBool())
		return false;

	static auto mp_teams_unbalance_limit = U::ConVars.FindVar("mp_teams_unbalance_limit");
	if (mp_teams_unbalance_limit && mp_teams_unbalance_limit->GetInt() <= 0)
		return false;

	const IMatchGroupDescription* pMatchDesc = pGameRules->GetMatchGroupDescription();
	if (pMatchDesc)
		return pMatchDesc->m_bUseAutoBalance;

	static auto mp_tournament = U::ConVars.FindVar("mp_tournament");
	if (mp_tournament && mp_tournament->GetBool())
		return false;

	static auto mp_autoteambalance = U::ConVars.FindVar("mp_autoteambalance");
	return mp_autoteambalance && mp_autoteambalance->GetInt() == 2;
}

bool CAntiAutobalance::AreTeamsUnbalanced()
{
	const auto pGameRules = I::TFGameRules();
	if (!pGameRules)
		return false;

	if (pGameRules->m_iRoundState() != GR_STATE_RND_RUNNING)
		return false;

	static auto mp_teams_unbalance_limit = U::ConVars.FindVar("mp_teams_unbalance_limit");
	if (mp_teams_unbalance_limit->GetInt() <= 0)
		return false;

	if (pGameRules->m_bHelltowerPlayersInHell())
		return false;

	if (!IsOkayToBalancePlayers())
		return false;

	const auto pLocal = H::Entities.GetLocal();
	if (!pLocal)
		return false;

	int nDiffBetweenTeams = 0;
	m_iLightestTeam = m_iHeaviestTeam = TEAM_INVALID;
	m_nNeeded = 0;

	int pRedTeamAmt = 0;
	int pBlueTeamAmt = 0;
	if (pLocal->m_iTeamNum() == TF_TEAM_RED)
	{
		pRedTeamAmt = (int)H::Entities.GetGroup(EntityEnum::PlayerTeam).size();
		pBlueTeamAmt = (int)H::Entities.GetGroup(EntityEnum::PlayerEnemy).size();
	}
	else
	{
		pRedTeamAmt = (int)H::Entities.GetGroup(EntityEnum::PlayerEnemy).size();
		pBlueTeamAmt = (int)H::Entities.GetGroup(EntityEnum::PlayerTeam).size();
	}

	m_iLightestTeam = pRedTeamAmt > pBlueTeamAmt ? TF_TEAM_BLUE : TF_TEAM_RED;
	m_iHeaviestTeam = pRedTeamAmt > pBlueTeamAmt ? TF_TEAM_RED : TF_TEAM_BLUE;

	nDiffBetweenTeams = abs(pRedTeamAmt - pBlueTeamAmt);

	if (nDiffBetweenTeams > mp_teams_unbalance_limit->GetInt())
	{
		m_nNeeded = nDiffBetweenTeams / 2;
#ifdef _DEBUG
		SDK::Output("antiautobalance", std::format("Teams are unbalanced. Heaviest: {} Lightest: {} (red = 2 blu = 3)", int(m_iHeaviestTeam), (int)m_iLightestTeam).c_str(), Color_t(0, 255, 0, 255), OUTPUT_CONSOLE | OUTPUT_CHAT);
#endif
		return true;
	}

	return false;
}

bool CAntiAutobalance::IsAlreadyCandidate(CTFPlayer* pTFPlayer) const
{
	FOR_EACH_VEC(m_vecCandidates, i)
	{
		if (m_vecCandidates[i].hPlayer == pTFPlayer)
			return true;
	}

	return false;
}

static bool IsCompetitiveMode()
{
	const IMatchGroupDescription* pMatchDesc = I::TFGameRules()->GetMatchGroupDescription();
	if (pMatchDesc)
		return pMatchDesc->m_eMatchType == MATCH_TYPE_COMPETITIVE || pMatchDesc->m_eMatchType == MATCH_TYPE_CASUAL;

	return false;
}

static inline TF_GC_TEAM GetGCTeamForGameTeam(int nGameTeam)
{
	const auto pGameRules = I::TFGameRules();
	if (!pGameRules)
		return TF_GC_TEAM_NOTEAM;

	if (nGameTeam == TF_TEAM_BLUE)
		return IsCompetitiveMode() && pGameRules->m_bTeamsSwitched() ? TF_GC_TEAM_DEFENDERS : TF_GC_TEAM_INVADERS;
	else if (nGameTeam == TF_TEAM_RED)
		return IsCompetitiveMode() && pGameRules->m_bTeamsSwitched() ? TF_GC_TEAM_INVADERS : TF_GC_TEAM_DEFENDERS;

	return TF_GC_TEAM_NOTEAM;
}

static inline int GetTotalSkillRatingForTeam(TF_GC_TEAM nTeam)
{
	int nSkillRating = 0;

	if (auto pLobby = I::TFGCClientSystem->GetLobby())
	{
		int iMembers = pLobby->GetNumMembers();
		for (int i = 0; i < iMembers; i++)
		{
			ConstTFLobbyPlayer pDetails; pLobby->GetMemberDetails(&pDetails, i);
			auto pProto = pDetails.Proto();
			if (pProto->team == nTeam)
				nSkillRating += pProto->normalized_rating;
		}
	}

	return nSkillRating;
}

double CAntiAutobalance::GetTeamAutoBalanceScore(int nTeam) const
{
	void* pMatch = I::TFGCClientSystem->GetLiveMatch();
	if (pMatch && I::TFGameRules())
	{
		const auto retval = GetTotalSkillRatingForTeam(GetGCTeamForGameTeam(nTeam));
#ifdef _DEBUG
		SDK::Output("antiautobalance", std::format("team {} has score {}", nTeam, retval).c_str());
#endif
		return retval;
	}

	int nTotalScore = 0;
	int nTeamScore = 0;
	const auto pTFPlayerResource = H::Entities.GetResource();
	if (pTFPlayerResource)
	{
		// Tally up total score across everyone and for the specified team
		for (int i = 1; i <= MAX_PLAYERS; i++)
		{
			auto pPlayer = I::ClientEntityList->GetClientEntity(i)->As<CTFPlayer>();
			if (!pPlayer)
				continue;

			if (pPlayer->m_iTeamNum() == nTeam)
				nTeamScore = pTFPlayerResource->m_iTotalScore(pPlayer->entindex());

			nTotalScore += pTFPlayerResource->m_iTotalScore(pPlayer->entindex());
		}
	}

#ifdef _DEBUG
	SDK::Output("antiautobalance", std::format("team {} has score {}", nTeam, nTeamScore).c_str());
	SDK::Output("antiautobalance", std::format("total score is {}", nTotalScore).c_str());
	SDK::Output("antiautobalance", std::format("returning team score of {}", (double)nTeamScore / (double)std::max(1, nTotalScore)).c_str());
#endif

	return (double)nTeamScore / (double)std::max(1, nTotalScore);
}

double CAntiAutobalance::GetPlayerAutoBalanceScore(CTFPlayer* pTFPlayer) const
{
	if (!pTFPlayer)
		return 0.0;

	if (auto pLobby = I::TFGCClientSystem->GetLobby())
	{
		player_info_t pi{};
		if (I::EngineClient->GetPlayerInfo(pTFPlayer->entindex(), &pi))
		{
			CSteamID tDesiredSteamID = CSteamID(pi.friendsID, 1, k_EUniversePublic, k_EAccountTypeIndividual);
			if (tDesiredSteamID.IsValid())
			{
				int iMembers = pLobby->GetNumMembers();
				for (int i = 0; i < iMembers; i++)
				{
					auto tSteamID = CSteamID(); pLobby->GetMember(&tSteamID, i);
					if (tSteamID.GetAccountID() != tDesiredSteamID.GetAccountID())
						continue;

					ConstTFLobbyPlayer pDetails; pLobby->GetMemberDetails(&pDetails, i);
					auto pProto = pDetails.Proto();
					auto retval = pProto->normalized_rating;
#ifdef _DEBUG
					SDK::Output("antiautobalance", std::format("player {} has score of {}", pi.name, retval).c_str());
#endif
					return retval;
				}
			}
		}
	}

	int nPlayerScore = 0;
	int nTotalScore = 0;
	const auto pTFPlayerResource = H::Entities.GetResource();
	if (pTFPlayerResource)
	{
		// Tally up total score across everyone and find the particular player's score
		for (int i = 1; i <= MAX_PLAYERS; i++)
		{
			auto pPlayer = I::ClientEntityList->GetClientEntity(i);
			if (!pPlayer)
				continue;

			if(pPlayer == pTFPlayer)
				nPlayerScore = pTFPlayerResource->m_iTotalScore(pPlayer->entindex());

			nTotalScore += pTFPlayerResource->m_iTotalScore(pPlayer->entindex());
		}
	}

#ifdef _DEBUG
	SDK::Output("antiautobalance", std::format("player has score {}", nPlayerScore).c_str());
	SDK::Output("antiautobalance", std::format("total score is {}", nTotalScore).c_str());
	double retval = (double)nPlayerScore / (double)std::max(1, nTotalScore);
	SDK::Output("antiautobalance", std::format("returning player score of {}", retval).c_str());
	return retval;
#else
	return (double)nPlayerScore / (double)std::max(1, nTotalScore);
#endif
}

bool CAntiAutobalance::ValidateCandidates()
{
	FOR_EACH_VEC_BACK(m_vecCandidates, i)
	{
		CTFPlayer* pTFPlayer = m_vecCandidates[i].hPlayer.Get();
		if (!pTFPlayer)
			m_vecCandidates.Remove(i);
	}

	return m_vecCandidates.Count() > 0;
}

CTFPlayer* CAntiAutobalance::FindNextCandidate()
{
	CTFPlayer* pRetVal = NULL;

	CUtlVector<CTFPlayer*> vecCandidates;
	const auto pLocal = H::Entities.GetLocal();

	for(CBaseEntity* pEntity : H::Entities.GetGroup(EntityEnum::PlayerAll))
	{
		if (pEntity->m_iTeamNum() != m_iHeaviestTeam)
			continue;

		auto pTFPlayer = pEntity->As<CTFPlayer>();
		if(pTFPlayer && !IsAlreadyCandidate(pTFPlayer))
			vecCandidates.AddToTail(pTFPlayer);
	}
#ifdef _DEBUG
	SDK::Output("antiautobalance", std::format("FindNextCandidate : found {} candidates on team {}", vecCandidates.Count(), m_iHeaviestTeam).c_str());
#endif

	// no need to go any further if there's only one candidate
	if (vecCandidates.Count() == 1)
		pRetVal = vecCandidates[0];
	else if (vecCandidates.Count() > 1)
	{
		double fTotalDiff = fabs(GetTeamAutoBalanceScore(m_iHeaviestTeam) - GetTeamAutoBalanceScore(m_iLightestTeam));
		double fAverageNeeded = ( fTotalDiff / 2.0 ) / m_nNeeded;

		// now look for a player on the heaviest team with skill rating closest to that average
		float fClosest = FLT_MAX;
		FOR_EACH_VEC(vecCandidates, iIndex)
		{
			double fDiff = fabs(fAverageNeeded - GetPlayerAutoBalanceScore(vecCandidates[iIndex]));
			if (fDiff < fClosest)
			{
				fClosest = fDiff;
				pRetVal = vecCandidates[iIndex];
			}
		}
	}

	return pRetVal;
}

bool CAntiAutobalance::FindCandidates()
{
	if (!AreTeamsUnbalanced())
	{
#ifdef _DEBUG
		SDK::Output("antiautobalance", "findcandidates : unbalanced teams returned false");
#endif
		Reset();
		return false;
	}

	m_vecCandidates.Purge();

	int iHeaviestTeamCount = 0;
	for (CBaseEntity* pEntity : H::Entities.GetGroup(EntityEnum::PlayerAll))
	{
		if (pEntity->m_iTeamNum() == m_iHeaviestTeam)
			iHeaviestTeamCount++;
	}
	int nMinToFind = (int)(iHeaviestTeamCount * 0.4f); // 40% of the team

	int nTotal = Max(m_nNeeded * 2, nMinToFind);
	int nNumFound = 0;

	while (nNumFound < nTotal)
	{
		CTFPlayer* pTFPlayer = FindNextCandidate();
		if (pTFPlayer)
		{
			// the best candidates are towards the tail of the list so
			// we can use for_each_vec_back to remove entries later
			int iIndex = m_vecCandidates.AddToHead();
			m_vecCandidates[iIndex].hPlayer = pTFPlayer;
			m_vecCandidates[iIndex].bSentForceMessage = false;

			nNumFound++;
			continue;
		}
#ifdef _DEBUG
		else
			SDK::Output("antiautobalance", std::format("findcandidates : no player found under findnextcandidate", nNumFound).c_str());
#endif
		break;
	}

	if (nNumFound <= 0)
	{
#ifdef _DEBUG
		SDK::Output("antiautobalance", "findcandidates : no candidates found");
#endif
		// we couldn't find anyone
		Reset();
		return false;
	}

	return true;
}

void CAntiAutobalance::PlayerChangeTeam(int iIndex)
{
#ifdef _DEBUG
	player_info_t pi{};
	if (I::EngineClient->GetPlayerInfo(iIndex, &pi))
		SDK::Output("antiautobalance", std::format("player \"{}\" is going to be autobalanced", pi.name).c_str(), Color_t(0,255,0,255), OUTPUT_CONSOLE | OUTPUT_CHAT);

	if (iIndex == I::EngineClient->GetLocalPlayer() && Vars::Misc::Automation::AntiAutobalance.Value)
		I::EngineClient->ClientCmd_Unrestricted("retry");
#endif
}

void CAntiAutobalance::ForceDeadCandidates()
{
	if(!AreTeamsUnbalanced() || !ValidateCandidates())
	{
#ifdef _DEBUG
		SDK::Output("antiautobalance", "ForceDeadCandidates : teams are balanced or candidates are invalid");
#endif
		Reset();
		return;
	}

	FOR_EACH_VEC_BACK(m_vecCandidates, i)
	{
		CTFPlayer* pTFPlayer = m_vecCandidates[i].hPlayer.Get();
		if (!pTFPlayer->IsAlive() && pTFPlayer->m_iObserverMode() > OBS_MODE_FREEZECAM)
		{
			PlayerChangeTeam(pTFPlayer->entindex());
			m_vecCandidates.Remove(i);

			if (!AreTeamsUnbalanced())
			{
#ifdef _DEBUG
				SDK::Output("antiautobalance", "ForceDeadCandidates : teams are balanced now, resetting.");
#endif
				Reset();
				break;
			}
		}
	}
}

void CAntiAutobalance::ForceCandidatesSetup()
{
	if (!AreTeamsUnbalanced() || !ValidateCandidates())
	{
		Reset();
		return;
	}

	int nNumTold = 0;
	FOR_EACH_VEC_BACK(m_vecCandidates, i)
	{
		CTFPlayer* pTFPlayer = m_vecCandidates[i].hPlayer.Get();
		if (pTFPlayer && nNumTold < m_nNeeded)
		{
			m_vecCandidates[i].bSentForceMessage = true;
			nNumTold++;
		}
	}
}

void CAntiAutobalance::ForceCandidatesExecution()
{
	if (!AreTeamsUnbalanced() || !ValidateCandidates())
	{
#ifdef _DEBUG
		SDK::Output("antiautobalance", "ForceCandidatesExecution : teams are balanced or candidates are invalid");
#endif
		Reset();
		return;
	}
	
	int nNumSwitched = 0;
	FOR_EACH_VEC_BACK(m_vecCandidates, i)
	{
		// did we warn this player?
		CTFPlayer* pTFPlayer = m_vecCandidates[i].hPlayer.Get();
		if (pTFPlayer && m_vecCandidates[i].bSentForceMessage)
		{
			// do we still need to switch players?
			if (nNumSwitched < m_nNeeded)
			{
				PlayerChangeTeam(pTFPlayer->entindex());
				nNumSwitched++;
			}
		}
	}
}
#ifdef _DEBUG
static autobalance_state_t nOldState = AB_STATE_INACTIVE;
#endif
void CAntiAutobalance::Run()
{
	//if (!Vars::Misc::Automation::AntiAutobalance.Value)
	//	return;

#ifdef _DEBUG
	if (nOldState != m_eCurrentState)
	{
		const char* sOldStateName = "";
		const char* sCurrentStateName = "";
		switch (nOldState)
		{
		case AB_STATE_INACTIVE:
			sOldStateName = "AB_STATE_INACTIVE";
			break;
		case AB_STATE_MONITOR:
			sOldStateName = "AB_STATE_MONITOR";
			break;
		case AB_STATE_FORCE_CANDIDATES_SETUP:
			sOldStateName = "AB_STATE_FORCE_CANDIDATES_SETUP";
			break;
		case AB_STATE_FORCE_DEAD_CANDIDATES:
			sOldStateName = "AB_STATE_FORCE_DEAD_CANDIDATES";
			break;
		case AB_STATE_FORCE_CANDIDATES_EXECUTION:
			sOldStateName = "AB_STATE_FORCE_CANDIDATES_EXECUTION";
			break;
		}
		switch (m_eCurrentState)
		{
		case AB_STATE_INACTIVE:
			sCurrentStateName = "AB_STATE_INACTIVE";
			break;
		case AB_STATE_MONITOR:
			sCurrentStateName = "AB_STATE_MONITOR";
			break;
		case AB_STATE_FORCE_CANDIDATES_SETUP:
			sCurrentStateName = "AB_STATE_FORCE_CANDIDATES_SETUP";
			break;
		case AB_STATE_FORCE_DEAD_CANDIDATES:
			sCurrentStateName = "AB_STATE_FORCE_DEAD_CANDIDATES";
			break;
		case AB_STATE_FORCE_CANDIDATES_EXECUTION:
			sCurrentStateName = "AB_STATE_FORCE_CANDIDATES_EXECUTION";
			break;
		}

		SDK::Output("antiautobalance", std::format("the state has changed from {} to {}", sOldStateName, sCurrentStateName).c_str(), Color_t(0, 255, 0, 255), OUTPUT_CONSOLE | OUTPUT_CHAT);
		nOldState = m_eCurrentState;
	}
#endif
	if (!ShouldBeActive())
	{
		Reset();
		return;
	}

	switch (m_eCurrentState)
	{
	case AB_STATE_INACTIVE:
		// we should be active if we've made it this far
		m_eCurrentState = AB_STATE_MONITOR;
		m_flNextStateChange = -1.f;
		break;
	case AB_STATE_MONITOR:
		if (m_flNextStateChange > 0 && m_flNextStateChange < I::GlobalVars->curtime)
		{
			if (FindCandidates())
			{
				m_eCurrentState = AB_STATE_FORCE_DEAD_CANDIDATES;
				m_flNextStateChange = -1.f;
			}
		}
		else
		{
			if (AreTeamsUnbalanced())
			{
				if (m_flNextStateChange < 0)
					m_flNextStateChange = I::GlobalVars->curtime;
			}
			else
				m_flNextStateChange = -1.f;
		}
		break;
	case AB_STATE_FORCE_DEAD_CANDIDATES:
		if (m_flNextStateChange > 0 && m_flNextStateChange < I::GlobalVars->curtime)
		{
			m_eCurrentState = AB_STATE_FORCE_CANDIDATES_SETUP;
			m_flNextStateChange = -1.f;
		}
		else
		{
			static auto tf_autobalance_dead_candidates_maxtime = U::ConVars.FindVar("tf_autobalance_dead_candidates_maxtime");
			if (m_flNextStateChange < 0 && tf_autobalance_dead_candidates_maxtime)
				m_flNextStateChange = I::GlobalVars->curtime + tf_autobalance_dead_candidates_maxtime->GetFloat();
			ForceDeadCandidates();
		}
		break;
	case AB_STATE_FORCE_CANDIDATES_SETUP:
		if (m_flNextStateChange > 0 && m_flNextStateChange < I::GlobalVars->curtime)
		{
			m_eCurrentState = AB_STATE_FORCE_CANDIDATES_EXECUTION;
			m_flNextStateChange = -1.f;
		}
		else
		{
			static auto tf_autobalance_force_candidates_maxtime = U::ConVars.FindVar("tf_autobalance_force_candidates_maxtime");
			if (m_flNextStateChange < 0 && tf_autobalance_force_candidates_maxtime)
			{
				m_flNextStateChange = I::GlobalVars->curtime + tf_autobalance_force_candidates_maxtime->GetFloat();
				ForceCandidatesSetup();
			}
		}
		break;
	case AB_STATE_FORCE_CANDIDATES_EXECUTION:
		ForceCandidatesExecution();
		Reset();
		break;
	default:
		break;
}
}

bool CAntiAutobalance::IsOkayToBalancePlayers()
{
	const auto pGameRules = I::TFGameRules();
	if (!pGameRules)
		return false;

	const void* pMatch = I::TFGCClientSystem->GetLiveMatch();
	const IMatchGroupDescription* pMatchDesc = pMatch ? pGameRules->GetMatchGroupDescription() : NULL;
	if (!pMatch || !pMatchDesc || !pMatchDesc->m_bUsesMapVoteOnRoundEnd || !pMatchDesc->m_bUseAutoBalance /*|| pMatch->m_bMatchEnded*/)
		return false;

	return true;
}
//#endif