#pragma once
#include "../../../SDK/SDK.h"

//#define ANTIAUTOBALANCETESTING

enum autobalance_state_t
{
	AB_STATE_INACTIVE = 0,
	AB_STATE_MONITOR,
	AB_STATE_FORCE_DEAD_CANDIDATES,
	AB_STATE_FORCE_CANDIDATES_SETUP,
	AB_STATE_FORCE_CANDIDATES_EXECUTION
};

typedef struct
{
	CHandle<CTFPlayer> hPlayer;
	bool bSentForceMessage;
} candidate_info_s;

// rebuilt CTFAutobalance
class CAntiAutobalance
{
private:
	void ForceDeadCandidates();
	void ForceCandidatesSetup();
	void ForceCandidatesExecution();

	bool IsAlreadyCandidate(CTFPlayer* pTFPlayer) const;
	
	double GetTeamAutoBalanceScore(int nTeam) const;
	double GetPlayerAutoBalanceScore(CTFPlayer* pTFPlayer) const;
	CTFPlayer* FindNextCandidate();
	bool ValidateCandidates();

	bool IsOkayToBalancePlayers();
	void PlayerChangeTeam(int iIndex);

	autobalance_state_t m_eCurrentState;

	int m_iLightestTeam;
	int m_iHeaviestTeam;
	int m_nNeeded;
	float m_flNextStateChange;

	CUtlVector<candidate_info_s> m_vecCandidates;

public:
	bool ShouldBeActive() const;
	bool AreTeamsUnbalanced();
	bool FindCandidates();
	CUtlVector<candidate_info_s>* GetCandidates() { return &m_vecCandidates; }

	void Reset();

	//TODO : figure out where to call this so that it is accurate with the server.
	void Run();
};

ADD_FEATURE(CAntiAutobalance, AntiAutobalance);