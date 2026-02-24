#include "Commands.h"

#include "../../Core/Core.h"
#include "../ImGui/Menu/Menu.h"
#include <utility>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/join.hpp>

MAKE_SIGNATURE(CMatchInfo_GetTotalSkillRatingForTeam, "server.dll", "48 89 5C 24 ? 44 8B 49 ? 45 33 D2", 0x0);
MAKE_SIGNATURE(CMatchInfo_GetMatchDataForPlayer, "server.dll", "44 8B 49 ? 33 C0 45 85 C9 7E ? 4C 8B 51 ? 90 4D 8B 04 C2", 0x0);

static bool IsCompetitiveMode()
{
	const IMatchGroupDescription* pMatchDesc = I::TFGameRules()->GetMatchGroupDescription();
	if (pMatchDesc)
		return pMatchDesc->m_eMatchType == MATCH_TYPE_COMPETITIVE || pMatchDesc->m_eMatchType == MATCH_TYPE_CASUAL;

	return false;
}

static inline CTFLobbyPlayerProto::TF_GC_TEAM GetGCTeamForGameTeam(int nGameTeam)
{
	if (nGameTeam == TF_TEAM_BLUE)
	{
		if (IsCompetitiveMode())
			return (I::TFGameRules()->m_bTeamsSwitched()) ? CTFLobbyPlayerProto::TF_GC_TEAM::TF_GC_TEAM_DEFENDERS :
			CTFLobbyPlayerProto::TF_GC_TEAM::TF_GC_TEAM_INVADERS;

		return CTFLobbyPlayerProto::TF_GC_TEAM::TF_GC_TEAM_INVADERS;
	}
	else if (nGameTeam == TF_TEAM_RED)
	{
		if (IsCompetitiveMode())
			return (I::TFGameRules()->m_bTeamsSwitched()) ? CTFLobbyPlayerProto::TF_GC_TEAM::TF_GC_TEAM_INVADERS :
			CTFLobbyPlayerProto::TF_GC_TEAM::TF_GC_TEAM_DEFENDERS;

		return CTFLobbyPlayerProto::TF_GC_TEAM::TF_GC_TEAM_DEFENDERS;
	}

	return CTFLobbyPlayerProto::TF_GC_TEAM::TF_GC_TEAM_NOTEAM;
}

static std::unordered_map<uint32_t, CommandCallback> s_mCommands = {
	{
		FNV1A::Hash32Const("setcvar"),
		[](const std::deque<const char*>& vArgs)
		{
			if (vArgs.size() < 2)
			{
				SDK::Output("Usage:\n\tsetcvar <cvar> <value>");
				return;
			}

			const char* sCVar = vArgs[0];
			auto pCVar = I::CVar->FindVar(sCVar);
			if (!pCVar)
			{
				SDK::Output(std::format("Could not find {}", sCVar).c_str());
				return;
			}

			std::string sValue = "";
			for (int i = 1; i < vArgs.size(); i++)
				sValue += std::format("{} ", vArgs[i]);
			sValue.pop_back();
			boost::replace_all(sValue, "\"", "");

			pCVar->SetValue(sValue.c_str());
			SDK::Output(std::format("Set {} to {}", sCVar, sValue).c_str());
		}
	},
	{
		FNV1A::Hash32Const("getcvar"),
		[](const std::deque<const char*>& vArgs)
		{
			if (vArgs.size() != 1)
			{
				SDK::Output("Usage:\n\tgetcvar <cvar>");
				return;
			}

			const char* sCVar = vArgs[0];
			auto pCVar = I::CVar->FindVar(sCVar);
			if (!pCVar)
			{
				SDK::Output(std::format("Could not find {}", sCVar).c_str());
				return;
			}

			SDK::Output(std::format("Value of {} is {}", sCVar, pCVar->GetString()).c_str());
		}
	},
	{
		FNV1A::Hash32Const("queue"),
		[](const std::deque<const char*>& vArgs)
		{
			static bool bHasLoaded = false;
			if (!bHasLoaded)
			{
				I::TFPartyClient->LoadSavedCasualCriteria();
				bHasLoaded = true;
			}
			I::TFPartyClient->RequestQueueForMatch(k_eTFMatchGroup_Casual_Default);
		}
	},
	{
		FNV1A::Hash32Const("clearchat"),
		[](const std::deque<const char*>& vArgs)
		{
			I::ClientModeShared->m_pChatElement->SetText("");
		}
	},
	{
		FNV1A::Hash32Const("menu"),
		[](const std::deque<const char*>& vArgs)
		{
			I::MatSystemSurface->SetCursorAlwaysVisible(F::Menu.m_bIsOpen = !F::Menu.m_bIsOpen);
		}
	},
	{
		FNV1A::Hash32Const("unload"),
		[](const std::deque<const char*>& vArgs)
		{
			if (F::Menu.m_bIsOpen)
				I::MatSystemSurface->SetCursorAlwaysVisible(F::Menu.m_bIsOpen = false);
			U::Core.m_bUnload = true;
		}
	},
	{
		FNV1A::Hash32Const("crash"),
		[](const std::deque<const char*>& vArgs)
		{	// if you want to time out of a server and rejoin
			switch (vArgs.empty() ? 0 : FNV1A::Hash32(vArgs.front()))
			{
			case FNV1A::Hash32Const("true"):
			case FNV1A::Hash32Const("t"):
			case FNV1A::Hash32Const("1"):
				break;
			default:
				Vars::Debug::CrashLogging.Value = false; // we are voluntarily crashing, don't give out log if we don't want one
			}
			reinterpret_cast<void(*)()>(0)();
		}
	},
	{
		FNV1A::Hash32Const("playerstats"),
		[](const std::deque<const char*>& vArgs)
		{
			auto pLocal = H::Entities.GetLocal();
			if (!I::EngineClient->IsInGame() || !pLocal)
				SDK::Output("Not in game.");
			else
			{
				if (const auto pResource = H::Entities.GetResource())
				{
					SDK::Output("Player stats");

					int iIndex = pLocal->entindex();

					int iKills = pResource->m_iScore(iIndex);
					int iDeaths = pResource->m_iDeaths(iIndex);
					SDK::Output(std::format("Kills: {}", iKills).c_str());
					SDK::Output(std::format("Deaths: {}", iDeaths).c_str());
					SDK::Output(std::format("KD ratio: {}", iKills / std::max(1, iDeaths)).c_str());

					SDK::Output(std::format("Damage: {}", pResource->m_iDamage(iIndex)).c_str());
					SDK::Output(std::format("Assist damage: {}", pResource->m_iDamageAssist(iIndex)).c_str());
					SDK::Output(std::format("Healing: {}", pResource->m_iHealing(iIndex)).c_str());
				}

				if (auto pLobby = I::TFGCClientSystem->GetLobby())
				{
					int iMembers = pLobby->GetNumMembers();
					for (int i = 0; i < iMembers; i++)
					{
						ConstTFLobbyPlayer pDetails; pLobby->GetMemberDetails(&pDetails, i);
						auto pProto = pDetails.Proto();
						if (pProto)
						{
							CSteamID tSteamID = { uint32(pProto->id - 76561197960265728), k_EUniversePublic, k_EAccountTypeIndividual };
							if (!tSteamID.IsValid())
								continue;

							SDK::Output(std::format("Player {}: {}", I::SteamFriends->GetFriendPersonaName(tSteamID), pProto->normalized_rating).c_str());
						}
					}
				}
			}
		}
	},
	{
		FNV1A::Hash32Const("genhash"),
		[](const std::deque<const char*>& vArgs)
		{
			if (vArgs.size() != 1)
				SDK::Output("Input a string to hash.");
			else
			{
				std::string tText = vArgs.front();
				boost::replace_all(tText, "\"", "");
				SDK::Output(std::format("{}", FNV1A::Hash32(tText.c_str())).c_str());
			}
		}
	},
	{
		FNV1A::Hash32Const("dumpnetvars"),
		[](const std::deque<const char*>& vArgs)
		{
			switch (vArgs.empty() ? FNV1A::Hash32Const("true") : FNV1A::Hash32(vArgs.front()))
			{
			case FNV1A::Hash32Const("true"):
			case FNV1A::Hash32Const("t"):
			case FNV1A::Hash32Const("1"):
				U::NetVars.DumpTables(true);
				break;
			default:
				U::NetVars.DumpTables(false);
			}
			SDK::Output("Dumped.");
		}
	},
};

bool CCommands::Run(const char* sCmd, std::deque<const char*>& vArgs)
{
	std::string sLower = sCmd;
	std::transform(sLower.begin(), sLower.end(), sLower.begin(), ::tolower);

	auto uHash = FNV1A::Hash32(sLower.c_str());
	if (!s_mCommands.contains(uHash))
		return false;

	s_mCommands[uHash](vArgs);
	return true;
}