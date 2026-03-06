#include "Hooks.h"
#include "Signatures.h"
#include "defs.h"

#include "../Core/Core.h"
#include "../Features/Visuals/Radar/Radar.h"
#include "../Features/Visuals/PlayerConditions/PlayerConditions.h"
#include "../Features/Visuals/SpectatorList/SpectatorList.h"
#include "../Features/Visuals/Notifications/Notifications.h"
#include "../Features/Visuals/OffscreenArrows/OffscreenArrows.h"
#include "../Features/Players/PlayerUtils.h"
#include "../Features/Ticks/Ticks.h"
#include "../Features/Resolver/Resolver.h"
#include "../Features/Backtrack/Backtrack.h"
#include "../Features/Resolver/Resolver.h"
#include "../Features/EnginePrediction/EnginePrediction.h"
#include "../Features/Spectate/Spectate.h"
#include "../Features/Commands/Commands.h"
#include "../Features/Visuals/Chams/Chams.h"
#include "../Features/Visuals/ESP/ESP.h"
#include "../Features/Visuals/Glow/Glow.h"
#include "../Features/Visuals/CameraWindow/CameraWindow.h"
#include "../Features/Visuals/Visuals.h"
#include "../Features/Visuals/Materials/Materials.h"
#include "../Features/Visuals/Groups/Groups.h"
#include "../Features/Visuals/FakeAngle/FakeAngle.h"
#include "../Features/PacketManip/PacketManip.h"
#include "../Features/Misc/Misc.h"
#include "../Features/Misc/Avoidance/Avoidance.h"
#include "../Features/NoSpread/NoSpread.h"
#include "../Features/NoSpread/NoSpreadHitscan/NoSpreadHitscan.h"
#include "../Features/Aimbot/Aimbot.h"
#include "../Features/Aimbot/AutoStickyJump/AutoStickyJump.h"
#include "../Features/Backtrack/Backtrack.h"
#include "../Features/CritHack/CritHack.h"
#include "../Features/Misc/AutoVote/AutoVote.h"
#include "../Features/Aimbot/AutoHeal/AutoHeal.h"
#include "../Features/Output/Output.h"
#include "../Features/Visuals/OffscreenArrows/OffscreenArrows.h"
#include "../Features/Simulation/MovementSimulation/MovementSimulation.h"
#include "../Features/CheaterDetection/CheaterDetection.h"
#include "../Features/Binds/Binds.h"
#include "../Features/NetworkFix/NetworkFix.h"
#include "../Features/Misc/AutoQueue/AutoQueue.h"
#include "../Features/Players/PlayerCore.h"
#include "../Features/Simulation/ProjectileSimulation/ProjectileSimulation.h"
#include "../Features/ImGui/Render.h"
#include "../Features/ImGui/Menu/Menu.h"
#include "../Features/Visuals/Spotify/Spotify.h"

//#define DEBUG_VISUALS
#ifdef DEBUG_VISUALS
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#endif
#include <unordered_set>
#include <windows.h>
#include <ImGui/imgui_impl_dx9.h>

bool __fastcall bf_read_ReadString(void* rcx, char* pStr, int maxLen, bool bLine, int* pOutNumChars)
{
	UNLOAD_RETURN(bf_read_ReadString, bool, rcx, pStr, maxLen, bLine, pOutNumChars);

	static const auto dwDesired = S::CHudVote_MsgFunc_VoteStart_ReadString_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	bool bReturn = CALL_ORIGINAL(bf_read_ReadString, bool, rcx, pStr, maxLen, bLine, pOutNumChars);

	if (dwRetAddr == dwDesired)
	{
		auto pMsg = reinterpret_cast<bf_read*>(rcx);
		const int iOriginalBit = pMsg->m_iCurBit;
		const int iTarget = pMsg->ReadByte() >> 1;
		pMsg->Seek(iOriginalBit);

		if (!iTarget)
			return bReturn;

		int iType = 0; const char* sName = F::PlayerUtils.GetPlayerName(iTarget, nullptr, &iType);
		if (iType == 0)
			return bReturn;

		int iChar = 0;
		while (1)
		{
			char val = sName[iChar];
			if (val == 0)
				break;
			else if (bLine && val == '\n')
				break;

			if (iChar < (maxLen - 1))
			{
				pStr[iChar] = val;
				++iChar;
			}
		}
		pStr[iChar] = 0;
		if (pOutNumChars)
			*pOutNumChars = iChar;
	}

	return bReturn;
}

bool __fastcall CAchievementMgr_CheckAchievementsEnabled(void* rcx)
{
	UNLOAD_RETURN(CAchievementMgr_CheckAchievementsEnabled, bool, rcx);

	return !I::EngineClient->IsPlayingDemo();
}

int __fastcall CAttributeManager_AttribHookInt(int value, const char* name, void* econent, void* buffer, bool isGlobalConstString)
{
	UNLOAD_RETURN(CAttributeManager_AttribHookInt, int, value, name, econent, buffer, isGlobalConstString);

	const auto dwRetAddr = uintptr_t(_ReturnAddress());
	const auto dwDesired = S::CTFPlayer_FireEvent_AttribHookValue_Call();

	if (!Vars::Visuals::Effects::SpellFootsteps.Value || econent != H::Entities.GetLocal() || I::EngineClient->IsTakingScreenshot() && Vars::Visuals::UI::CleanScreenshots.Value)
		return CALL_ORIGINAL(CAttributeManager_AttribHookInt, int, value, name, econent, buffer, isGlobalConstString);

	if (dwRetAddr == dwDesired && FNV1A::Hash32(name) == FNV1A::Hash32Const("halloween_footstep_type"))
	{
		switch (Vars::Visuals::Effects::SpellFootsteps.Value)
		{
		case Vars::Visuals::Effects::SpellFootstepsEnum::Color: return ColorToInt(Vars::Colors::SpellFootstep.Value);
		case Vars::Visuals::Effects::SpellFootstepsEnum::Team: return 1;
		case Vars::Visuals::Effects::SpellFootstepsEnum::Halloween: return 2;
		}
	}

	return CALL_ORIGINAL(CAttributeManager_AttribHookInt, int, value, name, econent, buffer, isGlobalConstString);
}

bool __fastcall CBaseAnimating_Interpolate(void* rcx, float currentTime)
{
	UNLOAD_RETURN(CBaseAnimating_Interpolate, bool, rcx, currentTime);

	if (rcx == H::Entities.GetLocal() ? F::Ticks.m_bRecharge : Vars::Visuals::Removals::Interpolation.Value)
		return true;

	// fixes weird stupid flickering with the resolver, but completely disables interpolation
	// if anyone knows of a better solution let me know. doesn't seem to cause nonvisual issues so i'm leaving this commented out for now
	//if (F::Resolver.GetAngles(reinterpret_cast<CTFPlayer*>(rcx)))
	//	return true;

	return CALL_ORIGINAL(CBaseAnimating_Interpolate, bool, rcx, currentTime);
}

void __fastcall CBaseAnimating_MaintainSequenceTransitions(void* rcx, void* boneSetup, float flCycle, Vec3 pos[], Vector4D q[])
{
	UNLOAD_RETURN(CBaseAnimating_MaintainSequenceTransitions, void, rcx, boneSetup, flCycle, pos, q);

	return;
}

void __fastcall CBaseAnimating_SetSequence(void* rcx, int nSequence)
{
	UNLOAD_RETURN(CBaseAnimating_SetSequence, void, rcx, nSequence);

	auto pEntity = reinterpret_cast<CBaseAnimating*>(rcx);
	if (pEntity->m_nSequence() != nSequence && !pEntity->m_bSequenceLoops())
		pEntity->m_flCycle() = 0.f; // set on the server but not client

	CALL_ORIGINAL(CBaseAnimating_SetSequence, void, rcx, nSequence);
}

bool __fastcall CBaseAnimating_SetupBones(void* rcx, matrix3x4* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
{
	UNLOAD_RETURN(CBaseAnimating_SetupBones, bool, rcx, pBoneToWorldOut, nMaxBones, boneMask, currentTime);

	if (!Vars::Misc::Game::SetupBonesOptimization.Value || F::Backtrack.IsSettingUpBones())
		return CALL_ORIGINAL(CBaseAnimating_SetupBones, bool, rcx, pBoneToWorldOut, nMaxBones, boneMask, currentTime);

	auto pAnimating = reinterpret_cast<CBaseEntity*>(uintptr_t(rcx) - 8);
	if (!pAnimating)
		return CALL_ORIGINAL(CBaseAnimating_SetupBones, bool, rcx, pBoneToWorldOut, nMaxBones, boneMask, currentTime);

	auto pOwner = pAnimating->GetRootMoveParent();
	auto pEntity = pOwner ? pOwner : pAnimating;
	if (!pEntity->IsPlayer() || pEntity->entindex() == I::EngineClient->GetLocalPlayer())
		return CALL_ORIGINAL(CBaseAnimating_SetupBones, bool, rcx, pBoneToWorldOut, nMaxBones, boneMask, currentTime);

	if (pBoneToWorldOut)
	{
		auto& aBones = pEntity->As<CBaseAnimating>()->m_CachedBoneData();
		if (nMaxBones >= aBones.Count())
			memcpy(pBoneToWorldOut, aBones.Base(), sizeof(matrix3x4) * aBones.Count());
		else
			return false;
	}

	return true;
}

void __fastcall CBaseAnimating_UpdateClientSideAnimation(void* rcx)
{
	UNLOAD_RETURN(CBaseAnimating_UpdateClientSideAnimation, void, rcx);

	auto pLocal = H::Entities.GetLocal();
	auto pPlayer = reinterpret_cast<CTFPlayer*>(rcx);
	if ((Vars::Visuals::Removals::Interpolation.Value || F::Resolver.GetAngles(pPlayer)) && !G::UpdatingAnims
		|| pPlayer == pLocal && !pLocal->InCond(TF_COND_HALLOWEEN_KART) && !I::EngineClient->IsPlayingDemo())
		return;

	CALL_ORIGINAL(CBaseAnimating_UpdateClientSideAnimation, void, rcx);
}

void __fastcall CBaseEntity_AddVar(void* rcx, void* data, IInterpolatedVar* watcher, int type, bool bSetup)
{
	UNLOAD_RETURN(CBaseEntity_AddVar, void, rcx, data, watcher, type, bSetup);

	if (Vars::Misc::Game::AccuracyImprovements.Value && watcher)
	{
		uint32_t uHash = FNV1A::Hash32(watcher->GetDebugName());
		if (uHash == FNV1A::Hash32Const("C_BaseEntity::m_iv_vecVelocity") ||
			uHash == FNV1A::Hash32Const("C_BaseAnimating::m_iv_flPoseParameter") ||
			uHash == FNV1A::Hash32Const("C_BaseAnimating::m_iv_flCycle") ||
			uHash == FNV1A::Hash32Const("CMultiPlayerAnimState::m_iv_flMaxGroundSpeed"))
			return;

		if (rcx != H::Entities.GetLocal() &&
			uHash == FNV1A::Hash32Const("C_TFPlayer::m_iv_angEyeAngles"))
			return;
	}

	CALL_ORIGINAL(CBaseEntity_AddVar, void, rcx, data, watcher, type, bSetup);
}

int __fastcall CBaseEntity_BaseInterpolatePart1(void* rcx, float& currentTime, Vector& oldOrigin, QAngle& oldAngles, Vector& oldVel, int& bNoMoreChanges)
{
	UNLOAD_RETURN(CBaseEntity_BaseInterpolatePart1, int, rcx, std::ref(currentTime), std::ref(oldOrigin), std::ref(oldAngles), std::ref(oldVel), std::ref(bNoMoreChanges));

	auto pEntity = reinterpret_cast<CBaseEntity*>(rcx);
	if (pEntity && pEntity->GetClassID() == ETFClassID::CTFViewModel && F::Ticks.m_bRecharge)
	{
		bNoMoreChanges = 1;
		return 0;
	}

	return CALL_ORIGINAL(CBaseEntity_BaseInterpolatePart1, int, rcx, std::ref(currentTime), std::ref(oldOrigin), std::ref(oldAngles), std::ref(oldVel), std::ref(bNoMoreChanges));
}

void __fastcall CBaseEntity_EstimateAbsVelocity(void* rcx, Vector& vel)
{
	UNLOAD_RETURN(CBaseEntity_EstimateAbsVelocity, void, rcx, std::ref(vel));

	auto pPlayer = reinterpret_cast<CTFPlayer*>(rcx);
	if (!pPlayer->IsPlayer())
		return CALL_ORIGINAL(CBaseEntity_EstimateAbsVelocity, void, rcx, std::ref(vel));

	if (pPlayer->entindex() == I::EngineClient->GetLocalPlayer())
	{
		vel = pPlayer->m_vecVelocity();
		return;
	}

	if (!Vars::Visuals::Removals::Interpolation.Value)
	{
		CALL_ORIGINAL(CBaseEntity_EstimateAbsVelocity, void, rcx, std::ref(vel));
		vel.z = pPlayer->m_vecVelocity().z;
	}
	else
		vel = pPlayer->m_vecVelocity();

	if (pPlayer->IsOnGround() && vel.Length2DSqr() < 2.f)
	{
		bool bMinwalk;
		if (F::Resolver.GetAngles(pPlayer, nullptr, nullptr, &bMinwalk) && bMinwalk)
			vel = { 1, 1 };
	}
}

void __fastcall CBaseEntity_InterpolateServerEntities(void* rcx)
{
	UNLOAD_RETURN(CBaseEntity_InterpolateServerEntities, void, rcx);

	if (Vars::Misc::Game::AccuracyImprovements.Value)
	{
		static auto cl_extrapolate = U::ConVars.FindVar("cl_extrapolate");
		if (cl_extrapolate && cl_extrapolate->GetInt())
			cl_extrapolate->SetValue(0);
	}

	CALL_ORIGINAL(CBaseEntity_InterpolateServerEntities, void, rcx);
}

void __fastcall CBaseEntity_ResetLatched(void* rcx)
{
	UNLOAD_RETURN(CBaseEntity_ResetLatched, void, rcx);

	if (rcx == H::Entities.GetLocal())
		return;

	CALL_ORIGINAL(CBaseEntity_ResetLatched, void, rcx);
}

void __fastcall CBaseEntity_SetAbsVelocity(void* rcx, const Vec3& vecAbsVelocity)
{
	UNLOAD_RETURN(CBaseEntity_SetAbsVelocity, void, rcx, std::ref(vecAbsVelocity));

	const auto dwRetAddr = uintptr_t(_ReturnAddress());
	const auto dwDesired = S::CBasePlayer_PostDataUpdate_SetAbsVelocity_Call();

	if (dwRetAddr != dwDesired)
		return CALL_ORIGINAL(CBaseEntity_SetAbsVelocity, void, rcx, std::ref(vecAbsVelocity));

	const auto pPlayer = reinterpret_cast<CTFPlayer*>(rcx);
	if (pPlayer->IsDormant())
		return CALL_ORIGINAL(CBaseEntity_SetAbsVelocity, void, rcx, std::ref(vecAbsVelocity));

	auto pRecords = H::Entities.GetOrigins(pPlayer->entindex());
	if (!pRecords || pRecords->empty())
		return CALL_ORIGINAL(CBaseEntity_SetAbsVelocity, void, rcx, std::ref(vecAbsVelocity));

	auto& tOldRecord = pRecords->front();
	auto tNewRecord = VelFixRecord(pPlayer->m_vecOrigin() + Vec3(0, 0, pPlayer->GetSize().z), pPlayer->m_flSimulationTime());

	int iDeltaTicks = TIME_TO_TICKS(tNewRecord.m_flSimulationTime - tOldRecord.m_flSimulationTime);
	float flDeltaTime = TICKS_TO_TIME(iDeltaTicks);
	if (iDeltaTicks <= 0)
		return;

	static auto sv_lagcompensation_teleport_dist = U::ConVars.FindVar("sv_lagcompensation_teleport_dist");
	float flDist = powf(sv_lagcompensation_teleport_dist->GetFloat(), 2.f) * iDeltaTicks;
	if ((tNewRecord.m_vecOrigin - tOldRecord.m_vecOrigin).Length2DSqr() >= flDist)
		return pRecords->clear();

	bool bGrounded = pPlayer->IsOnGround();

	AxisInfo tAxisInfo = {};
	for (int i = 0; i < 3; i++)
	{
		tAxisInfo[i].m_flOldAxisValue = tOldRecord.m_vecOrigin[i];
		tAxisInfo[i].m_flNewAxisValue = tNewRecord.m_vecOrigin[i];
		tAxisInfo[i].m_flOldSimulationTime = tOldRecord.m_flSimulationTime;
		tAxisInfo[i].m_flNewSimulationTime = tNewRecord.m_flSimulationTime;

		if (i == 2 && bGrounded)
			break;

		float flOldPos1 = tOldRecord.m_vecOrigin[i], flOldPos2 = flOldPos1 + PLAYER_ORIGIN_COMPRESSION * sign(flOldPos1);
		float flNewPos1 = tNewRecord.m_vecOrigin[i], flNewPos2 = flNewPos1 + PLAYER_ORIGIN_COMPRESSION * sign(flNewPos1);
		if (!flOldPos1) flOldPos1 = -PLAYER_ORIGIN_COMPRESSION, flOldPos2 = PLAYER_ORIGIN_COMPRESSION;
		if (!flNewPos1) flNewPos1 = -PLAYER_ORIGIN_COMPRESSION, flNewPos2 = PLAYER_ORIGIN_COMPRESSION;

		FloatRange_t flVelocityRange;
		{
			std::deque<float> vDeltas = { flNewPos1 - flOldPos1, flNewPos2 - flOldPos1, flNewPos1 - flOldPos2, flNewPos2 - flOldPos2 };
			std::sort(vDeltas.begin(), vDeltas.end(), std::less<float>());
			flVelocityRange = { vDeltas.front() / flDeltaTime, vDeltas.back() / flDeltaTime };
		}

		for (auto& tRecord : *pRecords)
		{
			if (tAxisInfo[i].m_flOldSimulationTime <= tRecord.m_flSimulationTime)
				continue;

			float flRewind = -ROUND_TO_TICKS(tNewRecord.m_flSimulationTime - tRecord.m_flSimulationTime);
			FloatRange_t flPositionRange = { tAxisInfo[i].m_flNewAxisValue + flVelocityRange.Max * flRewind, tAxisInfo[i].m_flNewAxisValue + flVelocityRange.Min * flRewind };
			if (i == 2)
			{
				static auto sv_gravity = U::ConVars.FindVar("sv_gravity");
				float flGravityCorrection = sv_gravity->GetFloat() * powf(flRewind + TICK_INTERVAL / 2, 2.f) / 2;
				flPositionRange.Min -= flGravityCorrection, flPositionRange.Max -= flGravityCorrection;
			}
			if (flPositionRange.Min > tRecord.m_vecOrigin[i] || tRecord.m_vecOrigin[i] > flPositionRange.Max)
				break;

			tAxisInfo[i].m_flOldAxisValue = tRecord.m_vecOrigin[i];
			tAxisInfo[i].m_flOldSimulationTime = tRecord.m_flSimulationTime;
		}
	}

	H::Entities.SetAvgVelocity(pPlayer->entindex(), tAxisInfo.Get(bGrounded));
	CALL_ORIGINAL(CBaseEntity_SetAbsVelocity, void, rcx, (tNewRecord.m_vecOrigin - tOldRecord.m_vecOrigin) / flDeltaTime);
}

const Vec3* __fastcall CBaseEntity_WorldSpaceCenter(void* rcx)
{
	UNLOAD_RETURN(CBaseEntity_WorldSpaceCenter, const Vec3*, rcx);

	const auto dwRetAddr = uintptr_t(_ReturnAddress());
	const auto dwDesired = S::CDamageAccountPanel_DisplayDamageFeedback_WorldSpaceCenter_Call();

	return dwRetAddr == dwDesired && Vars::Visuals::Effects::DrawDamageNumbersThroughWalls.Value ? S::MainViewOrigin.Call<const Vec3*>() : CALL_ORIGINAL(CBaseEntity_WorldSpaceCenter, const Vec3*, rcx);
}

/*
void __fastcall CBaseHudChat_StartMessageMode(void* rcx, int iMessageModeType)
{
	UNLOAD_RETURN(CBaseHudChat_StartMessageMode, void, rcx, iMessageModeType);

	CALL_ORIGINAL(CBaseHudChat_StartMessageMode, void, rcx, iMessageModeType);

	if (iMessageModeType && SDK::StdRandomInt(0, 69) < 5)
	{
		// this has to crash at some point right?
		void* chatInput = *(void**)((uintptr_t)rcx + 0x298);
		void* textPrompt = *(void**)((uintptr_t)chatInput + 0x1F0);
		const void* chatInputVTable = *(void**)textPrompt;
		const uintptr_t setPromptFunc = *(uintptr_t*)((uintptr_t)chatInputVTable + 0x6A8);

		const wchar_t* pszPrompt;
		switch (iMessageModeType)
		{
		case MM_SAY_TEAM:
			pszPrompt = L"A retard would say this in team chat : ";
			break;
		case MM_SAY_PARTY:
			pszPrompt = L"A retard would say this in party chat : ";
			break;
		default:
			pszPrompt = L"A retard would say : ";
		}
		using FnSetPrompt = void(__fastcall*)(void* rcx, const wchar_t* prompt, uint64_t balls);
		reinterpret_cast<FnSetPrompt>(setPromptFunc)((void*)textPrompt, pszPrompt, 0);
		reinterpret_cast<CBaseHudChatInputLine*>(chatInput)->InvalidateLayout(false, false);
	}
}
*/

void __fastcall CBaseHudChatLine_InsertAndColorizeText(void* rcx, wchar_t* buf, int clientIndex)
{
	UNLOAD_RETURN(CBaseHudChatLine_InsertAndColorizeText, void, rcx, buf, clientIndex);

	std::string sMessage = SDK::ConvertWideToUTF8(buf);

	if (clientIndex)
	{
		player_info_t pi{};
		if (!I::EngineClient->GetPlayerInfo(clientIndex, &pi))
			return CALL_ORIGINAL(CBaseHudChatLine_InsertAndColorizeText, void, rcx, buf, clientIndex);

		const char* sName = pi.name;
		auto iFind = sMessage.find(sName);

		int iType = 0;
		if (const char* sReplace = F::PlayerUtils.GetPlayerName(clientIndex, nullptr, &iType))
		{
			if (iFind != std::string::npos)
				sMessage = sMessage.replace(std::max(int(iFind) - 1, 0), strlen(sName) + 1, std::format("\x3{}\x1", sReplace));
			sName = sReplace;
		}

		if (Vars::Visuals::UI::ChatTags.Value && iType != 1)
		{
			std::string sTag, cColor;
			if (Vars::Visuals::UI::ChatTags.Value & Vars::Visuals::UI::ChatTagsEnum::Local && clientIndex == I::EngineClient->GetLocalPlayer())
				sTag = "You", cColor = Vars::Colors::Local.Value.ToHexA();
			else if (Vars::Visuals::UI::ChatTags.Value & Vars::Visuals::UI::ChatTagsEnum::Friends && H::Entities.IsFriend(clientIndex))
				sTag = "Friend", cColor = F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(FRIEND_TAG)].m_tColor.ToHexA();
			else if (Vars::Visuals::UI::ChatTags.Value & Vars::Visuals::UI::ChatTagsEnum::Party && H::Entities.InParty(clientIndex))
				sTag = "Party", cColor = F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(PARTY_TAG)].m_tColor.ToHexA();
			else if (Vars::Visuals::UI::ChatTags.Value & Vars::Visuals::UI::ChatTagsEnum::Assigned)
			{
				if (auto pTag = F::PlayerUtils.GetSignificantTag(clientIndex, 0))
					sTag = pTag->m_sName, cColor = pTag->m_tColor.ToHexA();
			}

			if (!sTag.empty())
			{
				if (iFind != std::string::npos)
					sMessage = sMessage.replace(std::max(int(iFind) - 1, 0), strlen(sName) + 1, std::format("\x3{}\x1", sName));
				sMessage.insert(0, std::format("{}[{}] \x3", cColor, sTag));
			}
		}
	}

	if (Vars::Visuals::UI::StreamerMode.Value)
	{
		if (auto pResource = H::Entities.GetResource())
		{
			std::vector<std::pair<std::string, std::string>> vReplace;
			for (auto& pEntity : H::Entities.GetGroup(EntityEnum::PlayerAll))
			{
				int iIndex = pEntity->entindex();
				int iType = 0; const char* sReplace = F::PlayerUtils.GetPlayerName(iIndex, nullptr, &iType);
				if (sReplace && iType == 1)
					vReplace.emplace_back(pResource->GetName(iIndex), sReplace);
			}
			for (auto& [sFind, sReplace] : vReplace)
			{
				{
					std::string sReplace2 = sReplace;
					std::transform(sFind.begin(), sFind.end(), sFind.begin(), ::tolower);
					std::transform(sReplace2.begin(), sReplace2.end(), sReplace2.begin(), ::tolower);
					if (FNV1A::Hash32(sFind.c_str()) == FNV1A::Hash32(sReplace2.c_str()))
						continue;
				}

				size_t iPos = 0;
				while (true)
				{
					std::string sMessage2 = sMessage;
					std::transform(sMessage2.begin(), sMessage2.end(), sMessage2.begin(), ::tolower);

					auto iFind = sMessage2.find(sFind, iPos);
					if (iFind == std::string::npos)
						break;

					iPos = iFind + sReplace.length();
					sMessage = sMessage.replace(iFind, sFind.length(), sReplace);
				}
			}
		}
	}

	CALL_ORIGINAL(CBaseHudChatLine_InsertAndColorizeText, void, rcx, const_cast<wchar_t*>(SDK::ConvertUtf8ToWide(sMessage).c_str()), clientIndex);
}

void __fastcall CBasePlayer_CalcObserverView(void* rcx, Vector& eyeOrigin, QAngle& eyeAngles, float& fov)
{
	UNLOAD_RETURN(CBasePlayer_CalcObserverView, void, rcx, std::ref(eyeOrigin), std::ref(eyeAngles), std::ref(fov));

	if (!F::Spectate.HasTarget())
		return CALL_ORIGINAL(CBasePlayer_CalcObserverView, void, rcx, std::ref(eyeOrigin), std::ref(eyeAngles), std::ref(fov));

	auto pPlayer = reinterpret_cast<CBasePlayer*>(rcx);
	auto pTarget = pPlayer->m_hObserverTarget()->As<CTFPlayer>();
	if (!pTarget || !pTarget->IsPlayer())
		return CALL_ORIGINAL(CBasePlayer_CalcObserverView, void, rcx, std::ref(eyeOrigin), std::ref(eyeAngles), std::ref(fov));

	Vec3 vOldOffset = pPlayer->m_vecViewOffset();
	pPlayer->m_vecViewOffset() = pTarget->GetViewOffset();
	CALL_ORIGINAL(CBasePlayer_CalcObserverView, void, rcx, std::ref(eyeOrigin), std::ref(eyeAngles), std::ref(fov));
	pPlayer->m_vecViewOffset() = vOldOffset;
}

void __fastcall CBasePlayer_CalcView(void* rcx, Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov)
{
	UNLOAD_RETURN(CBasePlayer_CalcView, void, rcx, std::ref(eyeOrigin), std::ref(eyeAngles), std::ref(zNear), std::ref(zFar), std::ref(fov));

	if (!Vars::Visuals::Removals::ViewPunch.Value && !F::Spectate.HasTarget())
		return CALL_ORIGINAL(CBasePlayer_CalcView, void, rcx, std::ref(eyeOrigin), std::ref(eyeAngles), std::ref(zNear), std::ref(zFar), std::ref(fov));

	auto pPlayer = reinterpret_cast<CBasePlayer*>(rcx);

	Vec3 vOriginalPunch = pPlayer->m_vecPunchAngle();
	pPlayer->m_vecPunchAngle() = {};
	CALL_ORIGINAL(CBasePlayer_CalcView, void, rcx, std::ref(eyeOrigin), std::ref(eyeAngles), std::ref(zNear), std::ref(zFar), std::ref(fov));
	pPlayer->m_vecPunchAngle() = vOriginalPunch;
}

void __fastcall CTFPlayer_HandleTaunting(void* rcx)
{
	UNLOAD_RETURN(CTFPlayer_HandleTaunting, void, rcx);

	if (!F::Spectate.HasTarget())
		return CALL_ORIGINAL(CTFPlayer_HandleTaunting, void, rcx);

	I::ThirdPersonManager->m_bOverrideThirdPerson = false;
}

Vector __fastcall CThirdPersonManager_GetFinalCameraOffset(void* rcx)
{
	UNLOAD_RETURN(CThirdPersonManager_GetFinalCameraOffset, Vector, rcx);

	if (!F::Spectate.HasTarget())
		return CALL_ORIGINAL(CThirdPersonManager_GetFinalCameraOffset, Vector, rcx);

	float flOriginalUpOffset = I::ThirdPersonManager->m_flUpOffset;
	I::ThirdPersonManager->m_flUpOffset = 0.f;
	Vec3 vReturn = CALL_ORIGINAL(CThirdPersonManager_GetFinalCameraOffset, Vector, rcx);
	I::ThirdPersonManager->m_flUpOffset = flOriginalUpOffset;
	return vReturn;
}

void __fastcall CBaseViewModel_CalcViewModelView(void* rcx, CBasePlayer* owner, /*const*/ Vector& eyePosition, /*const*/ QAngle& eyeAngles)
{
	UNLOAD_RETURN(CBaseViewModel_CalcViewModelView, void, rcx, owner, std::ref(eyePosition), std::ref(eyeAngles));

	Vec3 vOffset = { Vars::Visuals::Viewmodel::OffsetX.Value, Vars::Visuals::Viewmodel::OffsetY.Value, Vars::Visuals::Viewmodel::OffsetZ.Value };
	Vec3 vAngles = { Vars::Visuals::Viewmodel::Pitch.Value, Vars::Visuals::Viewmodel::Yaw.Value, Vars::Visuals::Viewmodel::Roll.Value };
	if (!Vars::Visuals::Viewmodel::ViewmodelAim.Value && vOffset.IsZero() && vAngles.IsZero() || Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return CALL_ORIGINAL(CBaseViewModel_CalcViewModelView, void, rcx, owner, std::ref(eyePosition), std::ref(eyeAngles));

	bool bFlip = G::FlipViewmodels;

	if (Vars::Visuals::Viewmodel::ViewmodelAim.Value)
	{
		if (auto pLocal = H::Entities.GetLocal(); pLocal && pLocal->IsAlive() && G::AimPoint.m_iTickCount)
		{
			Vec3 vDiff = I::EngineClient->GetViewAngles() - Math::CalcAngle(eyePosition, G::AimPoint.m_vOrigin);
			if (bFlip)
				vDiff.y *= -1;
			eyeAngles = I::EngineClient->GetViewAngles() - vDiff;
		}
	}

	if (!vOffset.IsZero())
	{
		Vec3 vForward, vRight, vUp; Math::AngleVectors(eyeAngles, &vForward, &vRight, &vUp);
		eyePosition += vForward * vOffset.y;
		eyePosition += vRight * vOffset.x * (bFlip ? -1 : 1);
		eyePosition += vUp * vOffset.z;
	}
	if (vAngles.x)
		eyeAngles.x += vAngles.x;
	if (vAngles.y)
		eyeAngles.y += vAngles.y * (bFlip ? -1 : 1);
	if (vAngles.z)
		eyeAngles.z += vAngles.z * (bFlip ? -1 : 1);

	CALL_ORIGINAL(CBaseViewModel_CalcViewModelView, void, rcx, owner, std::ref(eyePosition), std::ref(eyeAngles));
}

void __fastcall CBasePlayer_CalcViewModelView(void* rcx, /*const*/ Vector& eyeOrigin, /*const*/ QAngle& eyeAngles)
{
	UNLOAD_RETURN(CBasePlayer_CalcViewModelView, void, rcx, std::ref(eyeOrigin), std::ref(eyeAngles));

	Vector vOldEyeOrigin = eyeOrigin, vOldEyeAngles = eyeAngles;
	CALL_ORIGINAL(CBasePlayer_CalcViewModelView, void, rcx, std::ref(eyeOrigin), std::ref(eyeAngles));
	eyeOrigin = vOldEyeOrigin, eyeAngles = vOldEyeAngles;
}

void __fastcall CBasePlayer_ItemPostFrame(void* rcx)
{
	UNLOAD_RETURN(CBasePlayer_ItemPostFrame, void, rcx);

	auto pLocal = reinterpret_cast<CTFPlayer*>(rcx);
	auto pWeapon = H::Entities.GetWeapon();
	if (!pWeapon)
		return CALL_ORIGINAL(CBasePlayer_ItemPostFrame, void, rcx);

	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_ROCKETLAUNCHER:
	case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
	case TF_WEAPON_PARTICLE_CANNON:
		if (!pWeapon->IsInReload())
			return CALL_ORIGINAL(CBasePlayer_ItemPostFrame, void, rcx);
		break;
	default:
		return CALL_ORIGINAL(CBasePlayer_ItemPostFrame, void, rcx);
	}

	if (pWeapon->m_iItemDefinitionIndex() == Soldier_m_TheBeggarsBazooka)
	{
		if (I::GlobalVars->curtime < pLocal->m_flNextAttack())
			return CALL_ORIGINAL(CBasePlayer_ItemPostFrame, void, rcx);
	}
	else
	{
		// not perfect but seems to work fine enough for casual use
		auto pViewmodel = pLocal->m_hViewModel()->As<CBaseAnimating>();
		if (!pViewmodel)
			return CALL_ORIGINAL(CBasePlayer_ItemPostFrame, void, rcx);

		auto pStudio = pViewmodel->GetModelPtr();
		if (!pStudio)
			return CALL_ORIGINAL(CBasePlayer_ItemPostFrame, void, rcx);

		float flReloadTime = pViewmodel->SequenceDuration();
		float flReloadSpeed = 1.f / pViewmodel->m_flPlaybackRate();

		float flLastCycle = (I::GlobalVars->curtime - pWeapon->m_flReloadPriorNextFire() - TICK_INTERVAL) / (flReloadTime * flReloadSpeed);
		float flCurrCycle = (I::GlobalVars->curtime - pWeapon->m_flReloadPriorNextFire()) / (flReloadTime * flReloadSpeed);

		animevent_t event; int index = 0;
		index = S::GetAnimationEvent.Call<int>(pStudio, pViewmodel->m_nSequence(), &event, flLastCycle, flCurrCycle, index);
		if (!index || event.event != AE_WPN_INCREMENTAMMO)
			return CALL_ORIGINAL(CBasePlayer_ItemPostFrame, void, rcx);
	}

	CALL_ORIGINAL(CBasePlayer_ItemPostFrame, void, rcx);
	pWeapon->IncrementAmmo();
	pWeapon->m_bReloadedThroughAnimEvent() = true;
}

bool __fastcall CBaseViewModel_ShouldFlipViewModel(void* rcx)
{
	UNLOAD_RETURN(CBaseViewModel_ShouldFlipViewModel, bool, rcx);

	return G::FlipViewmodels = CALL_ORIGINAL(CBaseViewModel_ShouldFlipViewModel, bool, rcx);
}

void __fastcall Cbuf_ExecuteCommand(CCommand& args, cmd_source_t source)
{
	UNLOAD_RETURN(Cbuf_ExecuteCommand, void, std::ref(args), source);

	if (args.ArgC())
	{
		const char* sCommand = args[0];
		std::deque<const char*> vArgs;
		for (int i = 1; i < args.ArgC(); i++)
			vArgs.push_back(args[i]);

		if (F::Commands.Run(sCommand, vArgs))
			return;

		switch (FNV1A::Hash32(sCommand))
		{
		case FNV1A::Hash32Const("say"):
		case FNV1A::Hash32Const("say_team"):
		{
			s_sCmdString = args.m_pArgSBuffer;
			s_sCmdString = s_sCmdString.replace(0, args.m_nArgv0Size, "");

			for (auto& [sFind, sReplace] : s_vStatic)
			{
				size_t iPos = 0;
				while (true)
				{
					auto iFind = s_sCmdString.find(sFind, iPos);
					if (iFind == std::string::npos)
						break;

					iPos = iFind + sReplace.length();
					s_sCmdString = s_sCmdString.replace(iFind, sFind.length(), sReplace);
				}
			}
			for (auto& fFunction : s_vDynamic)
				fFunction();

			s_sCmdString = std::format("{} {}", sCommand, s_sCmdString).substr(0, COMMAND_MAX_LENGTH - 1);
			strncpy_s(args.m_pArgSBuffer, s_sCmdString.c_str(), COMMAND_MAX_LENGTH);
			args.m_nArgv0Size = int(strlen(sCommand)) + 1;

			break;
		}
		case FNV1A::Hash32Const("cl_flipviewmodels"):
		{	// server does string comparison to "1"
			auto pCVar = I::CVar->FindVar(sCommand);
			if (!pCVar)
				break;

			CALL_ORIGINAL(Cbuf_ExecuteCommand, void, std::ref(args), source);

			try
			{
				if (pCVar->GetFloat() != float(std::stoi(pCVar->GetString())))
					break;

				auto sValue = std::format("{}", pCVar->GetInt());
				if (FNV1A::Hash32(sValue.c_str()) == FNV1A::Hash32(pCVar->GetString()))
					break;

				pCVar->SetValue(sValue.c_str());
			}
			catch (...) {}

			return;
		}
		}
	}

	CALL_ORIGINAL(Cbuf_ExecuteCommand, void, std::ref(args), source);
}

bool __fastcall CClientModeShared_DoPostScreenSpaceEffects(void* rcx, const CViewSetup* pSetup)
{
	UNLOAD_RETURN(CClientModeShared_DoPostScreenSpaceEffects, bool, rcx, pSetup);

	if (Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return CALL_ORIGINAL(CClientModeShared_DoPostScreenSpaceEffects, bool, rcx, pSetup);

	auto pLocal = H::Entities.GetLocal();
	auto pWeapon = H::Entities.GetWeapon();
	if (pLocal)
	{
		if (pWeapon)
		{
			F::Visuals.SplashRadius(pLocal);
			F::Visuals.ProjectileTrace(pLocal, pWeapon);
		}
		F::ESP.DrawSoundESP();
	}

	if (!F::CameraWindow.m_bDrawing)
	{
		F::Visuals.DrawEffects();
		F::Chams.m_mEntities.clear();
		if (!I::EngineVGui->IsGameUIVisible() && F::Materials.m_bLoaded)
		{
			F::Chams.RenderMain();
			F::Glow.RenderMain();
		}
	}

	return CALL_ORIGINAL(CClientModeShared_DoPostScreenSpaceEffects, bool, rcx, pSetup);
}

void __fastcall CClientModeShared_OverrideView(void* rcx, CViewSetup* pView)
{
	CALL_ORIGINAL(CClientModeShared_OverrideView, void, rcx, pView);
	if (Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return;

	auto pLocal = H::Entities.GetLocal();
	if (pLocal && pView)
	{
		F::Visuals.FOV(pLocal, pView);
		F::Visuals.ThirdPerson(pLocal, pView);
	}
}

bool __fastcall CClientModeShared_ShouldDrawViewModel(void* rcx)
{
	UNLOAD_RETURN(CClientModeShared_ShouldDrawViewModel, bool, rcx);

	if (Vars::Visuals::UI::ZoomFieldOfView.Value)
	{
		auto pLocal = H::Entities.GetLocal();
		if (pLocal && pLocal->InCond(TF_COND_ZOOMED))
			return true;
	}

	return CALL_ORIGINAL(CClientModeShared_ShouldDrawViewModel, bool, rcx);
}

float __fastcall CClientState_GetClientInterpAmount(void* rcx)
{
	UNLOAD_RETURN(CClientState_GetClientInterpAmount, float, rcx);

	G::Lerp = CALL_ORIGINAL(CClientState_GetClientInterpAmount, float, rcx);
	return 0.f;
}

bool __fastcall CClientState_ProcessFixAngle(void* rcx, SVC_FixAngle* msg)
{
	UNLOAD_RETURN(CClientState_ProcessFixAngle, bool, rcx, msg);

	if (Vars::Visuals::Removals::AngleForcing.Value)
		return false;

	if (F::Spectate.HasTarget())
		F::Spectate.m_vOldView = msg->m_Angle;
	return CALL_ORIGINAL(CClientState_ProcessFixAngle, bool, rcx, msg);
}

std::unordered_map<int, CGlowObject*> mGlowObjects;

void __fastcall CGlowObjectManager_RenderGlowEffects(CGlowObjectManager* rcx, const CViewSetup* pSetup, int nSplitScreenSlot)
{
	if (!Vars::Colors::ClassicGlow.Value || G::Unload || !F::Groups.GroupsActive() ||
		Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
	{
		for (auto& [pEntity, pGroup] : F::Groups.GetGroup())
		{
			if (!pEntity)
				continue;
			if (pEntity->IsBaseCombatCharacter())
				pEntity->As<CBaseCombatCharacter>()->m_bGlowEnabled() = false;
			else if (pEntity->GetClassID() == ETFClassID::CCaptureFlag)
				pEntity->As<CCaptureFlag>()->m_bGlowEnabled() = false;
		}

		for (auto& [pEntity, pGlow] : mGlowObjects)
		{
			delete pGlow;
			pGlow = nullptr;
		}

		mGlowObjects.clear();

		return CALL_ORIGINAL(CGlowObjectManager_RenderGlowEffects, void, rcx, pSetup, nSplitScreenSlot);
	}

	std::unordered_set<int> activeIndices;

	for (auto& [pEntity, pGroup] : F::Groups.GetGroup())
	{
		if (!pEntity || !pGroup)
			continue;

		const int iIndex = pEntity->entindex();
		if (iIndex <= 0 || !I::ClientEntityList->GetClientEntity(iIndex))
			continue;

		activeIndices.insert(iIndex);

		if (pGroup->m_tGlow())
		{
			Color_t tColor;

			EHANDLE pOwner = pEntity->IsBuilding()
				? pEntity->As<CBaseObject>()->m_hBuilder()
				: pEntity->m_hOwnerEntity();
			if (pGroup->m_bUseHealthGlow &&
				(pOwner ? (pOwner->IsBuilding() || pOwner->IsPlayer()) : (pEntity->IsBuilding() || pEntity->IsPlayer())))
				tColor = GetHealthColor(pOwner ? pOwner.Get() : pEntity, pGroup, true);
			else
				tColor = F::Groups.GetColor(pEntity, pGroup);

			const Vec3 vColor = { tColor.r / 255.f, tColor.g / 255.f, tColor.b / 255.f };
			const float flAlpha = tColor.a / 255.f;

			auto it = mGlowObjects.find(iIndex);
			if (it == mGlowObjects.end())
				mGlowObjects[iIndex] = new CGlowObject(rcx, pEntity, vColor, flAlpha, true, true, GLOW_FOR_ALL_SPLIT_SCREEN_SLOTS);
			else
			{
				CGlowObject* pGlow = it->second;
				pGlow->SetEntity(pEntity);
				pGlow->SetColor(vColor);
				pGlow->SetAlpha(flAlpha);
				pGlow->SetRenderFlags(true, true);
			}
		}
		else
		{
			auto it = mGlowObjects.find(iIndex);
			if (it != mGlowObjects.end())
			{
				DestroyGlowObject(it);
				mGlowObjects.erase(it);
			}
		}
	}

	for (auto it = mGlowObjects.begin(); it != mGlowObjects.end();)
	{
		if (!I::ClientEntityList->GetClientEntity(it->first) || activeIndices.find(it->first) == activeIndices.end())
		{
			DestroyGlowObject(it);
			it = mGlowObjects.erase(it);
		}
		else
			++it;
	}

	// rebuild so we can ignore the convar checks
	auto pRenderContext = CMatRenderContextPtr(I::MaterialSystem);

	int nX, nY, nWidth, nHeight;
	pRenderContext->GetViewport(nX, nY, nWidth, nHeight);

	PIXEvent _pixEvent(pRenderContext, "EntityGlowEffects");
	// flBloomScale doesn't matter because it's never used
	S::CGlowObjectManager_ApplyEntityGlowEffects.Call<void>(rcx, pSetup, nSplitScreenSlot, &pRenderContext, 999999.f, nX, nY, nWidth, nHeight);
}

void __fastcall CHLClient_CreateMove(void* rcx, int sequence_number, float input_sample_frametime, bool active)
{
	UNLOAD_RETURN(CHLClient_CreateMove, void, rcx, sequence_number, input_sample_frametime, active);

	CALL_ORIGINAL(CHLClient_CreateMove, void, rcx, sequence_number, input_sample_frametime, active);

	auto pLocal = H::Entities.GetLocal();
	auto pWeapon = H::Entities.GetWeapon();
	if (!pLocal)
		return;

	//bool* pSendPacket = reinterpret_cast<bool*>(uintptr_t(_AddressOfReturnAddress()) + 0x20);
	CUserCmd* pCmd = &I::Input->m_pCommands[sequence_number % MULTIPLAYER_BACKUP];

	I::Prediction->Update(I::ClientState->m_nDeltaTick, I::ClientState->m_nDeltaTick > 0, I::ClientState->last_command_ack, I::ClientState->lastoutgoingcommand + I::ClientState->chokedcommands);

	UpdateInfo(pLocal, pWeapon, pCmd);
	F::Spectate.CreateMove(pCmd);
	F::Backtrack.CreateMove(pLocal, pWeapon, pCmd);
	F::Misc.RunPre(pLocal, pCmd);
	F::Ticks.Start(pLocal, pCmd);
	F::Aimbot.Run(pLocal, pWeapon, pCmd);
	F::Ticks.End(pLocal, pCmd);
	F::CritHack.Run(pLocal, pWeapon, pCmd);
	F::NoSpread.Run(pLocal, pWeapon, pCmd);
	F::Resolver.CreateMove();
	F::Misc.RunPost(pLocal, pCmd);
	F::StickyJump.Run(pCmd);
	F::Avoidance.Run(pLocal, pCmd);
	F::PacketManip.Run(pLocal, pWeapon, pCmd);
	F::Visuals.CreateMove(pLocal, pWeapon);
	F::Ticks.CreateMove(pLocal, pWeapon, pCmd);
	F::AntiAim.Run(pLocal, pWeapon, pCmd);
	F::NoSpreadHitscan.AskForPlayerPerf();
	F::EnginePrediction.End(pLocal, pCmd);

	AntiCheatCompatibility(pCmd);
	LocalAnimations(pLocal, pCmd);

	G::Choking = !G::SendPacket;
	G::LastUserCmd = pCmd;
}

bool __fastcall CHLClient_DispatchUserMessage(void* rcx, UserMessageType type, bf_read& msgData)
{
	UNLOAD_RETURN(CHLClient_DispatchUserMessage, bool, rcx, type, std::ref(msgData));

	auto bufData = reinterpret_cast<const char*>(msgData.m_pData);
	msgData.SetAssertOnOverflow(false);
	msgData.Seek(0);

	switch (type)
	{
	case VoteStart:
		F::Output.UserMessage(msgData);
		F::AutoVote.UserMessage(msgData);

		break;
	case VoiceSubtitle:
	{
		int iEntityID = msgData.ReadByte();
		int iVoiceMenu = msgData.ReadByte();
		int iCommandID = msgData.ReadByte();
		if (iVoiceMenu == 1 && iCommandID == 6)
			F::AutoHeal.m_mMedicCallers[iEntityID];

		break;
	}
	case TextMsg:
	{
		char rawMsg[256]; msgData.ReadString(rawMsg, sizeof(rawMsg), true);
		msgData.Seek(0);
		std::string sMsg = rawMsg;
		if (!sMsg.empty())
		{
			sMsg.erase(sMsg.begin());

			if (F::NoSpreadHitscan.ParsePlayerPerf(sMsg))
				return true;

#ifdef DEBUG_VISUALS
			if (sMsg.find("[Box] ") == 0)
			{
				try
				{
					sMsg.replace(0, strlen("[Box] "), "");
					std::vector<std::string> vValues = {};
					boost::split(vValues, sMsg, boost::is_any_of(" "));
					if (vValues.size() != 17)
						return true;

					Vec3 vOrigin = { std::stof(vValues[0]), std::stof(vValues[1]), std::stof(vValues[2]) };
					Vec3 vMins = { std::stof(vValues[3]), std::stof(vValues[4]), std::stof(vValues[5]) };
					Vec3 vMaxs = { std::stof(vValues[6]), std::stof(vValues[7]), std::stof(vValues[8]) };
					Vec3 vAngles = { std::stof(vValues[9]), std::stof(vValues[10]), std::stof(vValues[11]) };
					Color_t tColor = { byte(std::stoi(vValues[12])), byte(std::stoi(vValues[13])), byte(std::stoi(vValues[14])), byte(255 - std::stoi(vValues[15])) };
					float flDuration = std::stof(vValues[16]);

					G::BoxStorage.emplace_back(vOrigin, vMins, vMaxs, vAngles, I::GlobalVars->curtime + flDuration, tColor, Color_t(0, 0, 0, 0), true);
				}
				catch (...) {}

				return true;
			}
			if (sMsg.find("[Line] ") == 0)
			{
				try
				{
					sMsg.replace(0, strlen("[Line] "), "");
					std::vector<std::string> vValues = {};
					boost::split(vValues, sMsg, boost::is_any_of(" "));
					if (vValues.size() != 11)
						return true;

					Vec3 vStart = { std::stof(vValues[0]), std::stof(vValues[1]), std::stof(vValues[2]) };
					Vec3 vEnd = { std::stof(vValues[3]), std::stof(vValues[4]), std::stof(vValues[5]) };
					Color_t tColor = { byte(std::stoi(vValues[6])), byte(std::stoi(vValues[7])), byte(std::stoi(vValues[8])), byte(255 - std::stoi(vValues[9])) };
					float flDuration = std::stof(vValues[10]);

					G::LineStorage.emplace_back(std::pair<Vec3, Vec3>(vStart, vEnd), I::GlobalVars->curtime + flDuration, tColor, true);
				}
				catch (...) {}

				return true;
			}
#endif

			if (Vars::Misc::Automation::AntiAutobalance.Value && FNV1A::Hash32(sMsg.c_str()) == FNV1A::Hash32Const("#TF_Autobalance_TeamChangePending"))
				I::EngineClient->ClientCmd_Unrestricted("retry");
		}
		break;
	}
	case VGUIMenu:
		if (Vars::Visuals::Removals::MOTD.Value && bufData
			&& FNV1A::Hash32(bufData) == FNV1A::Hash32Const("info"))
		{
			I::EngineClient->ClientCmd_Unrestricted("closedwelcomemenu");
			return true;
		}
		break;
	case ForcePlayerViewAngles:
		return Vars::Visuals::Removals::AngleForcing.Value ? true : CALL_ORIGINAL(CHLClient_DispatchUserMessage, bool, rcx, type, std::ref(msgData));
	case SpawnFlyingBird:
	case PlayerGodRayEffect:
	case PlayerTauntSoundLoopStart:
	case PlayerTauntSoundLoopEnd:
		return Vars::Visuals::Removals::Taunts.Value ? true : CALL_ORIGINAL(CHLClient_DispatchUserMessage, bool, rcx, type, std::ref(msgData));
	case Shake:
	case Fade:
	case Rumble:
		return Vars::Visuals::Removals::ScreenEffects.Value ? true : CALL_ORIGINAL(CHLClient_DispatchUserMessage, bool, rcx, type, std::ref(msgData));
	}

	msgData.Seek(0);
	return CALL_ORIGINAL(CHLClient_DispatchUserMessage, bool, rcx, type, std::ref(msgData));
}

void __fastcall CHLClient_FrameStageNotify(void* rcx, ClientFrameStage_t curStage)
{
	UNLOAD_RETURN(CHLClient_FrameStageNotify, void, rcx, curStage);

	CALL_ORIGINAL(CHLClient_FrameStageNotify, void, rcx, curStage);

	switch (curStage)
	{
	case FRAME_NET_UPDATE_START:
	{
		auto pLocal = H::Entities.GetLocal();
		F::Spectate.NetUpdateStart(pLocal);

		H::Entities.Clear();
		break;
	}
	case FRAME_NET_UPDATE_END:
	{
		H::Entities.Store();
		F::PlayerUtils.Store();

		F::Backtrack.Store();
		F::MoveSim.Store();
		F::CritHack.Store();

		auto pLocal = H::Entities.GetLocal();
		F::Groups.Store(pLocal);
		F::ESP.Store(pLocal);
		F::Chams.Store(pLocal);
		F::Glow.Store(pLocal);
		F::Arrows.Store(pLocal);

		F::CheaterDetection.Run();
		F::Spectate.NetUpdateEnd(pLocal);

		F::Visuals.Modulate();
		break;
	}
	case FRAME_RENDER_START:
		for (auto& tBind : F::Binds.m_vBinds)
		{	// don't drop inputs for binds
			if (tBind.m_iType != BindEnum::Key)
				continue;

			auto& tKey = tBind.m_tKeyStorage;

			bool bOldIsDown = tKey.m_bIsDown;
			bool bOldIsPressed = tKey.m_bIsPressed;
			bool bOldIsDouble = tKey.m_bIsDouble;
			bool bOldIsReleased = tKey.m_bIsReleased;

			U::KeyHandler.StoreKey(tBind.m_iKey, &tKey);

			tKey.m_bIsDown = tKey.m_bIsDown || bOldIsDown;
			tKey.m_bIsPressed = tKey.m_bIsPressed || bOldIsPressed;
			tKey.m_bIsDouble = tKey.m_bIsDouble || bOldIsDouble;
			tKey.m_bIsReleased = tKey.m_bIsReleased || bOldIsReleased;
		}
		break;
	}
}

void __fastcall CHLClient_LevelShutdown(void* rcx)
{
	UNLOAD_RETURN(CHLClient_LevelShutdown, void, rcx);

	H::Entities.Clear(true);
	F::EnginePrediction.Unload();
	F::Spectate.Reset();

	CALL_ORIGINAL(CHLClient_LevelShutdown, void, rcx);
}

void __fastcall CHLTVCamera_CalcView(void* rcx, Vector& origin, QAngle& angles, float& fov)
{
	UNLOAD_RETURN(CHLTVCamera_CalcView, void, rcx, std::ref(origin), std::ref(angles), std::ref(fov));

	auto pHLTVCamera = reinterpret_cast<CHLTVCamera*>(rcx);

	if (F::Spectate.HasTarget())
		pHLTVCamera->m_nCameraMode = Vars::Visuals::Thirdperson::Enabled.Value ? OBS_MODE_THIRDPERSON : OBS_MODE_FIRSTPERSON;

	auto pEntity = I::ClientEntityList->GetClientEntity(pHLTVCamera->m_iTraget1)->As<CTFPlayer>();
	if (!pEntity)
		return CALL_ORIGINAL(CHLTVCamera_CalcView, void, rcx, std::ref(origin), std::ref(angles), std::ref(fov));

	auto pGameRules = I::TFGameRules();
	auto pViewVectors = pGameRules ? pGameRules->GetViewVectors() : nullptr;
	if (!pViewVectors)
		return CALL_ORIGINAL(CHLTVCamera_CalcView, void, rcx, std::ref(origin), std::ref(angles), std::ref(fov));

	Vec3 vOriginalOffset = pViewVectors->m_vView;
	pViewVectors->m_vView = pEntity->GetViewOffset(pHLTVCamera->m_iCameraMan <= 0);
	CALL_ORIGINAL(CHLTVCamera_CalcView, void, rcx, std::ref(origin), std::ref(angles), std::ref(fov));
	pViewVectors->m_vView = vOriginalOffset;
}

CBaseEntity* __fastcall CHLTVCamera_GetPrimaryTarget(void* rcx)
{
	UNLOAD_RETURN(CHLTVCamera_GetPrimaryTarget, CBaseEntity*, rcx);

	auto pHLTVCamera = reinterpret_cast<CHLTVCamera*>(rcx);

	if (F::Spectate.HasTarget())
		pHLTVCamera->m_iTraget1 = I::EngineClient->GetPlayerForUserID(F::Spectate.GetTarget()), F::Spectate.Reset();

	return CALL_ORIGINAL(CHLTVCamera_GetPrimaryTarget, CBaseEntity*, rcx);
}

int __fastcall CHLTVCamera_GetMode(void* rcx)
{
	UNLOAD_RETURN(CHLTVCamera_GetMode, int, rcx);

	auto pHLTVCamera = reinterpret_cast<CHLTVCamera*>(rcx);

	if (F::Spectate.HasTarget())
		pHLTVCamera->m_nCameraMode = Vars::Visuals::Thirdperson::Enabled.Value ? OBS_MODE_THIRDPERSON : OBS_MODE_FIRSTPERSON;

	return CALL_ORIGINAL(CHLTVCamera_GetMode, int, rcx);
}

int* __fastcall CHudChat_GetClientColor(void* rcx, int* iOutColor, int clientIndex)
{
	UNLOAD_RETURN(CHudChat_GetClientColor, int*, rcx, iOutColor, clientIndex);
	
	static thread_local int iChatColor;

	if (!Vars::Visuals::UI::ChatColors.Value || clientIndex == 0 || !F::Groups.GroupsActive())
		return CALL_ORIGINAL(CHudChat_GetClientColor, int*, rcx, iOutColor, clientIndex);

	auto pLocal = H::Entities.GetLocal();
	if (!pLocal)
		return CALL_ORIGINAL(CHudChat_GetClientColor, int*, rcx, iOutColor, clientIndex);

	if (clientIndex == I::EngineClient->GetLocalPlayer())
	{
		iChatColor = ColorToInt(Vars::Colors::Local.Value);
		return &iChatColor;
	}

	if (auto pPlayer = I::ClientEntityList->GetClientEntity(clientIndex)->As<CTFPlayer>())
	{
		Group_t* pGroup{};
		if (F::Groups.GroupsActive() && F::Groups.GetGroup(pPlayer, pGroup, false))
		{
			iChatColor = ColorToInt(F::Groups.GetColor(pPlayer, pGroup));
			return &iChatColor; // this is like really dangerous and could cause crashes but it seems to work fine?
		}
	}

	return CALL_ORIGINAL(CHudChat_GetClientColor, int*, rcx, iOutColor, clientIndex);
}

void __fastcall CHudCrosshair_GetDrawPosition(float* pX, float* pY, bool* pbBehindCamera, Vec3 angleCrosshairOffset)
{
	UNLOAD_RETURN(CHudCrosshair_GetDrawPosition, void, pX, pY, pbBehindCamera, angleCrosshairOffset);

	if (!Vars::Visuals::Viewmodel::CrosshairAim.Value && !Vars::Visuals::Thirdperson::Crosshair.Value
		|| Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return CALL_ORIGINAL(CHudCrosshair_GetDrawPosition, void, pX, pY, pbBehindCamera, angleCrosshairOffset);

	auto pLocal = H::Entities.GetLocal();
	if (!pLocal)
		return CALL_ORIGINAL(CHudCrosshair_GetDrawPosition, void, pX, pY, pbBehindCamera, angleCrosshairOffset);

	bool bSet = false;

	if (Vars::Visuals::Viewmodel::CrosshairAim.Value && pLocal->IsAlive() && G::AimPoint.m_iTickCount)
	{
		Vec3 vScreen;
		if (SDK::W2S(G::AimPoint.m_vOrigin, vScreen))
		{
			if (pX) *pX = vScreen.x;
			if (pY) *pY = vScreen.y;
			if (pbBehindCamera) *pbBehindCamera = false;
			bSet = true;
		}
	}

	if (Vars::Visuals::Thirdperson::Crosshair.Value && !bSet && I::Input->CAM_IsThirdPerson())
	{
		Vec3 vAngles = I::EngineClient->GetViewAngles();
		Vec3 vForward; Math::AngleVectors(vAngles, &vForward);

		Vec3 vStartPos = pLocal->GetEyePosition();
		Vec3 vEndPos = vStartPos + vForward * 8192;

		CGameTrace trace = {};
		CTraceFilterHitscan filter = {};
		filter.pSkip = pLocal;
		SDK::Trace(vStartPos, vEndPos, MASK_SHOT, &filter, &trace);

		Vec3 vScreen;
		if (SDK::W2S(trace.endpos, vScreen))
		{
			if (pX) *pX = vScreen.x;
			if (pY) *pY = vScreen.y;
			if (pbBehindCamera) *pbBehindCamera = false;
			bSet = true;
		}
	}

	if (!bSet)
		CALL_ORIGINAL(CHudCrosshair_GetDrawPosition, void, pX, pY, pbBehindCamera, angleCrosshairOffset);
}

CUserCmd* __fastcall CInput_GetUserCmd(void* rcx, int sequence_number)
{
	UNLOAD_RETURN(CInput_GetUserCmd, CUserCmd*, rcx, sequence_number);

	return &I::Input->m_pCommands[sequence_number % MULTIPLAYER_BACKUP];
}

void __fastcall CInput_ValidateUserCmd(void* rcx, CUserCmd* usercmd, int sequence_number)
{
	UNLOAD_RETURN(CInput_ValidateUserCmd, void, rcx, usercmd, sequence_number);

	return;
}

bool __fastcall CInventoryManager_ShowItemsPickedUp(void* rcx, bool bForce, bool bReturnToGame, bool bNoPanel)
{
	UNLOAD_RETURN(CInventoryManager_ShowItemsPickedUp, bool, rcx, bForce, bReturnToGame, bNoPanel);

	if (Vars::Misc::Automation::AcceptItemDrops.Value)
	{
		CALL_ORIGINAL(CInventoryManager_ShowItemsPickedUp, bool, rcx, true, true, true);
		return false;
	}
	return CALL_ORIGINAL(CInventoryManager_ShowItemsPickedUp, bool, rcx, bForce, bReturnToGame, bNoPanel);
}

void __fastcall CL_CheckForPureServerWhitelist(void** pFilesToReload)
{
	UNLOAD_RETURN(CL_CheckForPureServerWhitelist, void, pFilesToReload);

	if (Vars::Misc::Exploits::PureBypass.Value)
		return;

	CALL_ORIGINAL(CL_CheckForPureServerWhitelist, void, pFilesToReload);
}

void __fastcall CL_Move(float accumulated_extra_samples, bool bFinalTick)
{
	UNLOAD_RETURN(CL_Move, void, accumulated_extra_samples, bFinalTick);

	F::NetworkFix.FixInputDelay(bFinalTick);
	F::Backtrack.m_iTickCount = I::GlobalVars->tickcount + 1;
	if (!Vars::Misc::Game::NetworkFix.Value && !SDK::IsLoopback())
		F::Backtrack.m_iTickCount++;

	F::Binds.Run();
	F::PlayerCore.Run();
	F::Backtrack.SendLerp();
	F::Misc.PingReducer();
	F::AutoQueue.Run();

	F::Ticks.Move(accumulated_extra_samples, bFinalTick);

	for (auto& Line : G::PathStorage)
	{
		if (Line.m_flTime < 0.f)
			Line.m_flTime = std::min(Line.m_flTime + 1.f, 0.f);
	}
}

bool __fastcall CL_ProcessPacketEntities(SVC_PacketEntities* entmsg)
{
	UNLOAD_RETURN(CL_ProcessPacketEntities, bool, entmsg);

	if (entmsg->m_bIsDelta) // we won't need to restore
		return CALL_ORIGINAL(CL_ProcessPacketEntities, bool, entmsg);

	CTFPlayer* pLocal = H::Entities.GetLocal();
	if (!pLocal || !pLocal->m_hMyWeapons())
	{
		SDK::Output("ProcessPacketEntities", "Failed to restore weapon crit data! (1)", { 255, 100, 100 });
		return CALL_ORIGINAL(CL_ProcessPacketEntities, bool, entmsg);
	}

	std::unordered_map<int, CriticalStorage_t> mCriticalStorage = {};

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		auto pWeapon = pLocal->GetWeaponFromSlot(i);
		if (!pWeapon)
			continue;

		mCriticalStorage[i].m_flCritTokenBucket = pWeapon->m_flCritTokenBucket();
		mCriticalStorage[i].m_nCritChecks = pWeapon->m_nCritChecks();
		mCriticalStorage[i].m_nCritSeedRequests = pWeapon->m_nCritSeedRequests();

		if (Vars::Debug::Logging.Value) I::CVar->ConsolePrintf("\n");
		SDK::Output("ProcessPacketEntities", std::format("{} ({:#x}): mCriticalStorage[i].m_flCritTokenBucket = {}", i, uintptr_t(pWeapon), pWeapon->m_flCritTokenBucket()).c_str(), { 100, 150, 255 }, Vars::Debug::Logging.Value);
		SDK::Output("ProcessPacketEntities", std::format("{} ({:#x}): mCriticalStorage[i].m_nCritChecks = {}", i, uintptr_t(pWeapon), pWeapon->m_nCritChecks()).c_str(), { 100, 150, 255 }, Vars::Debug::Logging.Value);
		SDK::Output("ProcessPacketEntities", std::format("{} ({:#x}): mCriticalStorage[i].m_nCritSeedRequests = {}", i, uintptr_t(pWeapon), pWeapon->m_nCritSeedRequests()).c_str(), { 100, 150, 255 }, Vars::Debug::Logging.Value);
	}

	bool bReturn = CALL_ORIGINAL(CL_ProcessPacketEntities, bool, entmsg);

	pLocal = H::Entities.GetLocal();
	if (!pLocal || !pLocal->m_hMyWeapons())
	{
		SDK::Output("ProcessPacketEntities", "Failed to restore weapon crit data! (2)", { 255, 100, 100 });
		return bReturn;
	}

	for (auto& [iSlot, tStorage] : mCriticalStorage)
	{
		auto pWeapon = pLocal->GetWeaponFromSlot(iSlot);
		if (!pWeapon)
			break;

		pWeapon->m_flCritTokenBucket() = tStorage.m_flCritTokenBucket;
		pWeapon->m_nCritChecks() = tStorage.m_nCritChecks;
		pWeapon->m_nCritSeedRequests() = tStorage.m_nCritSeedRequests;

		if (Vars::Debug::Logging.Value) I::CVar->ConsolePrintf("\n");
		SDK::Output("ProcessPacketEntities", std::format("{} ({:#x}): pWeapon->m_flCritTokenBucket() = {}", iSlot, uintptr_t(pWeapon), pWeapon->m_flCritTokenBucket()).c_str(), { 100, 255, 150 }, Vars::Debug::Logging.Value);
		SDK::Output("ProcessPacketEntities", std::format("{} ({:#x}): pWeapon->m_nCritChecks() = {}", iSlot, uintptr_t(pWeapon), pWeapon->m_nCritChecks()).c_str(), { 100, 255, 150 }, Vars::Debug::Logging.Value);
		SDK::Output("ProcessPacketEntities", std::format("{} ({:#x}): pWeapon->m_nCritSeedRequests() = {}", iSlot, uintptr_t(pWeapon), pWeapon->m_nCritSeedRequests()).c_str(), { 100, 255, 150 }, Vars::Debug::Logging.Value);
	}

	return bReturn;
}

void __fastcall CL_ReadPackets(bool bFinalTick)
{
	UNLOAD_RETURN(CL_ReadPackets, void, bFinalTick);

	if (F::NetworkFix.ShouldReadPackets())
		CALL_ORIGINAL(CL_ReadPackets, void, bFinalTick);
}

bool __fastcall ClientModeTFNormal_BIsFriendOrPartyMember(void* rcx, CBaseEntity* pEntity)
{
	UNLOAD_RETURN(ClientModeTFNormal_BIsFriendOrPartyMember, bool, rcx, pEntity);

	static const auto dwDesired = S::CHudInspectPanel_UserCmd_InspectTarget_BIsFriendOrPartyMember_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	if (dwRetAddr == dwDesired && Vars::Misc::MannVsMachine::AllowInspect.Value)
		return true;
	return CALL_ORIGINAL(ClientModeTFNormal_BIsFriendOrPartyMember, bool, rcx, pEntity);
}

void __fastcall CMatchInviteNotification_OnTick(void* rcx)
{
	UNLOAD_RETURN(CMatchInviteNotification_OnTick, void, rcx);

	if (Vars::Misc::Queueing::FreezeQueue.Value)
		*reinterpret_cast<double*>(uintptr_t(rcx) + 616) = 0.0;

	CALL_ORIGINAL(CMatchInviteNotification_OnTick, void, rcx);
}

void __fastcall CMaterial_Uncache(IMaterial* rcx, bool bPreserveVars)
{
	UNLOAD_RETURN(CMaterial_Uncache, void, rcx, bPreserveVars);

	if (F::Materials.m_mMatList.contains(rcx))
		return;

	CALL_ORIGINAL(CMaterial_Uncache, void, rcx, bPreserveVars);
}

int __fastcall CNetChannel_SendDatagram(CNetChannel* pNetChan, bf_write* datagram)
{
	UNLOAD_RETURN(CNetChannel_SendDatagram, int, pNetChan, datagram);

	if (datagram)
		return CALL_ORIGINAL(CNetChannel_SendDatagram, int, pNetChan, datagram);

	F::Backtrack.AdjustPing(pNetChan);
	const int iReturn = CALL_ORIGINAL(CNetChannel_SendDatagram, int, pNetChan, datagram);
	F::Backtrack.RestorePing(pNetChan);
	return iReturn;
}

bool __fastcall CNetChannel_SendNetMsg(CNetChannel* pNetChan, INetMessage& msg, bool bForceReliable, bool bVoice)
{
	UNLOAD_RETURN(CNetChannel_SendNetMsg, bool, pNetChan, std::ref(msg), bForceReliable, bVoice);

	switch (msg.GetType())
	{
	case net_SetConVar:
	{
		auto pMsg = reinterpret_cast<NET_SetConVar*>(&msg);
		for (int i = 0; i < pMsg->m_ConVars.Count(); i++)
		{
			NET_SetConVar::CVar_t* localCvar = &pMsg->m_ConVars[i];

			// intercept and change any vars we want to control
			switch (FNV1A::Hash32(localCvar->Name))
			{
			case FNV1A::Hash32Const("cl_interp"):
				if (F::Backtrack.m_flSentInterp != -1.f)
					strncpy_s(localCvar->Value, std::to_string(F::Backtrack.m_flSentInterp).c_str(), MAX_OSPATH);
				if (Vars::Misc::Game::AntiCheatCompatibility.Value)
				{
					try {
						float flValue = std::stof(localCvar->Value);
						strncpy_s(localCvar->Value, std::to_string(std::min(flValue, 0.1f)).c_str(), MAX_OSPATH);
					}
					catch (...) {};
				}
				break;
			case FNV1A::Hash32Const("cl_cmdrate"):
				if (F::Misc.m_iWishCmdrate != -1)
					strncpy_s(localCvar->Value, std::to_string(F::Misc.m_iWishCmdrate).c_str(), MAX_OSPATH);
				if (Vars::Misc::Game::AntiCheatCompatibility.Value)
				{
					try {
						int iValue = std::stof(localCvar->Value);
						strncpy_s(localCvar->Value, std::to_string(std::max(iValue, 10)).c_str(), MAX_OSPATH);
					}
					catch (...) {};
				}
				break;
			case FNV1A::Hash32Const("cl_updaterate"):
				if (F::Misc.m_iWishUpdaterate != -1)
					strncpy_s(localCvar->Value, std::to_string(F::Misc.m_iWishUpdaterate).c_str(), MAX_OSPATH);
				break;
			case FNV1A::Hash32Const("cl_interp_ratio"):
			case FNV1A::Hash32Const("cl_interpolate"):
				strncpy_s(localCvar->Value, "1", MAX_OSPATH);
			}

			if (Vars::Debug::Logging.Value)
			{
				switch (FNV1A::Hash32(localCvar->Name))
				{
				case FNV1A::Hash32Const("cl_interp"):
				case FNV1A::Hash32Const("cl_interp_ratio"):
				case FNV1A::Hash32Const("cl_interpolate"):
				case FNV1A::Hash32Const("cl_cmdrate"):
				case FNV1A::Hash32Const("cl_updaterate"):
					SDK::Output("SendNetMsg", std::format("{}: {}", localCvar->Name, localCvar->Value).c_str(), { 100, 0, 255 });
				}
			}
		}
		break;
	}
	case clc_VoiceData:
		// stop lag with voice chat
		bVoice = true;
		break;
	case clc_RespondCvarValue:
		if (Vars::Misc::Game::AntiCheatCompatibility.Value)
		{
			auto pMsg = reinterpret_cast<uintptr_t*>(&msg);
			if (!pMsg) break;

			auto cvarName = reinterpret_cast<const char*>(pMsg[6]);
			if (!cvarName) break;

			auto pConVar = U::ConVars.FindVar(cvarName);
			if (!pConVar) break;

			static std::string sValue = "";
			switch (FNV1A::Hash32(cvarName))
			{
			case FNV1A::Hash32Const("cl_interp"):
				if (F::Backtrack.m_flSentInterp != -1.f)
					sValue = std::to_string(std::min(F::Backtrack.m_flSentInterp, 0.1f));
				else
					sValue = pConVar->GetString();
				break;
			case FNV1A::Hash32Const("cl_interp_ratio"):
				sValue = "1";
				break;
			case FNV1A::Hash32Const("cl_cmdrate"):
				if (F::Misc.m_iWishCmdrate != -1)
					sValue = std::to_string(F::Misc.m_iWishCmdrate);
				else
					sValue = pConVar->GetString();
				break;
			case FNV1A::Hash32Const("cl_updaterate"):
				if (F::Misc.m_iWishUpdaterate != -1)
					sValue = std::to_string(F::Misc.m_iWishUpdaterate);
				else
					sValue = pConVar->GetString();
				break;
			case FNV1A::Hash32Const("mat_dxlevel"):
				sValue = pConVar->GetString();
				break;
			default:
				sValue = pConVar->m_pParent->m_pszDefaultValue;
			}
			pMsg[7] = uintptr_t(sValue.c_str());

			SDK::Output("Convar spoof", msg.ToString(), Vars::Menu::Theme::Accent.Value, Vars::Debug::Logging.Value);
		}
		break;
	}

	return CALL_ORIGINAL(CNetChannel_SendNetMsg, bool, pNetChan, std::ref(msg), bForceReliable, bVoice);
}

void __fastcall COPRenderSprites_Render(void* rcx, IMatRenderContext* pRenderContext, CParticleCollection* pParticles, void* pContext)
{
	UNLOAD_RETURN(COPRenderSprites_Render, void, rcx, pRenderContext, pParticles, pContext);

	if (Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return CALL_ORIGINAL(COPRenderSprites_Render, void, rcx, pRenderContext, pParticles, pContext);

	bool bValid = false;

	if (Vars::Visuals::Effects::DrawCritsThroughWalls.Value)
	{
		std::string sParticleName = pParticles->m_pDef->m_Name.m_pString;
		std::transform(sParticleName.begin(), sParticleName.end(), sParticleName.begin(), ::tolower);
		if (sParticleName.find("crit") != std::string::npos)
			bValid = true;
	}

	if (Vars::Visuals::Effects::DrawIconsThroughWalls.Value && !bValid)
		switch (FNV1A::Hash32(pParticles->m_pDef->m_pszMaterialName))
		{
			// blue icons
		case FNV1A::Hash32Const("effects\\defense_buff_bullet_blue.vmt"):
		case FNV1A::Hash32Const("effects\\defense_buff_explosion_blue.vmt"):
		case FNV1A::Hash32Const("effects\\defense_buff_fire_blue.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_agility_icon_blue.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_haste_icon_blue.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_king_icon_blue.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_knockout_icon_blue.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_plague_icon_blue.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_precision_icon_blue.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_reflect_icon_blue.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_resist_icon_blue.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_strength_icon_blue.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_supernova_icon_blue.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_thorns_icon_blue.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_vampire_icon_blue.vmt"):
		{
			auto pLocal = H::Entities.GetLocal();
			bValid = !pLocal || pLocal->m_iTeamNum() != TF_TEAM_BLUE;
			break;
		}
		// red icons
		case FNV1A::Hash32Const("effects\\defense_buff_bullet_red.vmt"):
		case FNV1A::Hash32Const("effects\\defense_buff_explosion_red.vmt"):
		case FNV1A::Hash32Const("effects\\defense_buff_fire_red.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_agility_icon_red.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_haste_icon_red.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_king_icon_red.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_knockout_icon_red.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_plague_icon_red.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_precision_icon_red.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_reflect_icon_red.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_regen_icon_blue.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_regen_icon_red.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_resist_icon_red.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_strength_icon_red.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_supernova_icon_red.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_thorns_icon_red.vmt"):
		case FNV1A::Hash32Const("effects\\powerup_vampire_icon_red.vmt"):
		{
			auto pLocal = H::Entities.GetLocal();
			bValid = !pLocal || pLocal->m_iTeamNum() != TF_TEAM_RED;
			break;
		}
		case FNV1A::Hash32Const("effects\\particle_nemesis_blue.vmt"):
		case FNV1A::Hash32Const("effects\\particle_nemesis_red.vmt"):
		case FNV1A::Hash32Const("effects\\particle_nemesis_burst.vmt"):
		case FNV1A::Hash32Const("effects\\duel_blue.vmt"):
		case FNV1A::Hash32Const("effects\\duel_red.vmt"):
		case FNV1A::Hash32Const("effects\\duel_burst.vmt"):
		case FNV1A::Hash32Const("effects\\crit.vmt"):
		case FNV1A::Hash32Const("effects\\yikes.vmt"):
			bValid = true;
		}

	if (!bValid)
		return CALL_ORIGINAL(COPRenderSprites_Render, void, rcx, pRenderContext, pParticles, pContext);

	pRenderContext->DepthRange(0.f, 0.2f);
	CALL_ORIGINAL(COPRenderSprites_Render, void, rcx, pRenderContext, pParticles, pContext);
	pRenderContext->DepthRange(0.f, 1.f);
}

static CBaseEntity* pOwnerEnt = nullptr;
static std::unordered_map<int, std::unordered_set<void*>> ParticlesPerOwner{};
static std::unordered_map<void*, int> OwnerPerParticle{};

static inline CBaseEntity* FindOwner(void* rcx)
{
	auto it = OwnerPerParticle.find(rcx);
	if (it != OwnerPerParticle.end())
	{
		int idx = it->second;
		if (auto pEntity = I::ClientEntityList->GetClientEntity(idx)->As<CBaseEntity>())
			return pEntity;
	}
	return nullptr;
}

int __fastcall CNewParticleEffect_DrawModel(void* rcx, int flags)
{
	UNLOAD_RETURN(CNewParticleEffect_DrawModel, int, rcx, flags);

	if (Vars::Visuals::World::ParticleModulationStyle.Value != Vars::Visuals::World::ParticleModulationStyleEnum::GroupColored)
		return CALL_ORIGINAL(CNewParticleEffect_DrawModel, int, rcx, flags);

	// reserved here and not outside func so we dont dump memory into these when we will never use it
	static bool bInitialized = false;
	if (!bInitialized)
	{
		ParticlesPerOwner.reserve(size_t(I::EngineClient->GetMaxClients()));
		OwnerPerParticle.reserve(size_t(I::EngineClient->GetMaxClients() * 8));
		for (size_t i = 1; i <= 32; i++)
			ParticlesPerOwner[i].reserve(size_t(8));
		bInitialized = true;
	}

	pOwnerEnt = (*(EHANDLE*)(((uintptr_t)rcx - 16) + 10212)).Get();
	if (!pOwnerEnt)
	{
		if (auto pOwner = FindOwner(rcx))
			pOwnerEnt = pOwner;
		else
			return CALL_ORIGINAL(CNewParticleEffect_DrawModel, int, rcx, flags);
	}

	// find x's owner instead of making the particle owner x itself
	switch (pOwnerEnt->GetClassID())
	{
	case ETFClassID::CTFPlayer:
		break;
	case ETFClassID::CBaseGrenade:
	case ETFClassID::CTFWeaponBaseGrenadeProj:
	case ETFClassID::CTFWeaponBaseMerasmusGrenade:
	case ETFClassID::CTFGrenadePipebombProjectile:
	case ETFClassID::CTFStunBall:
	case ETFClassID::CTFBall_Ornament:
	case ETFClassID::CTFProjectile_Jar:
	case ETFClassID::CTFProjectile_Cleaver:
	case ETFClassID::CTFProjectile_JarGas:
	case ETFClassID::CTFProjectile_JarMilk:
	case ETFClassID::CTFProjectile_SpellBats:
	case ETFClassID::CTFProjectile_SpellKartBats:
	case ETFClassID::CTFProjectile_SpellMeteorShower:
	case ETFClassID::CTFProjectile_SpellMirv:
	case ETFClassID::CTFProjectile_SpellPumpkin:
	case ETFClassID::CTFProjectile_SpellSpawnBoss:
	case ETFClassID::CTFProjectile_SpellSpawnHorde:
	case ETFClassID::CTFProjectile_SpellSpawnZombie:
	case ETFClassID::CTFProjectile_SpellTransposeTeleport:
	case ETFClassID::CTFProjectile_Throwable:
	case ETFClassID::CTFProjectile_ThrowableBreadMonster:
	case ETFClassID::CTFProjectile_ThrowableBrick:
	case ETFClassID::CTFProjectile_ThrowableRepel:
	case ETFClassID::CTFBaseRocket:
	case ETFClassID::CTFFlameRocket:
	case ETFClassID::CTFProjectile_Arrow:
	case ETFClassID::CTFProjectile_GrapplingHook:
	case ETFClassID::CTFProjectile_HealingBolt:
	case ETFClassID::CTFProjectile_Rocket:
	case ETFClassID::CTFProjectile_BallOfFire:
	case ETFClassID::CTFProjectile_MechanicalArmOrb:
	case ETFClassID::CTFProjectile_SentryRocket:
	case ETFClassID::CTFProjectile_SpellFireball:
	case ETFClassID::CTFProjectile_SpellLightningOrb:
	case ETFClassID::CTFProjectile_SpellKartOrb:
	case ETFClassID::CTFProjectile_EnergyBall:
	case ETFClassID::CTFProjectile_Flare:
	case ETFClassID::CTFBaseProjectile:
	case ETFClassID::CTFProjectile_EnergyRing:
	{
		auto pOwner = F::ProjSim.GetEntities(pOwnerEnt).second->As<CBaseEntity>();
		pOwnerEnt = pOwner ? pOwner : pOwnerEnt;
		break;
	}
	case ETFClassID::CBaseObject:
	case ETFClassID::CObjectSentrygun:
	case ETFClassID::CObjectDispenser:
	case ETFClassID::CObjectTeleporter:
		if (pOwnerEnt->IsBuilding())
		{
			auto pBuildingOwner = pOwnerEnt->As<CBaseObject>()->m_hBuilder().Get();
			pOwnerEnt = pBuildingOwner ? pBuildingOwner : pOwnerEnt;
			break;
		}
		[[fallthrough]];
	case ETFClassID::CTFFlameManager:
	{
		auto pWeapon = pOwnerEnt->As<CTFFlameManager>()->m_hWeapon().Get();
		pOwnerEnt = pWeapon ? pWeapon->m_hOwnerEntity()->As<CTFPlayer>() : pOwnerEnt;
		break;
	}
	case ETFClassID::CTFViewModel:
	{
		auto pOwner = pOwnerEnt->As<CBaseViewModel>();
		pOwnerEnt = pOwner ? pOwner->m_hOwner().Get()->As<CTFPlayer>() : pOwnerEnt;
		break;
	}
	case ETFClassID::CWeaponMedigun:
	{
		auto pOwner = pOwnerEnt->As<CWeaponMedigun>();
		pOwnerEnt = pOwner ? pOwner->m_hOwner().Get()->As<CTFPlayer>() : pOwnerEnt;
		break;
	}
	case ETFClassID::CTFRagdoll:
	case ETFClassID::CRagdollProp:
	case ETFClassID::CRagdollPropAttached:
	{
		auto pOwner = pOwnerEnt->As<CTFRagdoll>()->m_hPlayer().Get();
		pOwnerEnt = pOwner ? pOwner : pOwnerEnt;
		break;
	}
	default:
	{
		auto pOwner = pOwnerEnt->m_hOwnerEntity().Get();
		pOwnerEnt = pOwner ? pOwner : pOwnerEnt;
		break;
	}
	}

	if (pOwnerEnt)
	{   // crashes sometimes for god knows what reason so put a safety check
		if (int iIndex = pOwnerEnt->entindex())
		{
			ParticlesPerOwner[iIndex].insert(rcx);
			OwnerPerParticle[rcx] = iIndex;
			//SDK::Output(pOwnerEnt->GetClientClass()->GetName());
		}
	}

	auto original = CALL_ORIGINAL(CNewParticleEffect_DrawModel, int, rcx, flags);
	pOwnerEnt = nullptr;
	return original;
}

void __fastcall CNewParticleEffect_Deconstructor(void* rcx)
{
	UNLOAD_RETURN(CNewParticleEffect_Deconstructor, void, rcx);

	CALL_ORIGINAL(CNewParticleEffect_Deconstructor, void, rcx);

	if (Vars::Visuals::World::ParticleModulationStyle.Value != Vars::Visuals::World::ParticleModulationStyleEnum::GroupColored)
		return;

	// delete it from the list once the game is done with it
	auto toFind = (void*)((uintptr_t)rcx + 0x10); // cnewparticleeffect_drawmodel uses some offset from the actual particle effect
	auto particleIt = OwnerPerParticle.find(toFind);
	if (particleIt != OwnerPerParticle.end())
	{
		int owner = particleIt->second;

		auto ownerIt = ParticlesPerOwner.find(owner);
		if (ownerIt != ParticlesPerOwner.end())
		{
			ownerIt->second.erase(toFind);
			if (ownerIt->second.empty())
				ParticlesPerOwner.erase(ownerIt);
		}

		OwnerPerParticle.erase(particleIt);
	}
}

void __fastcall COPRenderSprites_RenderSpriteCard(void* rcx, void* meshBuilder, void* pCtx, SpriteRenderInfo_t& info, int hParticle, ParticleRenderData_t* pSortList, void* pCamera)
{
	UNLOAD_RETURN(COPRenderSprites_RenderSpriteCard, void, rcx, meshBuilder, pCtx, std::ref(info), hParticle, pSortList, pCamera);

	if (!(Vars::Visuals::World::Modulations.Value & Vars::Visuals::World::ModulationsEnum::Particle)
		|| Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return CALL_ORIGINAL(COPRenderSprites_RenderSpriteCard, void, rcx, meshBuilder, pCtx, std::ref(info), hParticle, pSortList, pCamera);

	Color_t color = Vars::Colors::ParticleModulation.Value;
	switch (Vars::Visuals::World::ParticleModulationStyle.Value)
	{
	case Vars::Visuals::World::ParticleModulationStyleEnum::GroupColored:
		if (pOwnerEnt)
		{
			if (Group_t* pGroup{}; F::Groups.GetGroup(pOwnerEnt, pGroup, false))
				color = F::Groups.GetColor(pOwnerEnt, pGroup);
			else
				return CALL_ORIGINAL(COPRenderSprites_RenderSpriteCard, void, rcx, meshBuilder, pCtx, std::ref(info), hParticle, pSortList, pCamera);
		}
		break;
	case Vars::Visuals::World::ParticleModulationStyleEnum::Rainbow:
		color = H::Draw.Rainbow().Alpha(color.a);
	}

	if (!color.a)
		return CALL_ORIGINAL(COPRenderSprites_RenderSpriteCard, void, rcx, meshBuilder, pCtx, std::ref(info), hParticle, pSortList, pCamera);

	info.m_pRGB[((hParticle / 4) * info.m_nRGBStride) + 0].m128_f32[hParticle & 0x3] = color.r / 255.f;
	info.m_pRGB[((hParticle / 4) * info.m_nRGBStride) + 1].m128_f32[hParticle & 0x3] = color.g / 255.f;
	info.m_pRGB[((hParticle / 4) * info.m_nRGBStride) + 2].m128_f32[hParticle & 0x3] = color.b / 255.f;
	if (color.a != 255)
		pSortList->m_nAlpha = color.a;
	CALL_ORIGINAL(COPRenderSprites_RenderSpriteCard, void, rcx, meshBuilder, pCtx, std::ref(info), hParticle, pSortList, pCamera);
}

void __fastcall COPRenderSprites_RenderTwoSequenceSpriteCard(void* rcx, void* meshBuilder, void* pCtx, SpriteRenderInfo_t& info, int hParticle, ParticleRenderData_t* pSortList, void* pCamera)
{
	UNLOAD_RETURN(COPRenderSprites_RenderTwoSequenceSpriteCard, void, rcx, meshBuilder, pCtx, std::ref(info), hParticle, pSortList, pCamera);

	if (!(Vars::Visuals::World::Modulations.Value & Vars::Visuals::World::ModulationsEnum::Particle)
		|| Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return CALL_ORIGINAL(COPRenderSprites_RenderTwoSequenceSpriteCard, void, rcx, meshBuilder, pCtx, std::ref(info), hParticle, pSortList, pCamera);

	Color_t color = Vars::Colors::ParticleModulation.Value;
	switch (Vars::Visuals::World::ParticleModulationStyle.Value)
	{
	case Vars::Visuals::World::ParticleModulationStyleEnum::GroupColored:
		if (pOwnerEnt)
		{
			if (Group_t* pGroup{}; F::Groups.GetGroup(pOwnerEnt, pGroup, false))
				color = F::Groups.GetColor(pOwnerEnt, pGroup);
			else
				return CALL_ORIGINAL(COPRenderSprites_RenderTwoSequenceSpriteCard, void, rcx, meshBuilder, pCtx, std::ref(info), hParticle, pSortList, pCamera);
		}
		break;
	case Vars::Visuals::World::ParticleModulationStyleEnum::Rainbow:
		color = H::Draw.Rainbow().Alpha(color.a);
	}

	if (!color.a)
		return CALL_ORIGINAL(COPRenderSprites_RenderTwoSequenceSpriteCard, void, rcx, meshBuilder, pCtx, std::ref(info), hParticle, pSortList, pCamera);

	info.m_pRGB[((hParticle / 4) * info.m_nRGBStride) + 0].m128_f32[hParticle & 0x3] = color.r / 255.f;
	info.m_pRGB[((hParticle / 4) * info.m_nRGBStride) + 1].m128_f32[hParticle & 0x3] = color.g / 255.f;
	info.m_pRGB[((hParticle / 4) * info.m_nRGBStride) + 2].m128_f32[hParticle & 0x3] = color.b / 255.f;
	if (color.a != 255)
		pSortList->m_nAlpha = Vars::Colors::ParticleModulation.Value.a;
	CALL_ORIGINAL(COPRenderSprites_RenderTwoSequenceSpriteCard, void, rcx, meshBuilder, pCtx, std::ref(info), hParticle, pSortList, pCamera);
}

void __fastcall CParticleCollection_Render(void* rcx, IMatRenderContext* pRenderContext, bool bTranslucentOnly, void* pCameraObject)
{
	UNLOAD_RETURN(CParticleCollection_Render, void, rcx, pRenderContext, bTranslucentOnly, pCameraObject);

	auto pParticleCollection = reinterpret_cast<CParticleCollection*>(rcx);
	if (!pParticleCollection || !pOwnerEnt)
		return CALL_ORIGINAL(CParticleCollection_Render, void, rcx, pRenderContext, bTranslucentOnly, pCameraObject);

	auto& pDef = pParticleCollection->m_pDef; // some hud shit can cause crashes for no reason
	if (pDef && FNV1A::Hash32(pDef->m_Name.m_pString) != FNV1A::Hash32Const("laser_sight_beam"))
		return CALL_ORIGINAL(CParticleCollection_Render, void, rcx, pRenderContext, bTranslucentOnly, pCameraObject);

	// wrangler
	if (pOwnerEnt->IsPlayer() && pOwnerEnt->As<CTFPlayer>()->m_iClass() != TF_CLASS_SNIPER)
		return CALL_ORIGINAL(CParticleCollection_Render, void, rcx, pRenderContext, bTranslucentOnly, pCameraObject);

	Group_t* pGroup{};
	bool bFoundGroup = F::Groups.GetGroup(pOwnerEnt, pGroup, false);
	if (!bFoundGroup || bFoundGroup && !pGroup->m_bSightlinesIgnoreZ)
		return CALL_ORIGINAL(CParticleCollection_Render, void, rcx, pRenderContext, bTranslucentOnly, pCameraObject);

	pRenderContext->DepthRange(0.f, 0.2f);
	CALL_ORIGINAL(CParticleCollection_Render, void, rcx, pRenderContext, bTranslucentOnly, pCameraObject);
	pRenderContext->DepthRange(0.f, 1.f);
}

void* __fastcall CParticleProperty_Create_Name(void* rcx, const char* pszParticleName, ParticleAttachment_t iAttachType, const char* pszAttachmentName)
{
	UNLOAD_RETURN(CParticleProperty_Create_Name, void*, rcx, pszParticleName, iAttachType, pszAttachmentName);

	const auto dwRetAddr = uintptr_t(_ReturnAddress());
	const auto dwUpdateEffects1 = S::CWeaponMedigun_UpdateEffects_CreateName_Call1();
	const auto dwUpdateEffects2 = S::CWeaponMedigun_UpdateEffects_CreateName_Call2();
	const auto dwManageChargeEffect = S::CWeaponMedigun_ManageChargeEffect_CreateName_Call();

	bool bUpdateEffects = dwRetAddr == dwUpdateEffects1 || dwRetAddr == dwUpdateEffects2, bManageChargeEffect = dwRetAddr == dwManageChargeEffect;
	if (bUpdateEffects || bManageChargeEffect)
	{
		auto pLocal = H::Entities.GetLocal();
		if (!pLocal)
			return CALL_ORIGINAL(CParticleProperty_Create_Name, void*, rcx, pszParticleName, iAttachType, pszAttachmentName);

		/* // probably not needed
		auto pWeapon = pLocal->GetWeaponFromSlot(SLOT_SECONDARY);
		if (!pWeapon || pWeapon->GetWeaponID() != TF_WEAPON_MEDIGUN)
			return CALL_ORIGINAL(rcx, pszParticleName, iAttachType, pszAttachmentName);
		*/

		auto pModel = pLocal->GetRenderedWeaponModel();
		if (!pModel || rcx != pModel->m_Particles())
			return CALL_ORIGINAL(CParticleProperty_Create_Name, void*, rcx, pszParticleName, iAttachType, pszAttachmentName);

		bool bBlue = pLocal->m_iTeamNum() == TF_TEAM_BLUE;
		if (bUpdateEffects)
		{
			switch (FNV1A::Hash32(Vars::Visuals::Effects::MedigunBeam.Value.c_str()))
			{
			case FNV1A::Hash32Const("Default"): break;
			case FNV1A::Hash32Const("None"): return nullptr;
			case FNV1A::Hash32Const("Uber"): pszParticleName = bBlue ? "medicgun_beam_blue_invun" : "medicgun_beam_red_invun"; break;
			case FNV1A::Hash32Const("Dispenser"): pszParticleName = bBlue ? "dispenser_heal_blue" : "dispenser_heal_red"; break;
			case FNV1A::Hash32Const("Passtime"): pszParticleName = "passtime_beam"; break;
			case FNV1A::Hash32Const("Bombonomicon"): pszParticleName = "bombonomicon_spell_trail"; break;
			case FNV1A::Hash32Const("White"): pszParticleName = "medicgun_beam_machinery_stage3"; break;
			case FNV1A::Hash32Const("Orange"): pszParticleName = "medicgun_beam_red_trail_stage3"; break;
			default: pszParticleName = Vars::Visuals::Effects::MedigunBeam.Value.c_str();
			}
		}
		else if (bManageChargeEffect)
		{
			switch (FNV1A::Hash32(Vars::Visuals::Effects::MedigunCharge.Value.c_str()))
			{
			case FNV1A::Hash32Const("Default"): break;
			case FNV1A::Hash32Const("None"): return nullptr;
			case FNV1A::Hash32Const("Electrocuted"): pszParticleName = bBlue ? "electrocuted_blue" : "electrocuted_red"; break;
			case FNV1A::Hash32Const("Halloween"): pszParticleName = "ghost_pumpkin"; break;
			case FNV1A::Hash32Const("Fireball"): pszParticleName = bBlue ? "spell_fireball_small_trail_blue" : "spell_fireball_small_trail_red"; break;
			case FNV1A::Hash32Const("Teleport"): pszParticleName = bBlue ? "spell_teleport_blue" : "spell_teleport_red"; break;
			case FNV1A::Hash32Const("Burning"): pszParticleName = "superrare_burning1"; break;
			case FNV1A::Hash32Const("Scorching"): pszParticleName = "superrare_burning2"; break;
			case FNV1A::Hash32Const("Purple energy"): pszParticleName = "superrare_purpleenergy"; break;
			case FNV1A::Hash32Const("Green energy"): pszParticleName = "superrare_greenenergy"; break;
			case FNV1A::Hash32Const("Nebula"): pszParticleName = "unusual_invasion_nebula"; break;
			case FNV1A::Hash32Const("Purple stars"): pszParticleName = "unusual_star_purple_parent"; break;
			case FNV1A::Hash32Const("Green stars"): pszParticleName = "unusual_star_green_parent"; break;
			case FNV1A::Hash32Const("Sunbeams"): pszParticleName = "superrare_beams1"; break;
			case FNV1A::Hash32Const("Spellbound"): pszParticleName = "unusual_spellbook_circle_purple"; break;
			case FNV1A::Hash32Const("Purple sparks"): pszParticleName = "unusual_robot_orbiting_sparks2"; break;
			case FNV1A::Hash32Const("Yellow sparks"): pszParticleName = "unusual_robot_orbiting_sparks"; break;
			case FNV1A::Hash32Const("Green zap"): pszParticleName = "unusual_zap_green"; break;
			case FNV1A::Hash32Const("Yellow zap"): pszParticleName = "unusual_zap_yellow"; break;
			case FNV1A::Hash32Const("Plasma"): pszParticleName = "superrare_plasma1"; break;
			case FNV1A::Hash32Const("Frostbite"): pszParticleName = "unusual_eotl_frostbite"; break;
			case FNV1A::Hash32Const("Time warp"): pszParticleName = bBlue ? "unusual_robot_time_warp2" : "unusual_robot_time_warp"; break;
			case FNV1A::Hash32Const("Purple souls"): pszParticleName = "unusual_souls_purple_parent"; break;
			case FNV1A::Hash32Const("Green souls"): pszParticleName = "unusual_souls_green_parent"; break;
			case FNV1A::Hash32Const("Bubbles"): pszParticleName = "unusual_bubbles"; break;
			case FNV1A::Hash32Const("Hearts"): pszParticleName = "unusual_hearts_bubbling"; break;
			default: pszParticleName = Vars::Visuals::Effects::MedigunCharge.Value.c_str();
			}
		}
	}

	return CALL_ORIGINAL(CParticleProperty_Create_Name, void*, rcx, pszParticleName, iAttachType, pszAttachmentName);
}

void* CParticleProperty_Create_Point(void* rcx, const char* pszParticleName, ParticleAttachment_t iAttachType, int iAttachmentPoint, Vector vecOriginOffset)
{
	UNLOAD_RETURN(CParticleProperty_Create_Point, void*, rcx, pszParticleName, iAttachType, iAttachmentPoint, vecOriginOffset);

	if (pszParticleName)
	{
		switch (FNV1A::Hash32(pszParticleName))
		{
		case FNV1A::Hash32Const("kart_impact_sparks"):
			if (I::Prediction->InPrediction() && !I::Prediction->m_bFirstTimePredicted)
				return nullptr;
		}
	}

	if (FNV1A::Hash32(Vars::Visuals::Effects::ProjectileTrail.Value.c_str()) != FNV1A::Hash32Const("Default") && pszParticleName)
	{
		switch (FNV1A::Hash32(pszParticleName))
		{
			// any trails we want to replace
		case FNV1A::Hash32Const("peejar_trail_blu"):
		case FNV1A::Hash32Const("peejar_trail_red"):
		case FNV1A::Hash32Const("peejar_trail_blu_glow"):
		case FNV1A::Hash32Const("peejar_trail_red_glow"):
		case FNV1A::Hash32Const("stunballtrail_blue"):
		case FNV1A::Hash32Const("stunballtrail_red"):
		case FNV1A::Hash32Const("rockettrail"):
		case FNV1A::Hash32Const("rockettrail_airstrike"):
		case FNV1A::Hash32Const("drg_cow_rockettrail_normal_blue"):
		case FNV1A::Hash32Const("drg_cow_rockettrail_normal"):
		case FNV1A::Hash32Const("drg_cow_rockettrail_charged_blue"):
		case FNV1A::Hash32Const("drg_cow_rockettrail_charged"):
		case FNV1A::Hash32Const("rockettrail_RocketJumper"):
		case FNV1A::Hash32Const("rockettrail_underwater"):
		case FNV1A::Hash32Const("halloween_rockettrail"):
		case FNV1A::Hash32Const("eyeboss_projectile"):
		case FNV1A::Hash32Const("drg_bison_projectile"):
		case FNV1A::Hash32Const("flaregun_trail_blue"):
		case FNV1A::Hash32Const("flaregun_trail_red"):
		case FNV1A::Hash32Const("scorchshot_trail_blue"):
		case FNV1A::Hash32Const("scorchshot_trail_red"):
		case FNV1A::Hash32Const("drg_manmelter_projectile"):
		case FNV1A::Hash32Const("pipebombtrail_blue"):
		case FNV1A::Hash32Const("pipebombtrail_red"):
		case FNV1A::Hash32Const("stickybombtrail_blue"):
		case FNV1A::Hash32Const("stickybombtrail_red"):
		case FNV1A::Hash32Const("healshot_trail_blue"):
		case FNV1A::Hash32Const("healshot_trail_red"):
		case FNV1A::Hash32Const("flaming_arrow"):
		case FNV1A::Hash32Const("spell_fireball_small_trail_blue"):
		case FNV1A::Hash32Const("spell_fireball_small_trail_red"):
		{
			auto pLocal = H::Entities.GetLocal();
			if (!pLocal)
				return CALL_ORIGINAL(CParticleProperty_Create_Point, void*, rcx, pszParticleName, iAttachType, iAttachmentPoint, vecOriginOffset);

			bool bValid = false;
			for (auto pEntity : H::Entities.GetGroup(EntityEnum::WorldProjectile))
			{
				auto pOwner = F::ProjSim.GetEntities(pEntity).second;
				if (bValid = pLocal == pOwner && rcx == pEntity->m_Particles())
					break;
			}
			if (!bValid)
				return CALL_ORIGINAL(CParticleProperty_Create_Point, void*, rcx, pszParticleName, iAttachType, iAttachmentPoint, vecOriginOffset);

			bool bBlue = pLocal->m_iTeamNum() == TF_TEAM_BLUE;
			switch (FNV1A::Hash32(Vars::Visuals::Effects::ProjectileTrail.Value.c_str()))
			{
			case FNV1A::Hash32Const("None"): return nullptr;
			case FNV1A::Hash32Const("Rocket"): pszParticleName = "rockettrail"; break;
			case FNV1A::Hash32Const("Critical"): pszParticleName = bBlue ? "critical_rocket_blue" : "critical_rocket_red"; break;
			case FNV1A::Hash32Const("Energy"): pszParticleName = bBlue ? "drg_cow_rockettrail_normal_blue" : "drg_cow_rockettrail_normal"; break;
			case FNV1A::Hash32Const("Charged"): pszParticleName = bBlue ? "drg_cow_rockettrail_charged_blue" : "drg_cow_rockettrail_charged"; break;
			case FNV1A::Hash32Const("Ray"): pszParticleName = "drg_manmelter_projectile"; break;
			case FNV1A::Hash32Const("Fireball"): pszParticleName = bBlue ? "spell_fireball_small_trail_blue" : "spell_fireball_small_trail_red"; break;
			case FNV1A::Hash32Const("Teleport"): pszParticleName = bBlue ? "spell_teleport_blue" : "spell_teleport_red"; break;
			case FNV1A::Hash32Const("Fire"): pszParticleName = "flamethrower"; break;
			case FNV1A::Hash32Const("Flame"): pszParticleName = "flying_flaming_arrow"; break;
			case FNV1A::Hash32Const("Sparks"): pszParticleName = bBlue ? "critical_rocket_bluesparks" : "critical_rocket_redsparks"; break;
			case FNV1A::Hash32Const("Flare"): pszParticleName = bBlue ? "flaregun_trail_blue" : "flaregun_trail_red"; break;
			case FNV1A::Hash32Const("Trail"): pszParticleName = bBlue ? "stickybombtrail_blue" : "stickybombtrail_red"; break;
			case FNV1A::Hash32Const("Health"): pszParticleName = bBlue ? "healshot_trail_blue" : "healshot_trail_red"; break;
			case FNV1A::Hash32Const("Smoke"): pszParticleName = "rockettrail_airstrike_line"; break;
			case FNV1A::Hash32Const("Bubbles"): pszParticleName = bBlue ? "pyrovision_scorchshot_trail_blue" : "pyrovision_scorchshot_trail_red"; break;
			case FNV1A::Hash32Const("Halloween"): pszParticleName = "halloween_rockettrail"; break;
			case FNV1A::Hash32Const("Monoculus"): pszParticleName = "eyeboss_projectile"; break;
			case FNV1A::Hash32Const("Sparkles"): pszParticleName = bBlue ? "burningplayer_rainbow_blue" : "burningplayer_rainbow_red"; break;
			case FNV1A::Hash32Const("Rainbow"): pszParticleName = "flamethrower_rainbow"; break;
			default: pszParticleName = Vars::Visuals::Effects::ProjectileTrail.Value.c_str();
			}
			break;
		}
		/*
		// any additional trails
		case FNV1A::Hash32Const("stunballtrail_blue_crit"):
		case FNV1A::Hash32Const("stunballtrail_red_crit"):
		case FNV1A::Hash32Const("critical_rocket_blue"):
		case FNV1A::Hash32Const("critical_rocket_red"):
		case FNV1A::Hash32Const("critical_rocket_bluesparks"):
		case FNV1A::Hash32Const("critical_rocket_redsparks"):
		case FNV1A::Hash32Const("flaregun_trail_crit_blue"):
		case FNV1A::Hash32Const("flaregun_trail_crit_red"):
		case FNV1A::Hash32Const("critical_pipe_blue"):
		case FNV1A::Hash32Const("critical_pipe_red"):
		case FNV1A::Hash32Const("critical_grenade_blue"):
		case FNV1A::Hash32Const("critical_grenade_red"):
		*/
		case FNV1A::Hash32Const("rockettrail_airstrike_line"):
			return nullptr;
		}
	}

	return CALL_ORIGINAL(CParticleProperty_Create_Point, void*, rcx, pszParticleName, iAttachType, iAttachmentPoint, vecOriginOffset);
}

void __fastcall CParticleProperty_AddControlPoint_Pointer(void* rcx, void* pEffect, int iPoint, CBaseEntity* pEntity, ParticleAttachment_t iAttachType, const char* pszAttachmentName, Vector vecOriginOffset)
{
	if (!pEffect)
		return;

	CALL_ORIGINAL(CParticleProperty_AddControlPoint_Pointer, void, rcx, pEffect, iPoint, pEntity, iAttachType, pszAttachmentName, vecOriginOffset);
}

/*
// physics_debug_entity !picker
void __fastcall CPhysicsObject_OutputDebugInfo(void* rcx)
{
	//CALL_ORIGINAL(CPhysicsObject_OutputDebugInfo, void, rcx);

	Vec3 speed, angSpeed;
	reinterpret_cast<IPhysicsObject*>(rcx)->GetVelocity(&speed, &angSpeed);
	SDK::Output("Velocity", std::format("{}, {}, {} ({})", speed.x, speed.y, speed.z, speed.Length()).c_str());
	SDK::Output("Ang Velocity", std::format("{}, {}, {} ({})", angSpeed.x, angSpeed.y, angSpeed.z, angSpeed.Length()).c_str());

	SDK::Output("Linear drag", std::format("{:.6f}, {:.6f}, {:.6f} ({})", *reinterpret_cast<float*>(uintptr_t(rcx) + 10i64 * 4), *reinterpret_cast<float*>(uintptr_t(rcx) + 11i64 * 4), *reinterpret_cast<float*>(uintptr_t(rcx) + 12i64 * 4), *reinterpret_cast<float*>(uintptr_t(rcx) + 22i64 * 4)).c_str());
	SDK::Output("Angular drag", std::format("{:.6f}, {:.6f}, {:.6f} ({})", *reinterpret_cast<float*>(uintptr_t(rcx) + 13i64 * 4), *reinterpret_cast<float*>(uintptr_t(rcx) + 14i64 * 4), *reinterpret_cast<float*>(uintptr_t(rcx) + 15i64 * 4), *reinterpret_cast<float*>(uintptr_t(rcx) + 23i64 * 4)).c_str());
}
*/

const char* CPlayerResource_GetPlayerName(void* rcx, int iIndex)
{
	UNLOAD_RETURN(CPlayerResource_GetPlayerName, const char*, rcx, iIndex);

	return F::PlayerUtils.GetPlayerName(iIndex, CALL_ORIGINAL(CPlayerResource_GetPlayerName, const char*, rcx, iIndex));
}

#ifdef ANTIAUTOBALANCETESTING
void __fastcall CPrediction_PostEntityPacketReceived()
{
	UNLOAD_RETURN(CPrediction_PostEntityPacketReceived, void);

	CALL_ORIGINAL(CPrediction_PostEntityPacketReceived, void);
	F::AntiAutobalance.Run();
}
#endif

static std::vector<TickbaseFix_t> s_vTickbaseFixes = {};
void __fastcall CPrediction_RunSimulation(void* rcx, int current_command, float curtime, CUserCmd* cmd, CTFPlayer* localPlayer)
{
	UNLOAD_RETURN(CPrediction_RunSimulation, void, rcx, current_command, curtime, cmd, localPlayer);

	if (F::Ticks.m_bShifting && F::Ticks.m_iShiftedTicks + 1 == F::Ticks.m_iShiftStart)
	{
		s_vTickbaseFixes.emplace_back(G::CurrentUserCmd, I::ClientState->lastoutgoingcommand, F::Ticks.m_iShiftStart - F::Ticks.m_iShiftedGoal);
		F::Ticks.m_bShifting = false;
	}

	for (auto it = s_vTickbaseFixes.begin(); it != s_vTickbaseFixes.end();)
	{
		if (it->m_iLastOutgoingCommand < I::ClientState->last_command_ack)
		{
			it = s_vTickbaseFixes.erase(it);
			continue;
		}
		if (cmd == it->m_pCmd)
		{
			localPlayer->m_nTickBase() -= it->m_iTickbaseShift;
			break;
		}
		++it;
	}

	F::EnginePrediction.AdjustPlayers(localPlayer);
	CALL_ORIGINAL(CPrediction_RunSimulation, void, rcx, current_command, curtime, cmd, localPlayer);
	F::EnginePrediction.RestorePlayers();
}


#pragma warning(push)
#pragma warning(disable: 4305) // we are doing it purposefully here

void __fastcall CProxyAnimatedWeaponSheen_OnBind(CProxyAnimatedWeaponSheen* rcx, void* pEntity)
{
	UNLOAD_RETURN(CProxyAnimatedWeaponSheen_OnBind, void, rcx, pEntity);

	CALL_ORIGINAL(CProxyAnimatedWeaponSheen_OnBind, void, rcx, pEntity);

	if (!(Vars::Visuals::World::Modulations.Value & Vars::Visuals::World::ModulationsEnum::KillstreakSheen)
		|| Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot() || !rcx)
		return;

	IMaterialVar* m_pTintVar = rcx->m_pTintVar;
	if (!m_pTintVar)
		return;

	const float* color = m_pTintVar->GetVecValueInternal();
	if (!color || !color[3])
		return;

	const Color_t tColor = Vars::Colors::SheenModulation.Value;
	float flColors[4] = { tColor.r / 255.f, tColor.g / 255.f, tColor.b / 255.f, tColor.a / 255.f };
	m_pTintVar->SetVecValue(flColors, 4);
}

#pragma warning(pop)

void __fastcall CRendering3dView_EnableWorldFog()
{
	UNLOAD_RETURN(CRendering3dView_EnableWorldFog, void);

	if (!(Vars::Visuals::World::Modulations.Value & Vars::Visuals::World::ModulationsEnum::Fog) || I::EngineClient->IsTakingScreenshot() && Vars::Visuals::UI::CleanScreenshots.Value)
		return CALL_ORIGINAL(CRendering3dView_EnableWorldFog, void);

	CALL_ORIGINAL(CRendering3dView_EnableWorldFog, void);
	if (auto pRenderContext = I::MaterialSystem->GetRenderContext())
	{
		if (Vars::Colors::FogModulation.Value.a)
		{
			pRenderContext->FogColor3ub(Vars::Colors::FogModulation.Value.r, Vars::Colors::FogModulation.Value.g, Vars::Colors::FogModulation.Value.b);

			float flRatio = 255.f / Vars::Colors::FogModulation.Value.a;
			float flStart, flEnd; pRenderContext->GetFogDistances(&flStart, &flEnd, nullptr);


			pRenderContext->FogStart(flStart * flRatio);
			pRenderContext->FogEnd(flEnd * flRatio);
		}
		else
			pRenderContext->FogMode(MATERIAL_FOG_NONE);
	}
}

void __fastcall CSequenceTransitioner_CheckForSequenceChange(void* rcx, CStudioHdr* hdr, int nCurSequence, bool bForceNewSequence, bool bInterpolate)
{
	UNLOAD_RETURN(CSequenceTransitioner_CheckForSequenceChange, void, rcx, hdr, nCurSequence, bForceNewSequence, bInterpolate);

	if (Vars::Misc::Game::AccuracyImprovements.Value)
		bInterpolate = false;

	CALL_ORIGINAL(CSequenceTransitioner_CheckForSequenceChange, void, rcx, hdr, nCurSequence, bForceNewSequence, bInterpolate);
}

void __fastcall CSkyboxView_Enable3dSkyboxFog(void* rcx)
{
	UNLOAD_RETURN(CSkyboxView_Enable3dSkyboxFog, void, rcx);

	if (!(Vars::Visuals::World::Modulations.Value & Vars::Visuals::World::ModulationsEnum::Fog) || I::EngineClient->IsTakingScreenshot() && Vars::Visuals::UI::CleanScreenshots.Value)
		return CALL_ORIGINAL(CSkyboxView_Enable3dSkyboxFog, void, rcx);

	CALL_ORIGINAL(CSkyboxView_Enable3dSkyboxFog, void, rcx);
	if (auto pRenderContext = I::MaterialSystem->GetRenderContext())
	{
		if (Vars::Colors::FogModulation.Value.a)
		{
			pRenderContext->FogColor3ub(Vars::Colors::FogModulation.Value.r, Vars::Colors::FogModulation.Value.g, Vars::Colors::FogModulation.Value.b);

			float flRatio = 255.f / Vars::Colors::FogModulation.Value.a;
			float flStart, flEnd; pRenderContext->GetFogDistances(&flStart, &flEnd, nullptr);


			pRenderContext->FogStart(flStart * flRatio);
			pRenderContext->FogEnd(flEnd * flRatio);
		}
		else
			pRenderContext->FogMode(MATERIAL_FOG_NONE);
	}
}

void __fastcall CSniperDot_ClientThink(void* rcx)
{
	UNLOAD_RETURN(CSniperDot_ClientThink, void, rcx);

	const auto pGameRules = I::TFGameRules();
	if (!pGameRules)
		return;

	const auto pDotEntity = (CBaseEntity*)((uintptr_t)rcx - 24);
	if (!F::Groups.GroupsActive() || !pDotEntity)
		return CALL_ORIGINAL(CSniperDot_ClientThink, void, rcx);

	const auto pOwner = pDotEntity->m_hOwnerEntity()->As<CTFPlayer>();
	if (!pOwner || !pOwner->IsAlive() || pOwner->m_iClass() != TF_CLASS_SNIPER)
		return CALL_ORIGINAL(CSniperDot_ClientThink, void, rcx);

	const auto pLocal = H::Entities.GetLocal();
	if (!pLocal || pOwner == pLocal)
		return CALL_ORIGINAL(CSniperDot_ClientThink, void, rcx);

	if (pOwner == pLocal->m_hObserverTarget().Get() && pLocal->m_iObserverMode() == OBS_MODE_FIRSTPERSON)
		return CALL_ORIGINAL(CSniperDot_ClientThink, void, rcx);

	Group_t* pGroup;
	if (!F::Groups.GetGroup(pOwner, pLocal, pGroup, false) || !pGroup->m_bSightlines)
		return CALL_ORIGINAL(CSniperDot_ClientThink, void, rcx);

	const bool bOldMvM = pGameRules->m_bPlayingMannVsMachine();
	const int iOldTeamNum = pOwner->m_iTeamNum();

	pGameRules->m_bPlayingMannVsMachine() = true;
	pOwner->m_iTeamNum() = TF_TEAM_PVE_INVADERS;

	CALL_ORIGINAL(CSniperDot_ClientThink, void, rcx);

	pGameRules->m_bPlayingMannVsMachine() = bOldMvM;
	pOwner->m_iTeamNum() = iOldTeamNum;

	const uintptr_t dwLaserBeamEffect = *reinterpret_cast<uintptr_t*>((uintptr_t)rcx + 1984);
	if (dwLaserBeamEffect)
	{
		// fix up the origin for crouching because it appears like they are standing often
		Vec3 vOrigin = pOwner->m_vecOrigin() + Vec3(0.f, 0.f, ((pOwner->IsDucking() ? 45.f : 75.f) * pOwner->m_flModelScale()));
		S::CNewParticleEffect_SetControlPoint.Call<void>(dwLaserBeamEffect, 1, vOrigin);

		Color_t tColor = F::Groups.GetColor(pOwner, pGroup);
		S::CNewParticleEffect_SetControlPoint.Call<void>(dwLaserBeamEffect, 2, Vec3((float)tColor.r, (float)tColor.g, (float)tColor.b));
	}
}

static Vec3 s_vEyePosition;
static Vec3 s_vEyeAngles;
bool __fastcall CSniperDot_GetRenderingPositions(void* rcx, CTFPlayer* pPlayer, Vec3& vecAttachment, Vec3& vecEndPos, float& flSize)
{
	UNLOAD_RETURN(CSniperDot_GetRenderingPositions, bool, rcx, pPlayer, std::ref(vecAttachment), std::ref(vecEndPos), std::ref(flSize));

	if (pPlayer && pPlayer->entindex() != I::EngineClient->GetLocalPlayer())
	{
		auto pDot = reinterpret_cast<CSniperDot*>(rcx);

		s_vEyePosition = pPlayer->m_vecOrigin() + pPlayer->GetViewOffset();
		s_vEyeAngles = Math::VectorAngles(pDot->GetAbsOrigin() - s_vEyePosition);
	}

	return CALL_ORIGINAL(CSniperDot_GetRenderingPositions, bool, rcx, pPlayer, std::ref(vecAttachment), std::ref(vecEndPos), std::ref(flSize));
}

Vec3* __fastcall CBasePlayer_EyePosition(void* rcx, void* rdx)
{
	UNLOAD_RETURN(CBasePlayer_EyePosition, Vec3*, rcx, rdx);

	const auto dwRetAddr = uintptr_t(_ReturnAddress());
	const auto dwDesired = S::CSniperDot_GetRenderingPositions_EyePosition_Call();

	if (dwRetAddr == dwDesired)
		return &s_vEyePosition;

	return CALL_ORIGINAL(CBasePlayer_EyePosition, Vec3*, rcx, rdx);
}

Vec3* __fastcall CTFPlayer_EyeAngles(void* rcx)
{
	UNLOAD_RETURN(CTFPlayer_EyeAngles, Vec3*, rcx);

	const auto dwRetAddr = uintptr_t(_ReturnAddress());
	const auto dwDesired = S::CSniperDot_GetRenderingPositions_EyeAngles_Call();

	if (dwRetAddr == dwDesired)
		return &s_vEyeAngles;

	return CALL_ORIGINAL(CTFPlayer_EyeAngles, Vec3*, rcx);
}

void __fastcall CSoundEmitterSystem_EmitSound(void* rcx, IRecipientFilter& filter, int entindex, const EmitSound_t& ep)
{
	UNLOAD_RETURN(CSoundEmitterSystem_EmitSound, void, rcx, std::ref(filter), entindex, std::ref(ep));

	if (ShouldBlockSound(ep.m_pSoundName))
		return;

	return CALL_ORIGINAL(CSoundEmitterSystem_EmitSound, void, rcx, std::ref(filter), entindex, std::ref(ep));
}

void __fastcall CBaseEntity_EmitSound(void* rcx, const char* soundname, float soundtime, float* duration)
{
	UNLOAD_RETURN(CBaseEntity_EmitSound, void, rcx, soundname, soundtime, duration);

	if (soundname)
	{
		switch (FNV1A::Hash32(soundname))
		{
		case FNV1A::Hash32Const("BumperCar.Jump"):
		case FNV1A::Hash32Const("BumperCar.JumpLand"):
		case FNV1A::Hash32Const("BumperCar.Bump"):
		case FNV1A::Hash32Const("BumperCar.BumpHard"):
			if (I::Prediction->InPrediction() && !I::Prediction->m_bFirstTimePredicted)
				return;
		}
	}

	CALL_ORIGINAL(CBaseEntity_EmitSound, void, rcx, soundname, soundtime, duration);
}

/*
int __fastcall S_StartDynamicSound(StartSoundParams_t& params)
{
	UNLOAD_RETURN(S_StartDynamicSound, int, params);
	
	H::Entities.ManualNetwork(params);
	if (params.pSfx && ShouldBlockSound(params.pSfx->getname()))
		return 0;

	return CALL_ORIGINAL(S_StartDynamicSound, int, params);
}
*/

int __fastcall S_StartSound(StartSoundParams_t& params)
{
	UNLOAD_RETURN(S_StartSound, int, std::ref(params));

	if (!params.staticsound)
	{
		H::Entities.ManualNetwork(params);

		int iIndex = params.soundsource;
		if (!F::ESP.m_mSoundCircles.contains(iIndex))
			F::ESP.m_mSoundCircles[iIndex] = 0.f; // init circle
	}

	if (params.pSfx && ShouldBlockSound(params.pSfx->getname()))
		return 0;

	return CALL_ORIGINAL(S_StartSound, int, std::ref(params));
}
/*
int __fastcall CSpriteTrail_DrawModel(void* rcx, int flags)
{
	UNLOAD_RETURN(CSpriteTrail_DrawModel, int, rcx, flags);

	// clean screenshots wont do anything if we set m_clrRender directly
	if (!(Vars::Visuals::World::Modulations.Value & Vars::Visuals::World::ModulationsEnum::Particle))
		return CALL_ORIGINAL(CSpriteTrail_DrawModel, int, rcx, flags);

	auto pSpriteTrail = reinterpret_cast<CBaseEntity*>(rcx);
	if (!pSpriteTrail)
		return CALL_ORIGINAL(CSpriteTrail_DrawModel, int, rcx, flags);

	switch (Vars::Visuals::World::ParticleModulationStyle.Value)
	{
	case Vars::Visuals::World::ParticleModulationStyleEnum::Rainbow:
	{
		SetColorRender(pSpriteTrail, H::Draw.Rainbow());
		break;
	}
	case Vars::Visuals::World::ParticleModulationStyleEnum::SolidColor:
	{
		SetColorRender(pSpriteTrail, Vars::Colors::ParticleModulation.Value);
		break;
	}
	case Vars::Visuals::World::ParticleModulationStyleEnum::GroupColored:
	{
		auto pOwnerEntity = pSpriteTrail->m_hOwnerEntity().Get();
		if (!pOwnerEntity)
			return CALL_ORIGINAL(CSpriteTrail_DrawModel, int, rcx, flags);

		if (Group_t* pGroup{}; F::Groups.GetGroup(pOwnerEntity, pGroup, false))
			SetColorRender(pSpriteTrail, F::Groups.GetColor(pOwnerEntity, pGroup));
	}
	}

	return CALL_ORIGINAL(CSpriteTrail_DrawModel, int, rcx, flags);
}
*/

void __fastcall CStaticPropMgr_ComputePropOpacity(void* rcx, CStaticProp* pProp)
{
	UNLOAD_RETURN(CStaticPropMgr_ComputePropOpacity, void, rcx, pProp);

	if (Vars::Visuals::World::NoPropFade.Value && pProp)
	{
		pProp->m_Alpha = 255;
		return;
	}

	CALL_ORIGINAL(CStaticPropMgr_ComputePropOpacity, void, rcx, pProp);
}

static bool s_bDrawingProps = false;
void __fastcall CStaticPropMgr_DrawStaticProps(void* rcx, IClientRenderable** pProps, int count, bool bShadowDepth, bool drawVCollideWireframe)
{
	UNLOAD_RETURN(CStaticPropMgr_DrawStaticProps, void, rcx, pProps, count, bShadowDepth, drawVCollideWireframe);

	s_bDrawingProps = true;
	CALL_ORIGINAL(CStaticPropMgr_DrawStaticProps, void, rcx, pProps, count, bShadowDepth, drawVCollideWireframe);
	s_bDrawingProps = false;
}

void __fastcall CStudioRender_SetColorModulation(void* rcx, const float* pColor)
{
	UNLOAD_RETURN(CStudioRender_SetColorModulation, void, rcx, pColor);

	if (!s_bDrawingProps || !(Vars::Visuals::World::Modulations.Value & Vars::Visuals::World::ModulationsEnum::Prop)
		|| Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return CALL_ORIGINAL(CStudioRender_SetColorModulation, void, rcx, pColor);

	float flColor[3] = {
		Vars::Colors::PropModulation.Value.r / 255.f,
		Vars::Colors::PropModulation.Value.g / 255.f,
		Vars::Colors::PropModulation.Value.b / 255.f
	};
	CALL_ORIGINAL(CStudioRender_SetColorModulation, void, rcx, flColor);
}

void __fastcall CStudioRender_SetAlphaModulation(void* rcx, float flAlpha)
{
	UNLOAD_RETURN(CStudioRender_SetAlphaModulation, void, rcx, flAlpha);

	if (!s_bDrawingProps || !(Vars::Visuals::World::Modulations.Value & Vars::Visuals::World::ModulationsEnum::Prop)
		|| Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return CALL_ORIGINAL(CStudioRender_SetAlphaModulation, void, rcx, flAlpha);

	CALL_ORIGINAL(CStudioRender_SetAlphaModulation, void, rcx, Vars::Colors::PropModulation.Value.a / 255.f * flAlpha);
}

/*
void __fastcall CStudioRender_DrawModelStaticProp(void* rcx, const DrawModelState_t& pState, const matrix3x4& modelToWorld, int flags)
{
	UNLOAD_RETURN(CStudioRender_DrawModelStaticProp, void, rcx, std::ref(pState), std::ref(modelToWorld), flags);

	if (Vars::Visuals::World::NearPropFade.Value)
	{
		if (auto pLocal = H::Entities.GetLocal())
		{
			Vec3 vOrigin = { modelToWorld[0][3], modelToWorld[1][3], modelToWorld[2][3] };

			const float flDistance = pLocal->m_vecOrigin().DistTo(vOrigin);

			float flAlpha = 1.f;
			if (flDistance < 300.0f)
				flAlpha = Math::RemapVal(flDistance, 150.0f, 300.0f, 0.15f, 1.f);
			I::StudioRender->SetAlphaModulation(flAlpha);
		}
	}

	CALL_ORIGINAL(CStudioRender_DrawModelStaticProp, void, rcx, std::ref(pState), std::ref(modelToWorld), flags);
}
*/

void __fastcall CTFBadgePanel_SetupBadge(void* rcx, const IMatchGroupDescription* pMatchDesc, /*const*/ LevelInfo_t& levelInfo, const CSteamID& steamID)
{
	UNLOAD_RETURN(CTFBadgePanel_SetupBadge, void, rcx, pMatchDesc, std::ref(levelInfo), std::ref(steamID));

	if (!Vars::Visuals::UI::StreamerMode.Value)
		return CALL_ORIGINAL(CTFBadgePanel_SetupBadge, void, rcx, pMatchDesc, std::ref(levelInfo), std::ref(steamID));

	auto pResource = H::Entities.GetResource();
	if (!pResource)
		return CALL_ORIGINAL(CTFBadgePanel_SetupBadge, void, rcx, pMatchDesc, std::ref(levelInfo), std::ref(steamID));

	uint32_t uAccountID = steamID.GetAccountID();
	// probably only need to worry about local, friends, a/o party
	bool bShouldHide = false;
	if (pResource->m_iAccountID(I::EngineClient->GetLocalPlayer()) == uAccountID)
		bShouldHide = Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::Local;
	else if (H::Entities.IsFriend(uAccountID))
		bShouldHide = Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::Friends;
	else if (H::Entities.InParty(uAccountID))
		bShouldHide = Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::Party;
	if (!bShouldHide)
		return CALL_ORIGINAL(CTFBadgePanel_SetupBadge, void, rcx, pMatchDesc, std::ref(levelInfo), std::ref(steamID));

	int nOldLevelNum = levelInfo.m_nLevelNum;
	levelInfo.m_nLevelNum = 1;
	CALL_ORIGINAL(CTFBadgePanel_SetupBadge, void, rcx, pMatchDesc, std::ref(levelInfo), std::ref(steamID));
	levelInfo.m_nLevelNum = nOldLevelNum;
}

static int s_iPlayerIndex;
void __fastcall CTFClientScoreBoardDialog_UpdatePlayerAvatar(void* rcx, int playerIndex, KeyValues* kv)
{
	UNLOAD_RETURN(CTFClientScoreBoardDialog_UpdatePlayerAvatar, void, rcx, playerIndex, kv);

	s_iPlayerIndex = playerIndex;

	int iType = 0; F::PlayerUtils.GetPlayerName(playerIndex, nullptr, &iType);
	if (iType != 1)
		CALL_ORIGINAL(CTFClientScoreBoardDialog_UpdatePlayerAvatar, void, rcx, playerIndex, kv);
}

void __fastcall CTFMatchSummary_UpdatePlayerAvatar(void* rcx, int playerIndex, KeyValues* kv)
{
	UNLOAD_RETURN(CTFMatchSummary_UpdatePlayerAvatar, void, rcx, playerIndex, kv);

	int iType = 0; F::PlayerUtils.GetPlayerName(playerIndex, nullptr, &iType);
	if (iType != 1)
		CALL_ORIGINAL(CTFMatchSummary_UpdatePlayerAvatar, void, rcx, playerIndex, kv);
}

void __fastcall CTFHudMannVsMachineScoreboard_UpdatePlayerAvatar(void* rcx, int playerIndex, KeyValues* kv)
{
	UNLOAD_RETURN(CTFHudMannVsMachineScoreboard_UpdatePlayerAvatar, void, rcx, playerIndex, kv);

	int iType = 0; F::PlayerUtils.GetPlayerName(playerIndex, nullptr, &iType);
	if (iType != 1)
		CALL_ORIGINAL(CTFHudMannVsMachineScoreboard_UpdatePlayerAvatar, void, rcx, playerIndex, kv);
}

void __fastcall CTFHudMatchStatus_UpdatePlayerAvatar(void* rcx, int playerIndex, KeyValues* kv)
{
	UNLOAD_RETURN(CTFHudMatchStatus_UpdatePlayerAvatar, void, rcx, playerIndex, kv);

	int iType = 0; F::PlayerUtils.GetPlayerName(playerIndex, nullptr, &iType);
	if (iType != 1)
		CALL_ORIGINAL(CTFHudMatchStatus_UpdatePlayerAvatar, void, rcx, playerIndex, kv);
}

void __fastcall SectionedListPanel_SetItemFgColor(void* rcx, int itemID, Color_t color)
{
	UNLOAD_RETURN(SectionedListPanel_SetItemFgColor, void, rcx, itemID, color);

	static const auto dwDesired = S::CTFClientScoreBoardDialog_UpdatePlayerList_SetItemFgColor_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	if (dwDesired == dwRetAddr && Vars::Visuals::UI::ScoreboardColors.Value)
	{
		Color_t tColor = GetScoreboardColor(s_iPlayerIndex);
		if (tColor.a)
		{
			auto pResource = H::Entities.GetResource();
			if (pResource && !pResource->m_bAlive(s_iPlayerIndex))
				tColor = tColor.Lerp({ 127, 127, 127, tColor.a }, 0.5f);

			color = tColor;
		}
	}

	CALL_ORIGINAL(SectionedListPanel_SetItemFgColor, void, rcx, itemID, color);
}

bool __fastcall CTFGCClientSystem_UpdateAssignedLobby(void* rcx)
{
	UNLOAD_RETURN(CTFGCClientSystem_UpdateAssignedLobby, bool, rcx);

	bool bReturn = CALL_ORIGINAL(CTFGCClientSystem_UpdateAssignedLobby, bool, rcx);

	if (rcx && Vars::Misc::Game::F2PChatBypass.Value)
		I::TFGCClientSystem->SetNonPremiumAccount(false);

	return bReturn;
}

void __fastcall CTFInput_ApplyMouse(void* rcx, QAngle& viewangles, CUserCmd* cmd, float mouse_x, float mouse_y)
{
	UNLOAD_RETURN(CTFInput_ApplyMouse, void, rcx, std::ref(viewangles), cmd, mouse_x, mouse_y);

	// we should maybe predict the shield cond for better accuracy

	CALL_ORIGINAL(CTFInput_ApplyMouse, void, rcx, std::ref(viewangles), cmd, mouse_x, mouse_y);

	if (!Vars::Misc::Movement::ShieldTurnRate.Value)
		return;

	auto pLocal = H::Entities.GetLocal();
	auto pCmd = G::CurrentUserCmd;
	if (!pLocal || !pCmd || !pLocal->InCond(TF_COND_SHIELD_CHARGE))
		return;

	float flOriginalFrame = I::GlobalVars->frametime;
	I::GlobalVars->frametime = TICK_INTERVAL;
	float flCap = S::CTFPlayerShared_CalculateChargeCap.Call<float>(pLocal->m_Shared()) * 2.5f;
	I::GlobalVars->frametime = flOriginalFrame;

	float flOldYaw = pCmd->viewangles.y;
	float& flNewYaw = viewangles.y;
	float flDiff = abs(flOldYaw) - abs(flNewYaw);
	if (flDiff > flCap)
	{
		if (flNewYaw > flOldYaw)
			flNewYaw = flOldYaw + flCap;
		else
			flNewYaw = flOldYaw - flCap;
	}
}

float __fastcall CTFInput_CAM_CapYaw(void* rcx, float fVal)
{
	UNLOAD_RETURN(CTFInput_CAM_CapYaw, float, rcx, fVal);

	if (!Vars::Misc::Movement::ShieldTurnRate.Value)
		return CALL_ORIGINAL(CTFInput_CAM_CapYaw, float, rcx, fVal);

	return fVal;
}

void __fastcall CTFPlayer_AvoidPlayers(void* rcx, CUserCmd* pCmd)
{
	UNLOAD_RETURN(CTFPlayer_AvoidPlayers, void, rcx, pCmd);

	if (Vars::Misc::Movement::NoPush.Value)
		return;

	CALL_ORIGINAL(CTFPlayer_AvoidPlayers, void, rcx, pCmd);
}

bool __fastcall CTFPlayer_BRenderAsZombie(void* rcx, bool bWeaponsCheck)
{
	UNLOAD_RETURN(CTFPlayer_BRenderAsZombie, bool, rcx, bWeaponsCheck);

	static const auto dwDesired = S::CTFRagdoll_CreateTFRagdoll_BRenderAsZombie_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	if (Vars::Visuals::Removals::Gibs.Value && dwRetAddr == dwDesired)
		return true;

	return CALL_ORIGINAL(CTFPlayer_BRenderAsZombie, bool, rcx, bWeaponsCheck);
}

void __fastcall CTFPlayer_BuildTransformations(void* rcx, CStudioHdr* hdr, Vector* pos, Quaternion q[], const matrix3x4& cameraTransform, int boneMask, void* boneComputed)
{
	UNLOAD_RETURN(CTFPlayer_BuildTransformations, void, rcx, hdr, pos, q, std::ref(cameraTransform), boneMask, boneComputed);
	
	auto pPlayer = reinterpret_cast<CTFPlayer*>(rcx);
	auto iOriginal = pPlayer->m_fFlags();
	pPlayer->m_fFlags() &= ~FL_DUCKING;

	CALL_ORIGINAL(CTFPlayer_BuildTransformations, void, rcx, hdr, pos, q, std::ref(cameraTransform), boneMask, boneComputed);

	pPlayer->m_fFlags() = iOriginal;
}

void __fastcall CTFPlayer_ClientAdjustVOPitch(void* rcx, int& pitch)
{
	UNLOAD_RETURN(CTFPlayer_ClientAdjustVOPitch, void, rcx, std::ref(pitch));

	if (S::IsLocalPlayerUsingVisionFilterFlags.Call<bool>(TF_VISION_FILTER_PYRO))
		pitch *= Vars::Visuals::Effects::PyrovisionPitch.Value;
	else
		CALL_ORIGINAL(CTFPlayer_ClientAdjustVOPitch, void, rcx, std::ref(pitch));
}

void __fastcall CTFPlayer_DoAnimationEvent(CTFPlayer* rcx, PlayerAnimEvent_t event, int nData)
{
	UNLOAD_RETURN(CTFPlayer_DoAnimationEvent, void, rcx, event, nData);

	if (rcx->entindex() != I::EngineClient->GetLocalPlayer())
		return;

	CALL_ORIGINAL(CTFPlayer_DoAnimationEvent, void, rcx, event, nData);
}

void __fastcall CTFPlayer_FireBullet(void* rcx, CBaseCombatWeapon* pWeapon, const FireBulletsInfo_t& info, bool bDoEffects, int nDamageType, int nCustomDamageType)
{
	UNLOAD_RETURN(CTFPlayer_FireBullet, void, rcx, pWeapon, std::ref(info), bDoEffects, nDamageType, nCustomDamageType);

	auto pLocal = reinterpret_cast<CTFPlayer*>(rcx);
	if (pLocal != H::Entities.GetLocal() || !pWeapon)
		return CALL_ORIGINAL(CTFPlayer_FireBullet, void, rcx, pWeapon, std::ref(info), bDoEffects, nDamageType, nCustomDamageType);

	auto& sString = nDamageType & DMG_CRITICAL ? Vars::Visuals::Effects::CritTracer.Value : Vars::Visuals::Effects::BulletTracer.Value;
	auto uHash = FNV1A::Hash32(sString.c_str());
	if (uHash == FNV1A::Hash32Const("Default"))
		return CALL_ORIGINAL(CTFPlayer_FireBullet, void, rcx, pWeapon, std::ref(info), bDoEffects, nDamageType, nCustomDamageType);
	else if (uHash == FNV1A::Hash32Const("None"))
		return;

	const Vec3 vStart = info.m_vecSrc;
	const Vec3 vEnd = vStart + info.m_vecDirShooting * info.m_flDistance;
	CGameTrace trace = {};
	CTraceFilterHitscan filter = {};
	filter.pSkip = pLocal;
	SDK::Trace(vStart, vEnd, MASK_SHOT | CONTENTS_GRATE, &filter, &trace);

	int iIndex = I::EngineClient->GetLocalPlayer();
	int iTeam = pLocal->m_iTeamNum();
	int iAttachment = pWeapon->LookupAttachment("muzzle");
	pWeapon->GetAttachment(iAttachment, trace.startpos);

	switch (uHash)
	{
	case FNV1A::Hash32Const("Big nasty"):
		H::Particles.ParticleTracer(iTeam == TF_TEAM_RED ? "bullet_bignasty_tracer01_blue" : "bullet_bignasty_tracer01_red", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Distortion trail"):
		H::Particles.ParticleTracer("tfc_sniper_distortion_trail", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Machina"):
		H::Particles.ParticleTracer(iTeam == TF_TEAM_RED ? "dxhr_sniper_rail_red" : "dxhr_sniper_rail_blue", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Sniper rail"):
		H::Particles.ParticleTracer("dxhr_sniper_rail", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Short circuit"):
		H::Particles.ParticleTracer(iTeam == TF_TEAM_RED ? "dxhr_lightningball_hit_zap_red" : "dxhr_lightningball_hit_zap_blue", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("C.A.P.P.E.R"):
		H::Particles.ParticleTracer(iTeam == TF_TEAM_RED ? "bullet_tracer_raygun_red" : "bullet_tracer_raygun_blue", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Merasmus ZAP"):
		H::Particles.ParticleTracer("merasmus_zap", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Merasmus ZAP 2"):
		H::Particles.ParticleTracer("merasmus_zap_beam02", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Black ink"):
		H::Particles.ParticleTracer("merasmus_zap_beam01", trace.startpos, trace.endpos, iIndex, iAttachment, true);
		break;
	case FNV1A::Hash32Const("Line"):
	case FNV1A::Hash32Const("Line ignore Z"):
	{
		float flTime = I::GlobalVars->curtime + Vars::Visuals::Line::DrawDuration.Value;
		for (auto& tLine : G::LineStorage)
		{
			if (flTime != tLine.m_flTime)
			{
				G::LineStorage.clear();
				break;
			}
		}

		if (uHash == FNV1A::Hash32Const("Line"))
			G::LineStorage.emplace_back(std::pair<Vec3, Vec3>(trace.startpos, trace.endpos), flTime, Vars::Colors::Line.Value, true);
		else
			G::LineStorage.emplace_back(std::pair<Vec3, Vec3>(trace.startpos, trace.endpos), flTime, Vars::Colors::LineIgnoreZ.Value);

		break;
	}
	case FNV1A::Hash32Const("Beam"):
	{
		BeamInfo_t beamInfo;
		beamInfo.m_nType = 0;
		beamInfo.m_pszModelName = !Vars::Visuals::Beams::Model.Value.empty() ? Vars::Visuals::Beams::Model.Value.c_str() : "sprites/physbeam.vmt";
		beamInfo.m_nModelIndex = -1; // will be set by CreateBeamPoints if its -1
		beamInfo.m_flHaloScale = 0.0f;
		beamInfo.m_flLife = Vars::Visuals::Beams::Life.Value;
		beamInfo.m_flWidth = Vars::Visuals::Beams::Width.Value;
		beamInfo.m_flEndWidth = Vars::Visuals::Beams::EndWidth.Value;
		beamInfo.m_flFadeLength = Vars::Visuals::Beams::FadeLength.Value;
		beamInfo.m_flAmplitude = Vars::Visuals::Beams::Amplitude.Value;
		beamInfo.m_flBrightness = Vars::Visuals::Beams::Brightness.Value;
		beamInfo.m_flSpeed = Vars::Visuals::Beams::Speed.Value;
		beamInfo.m_nStartFrame = 0;
		beamInfo.m_flFrameRate = 0;
		beamInfo.m_flRed = Vars::Visuals::Beams::Color.Value.r;
		beamInfo.m_flGreen = Vars::Visuals::Beams::Color.Value.g;
		beamInfo.m_flBlue = Vars::Visuals::Beams::Color.Value.b;
		beamInfo.m_nSegments = Vars::Visuals::Beams::Segments.Value;
		beamInfo.m_bRenderable = true;
		beamInfo.m_nFlags = Vars::Visuals::Beams::Flags.Value;
		beamInfo.m_vecStart = trace.startpos;
		beamInfo.m_vecEnd = trace.endpos;

		if (auto pBeam = I::ViewRenderBeams->CreateBeamPoints(beamInfo))
			I::ViewRenderBeams->DrawBeam(pBeam);

		break;
	}
	default:
		H::Particles.ParticleTracer(sString.c_str(), trace.startpos, trace.endpos, iIndex, iAttachment, true);
	}
}


float __fastcall CTFPlayer_GetMinFOV(void* rcx)
{
	UNLOAD_RETURN(CTFPlayer_GetMinFOV, float, rcx);

	return 0.f;
}

bool __fastcall CTFPlayer_InSameDisguisedTeam(void* rcx, CBaseEntity* pEnt)
{
	UNLOAD_RETURN(CTFPlayer_InSameDisguisedTeam, bool, rcx, pEnt);

	if (F::Spectate.HasTarget())
		return true;

	return CALL_ORIGINAL(CTFPlayer_InSameDisguisedTeam, bool, rcx, pEnt);
}

bool __fastcall CTFFreezePanel_ShouldDraw(void* rcx)
{
	UNLOAD_RETURN(CTFFreezePanel_ShouldDraw, bool, rcx);

	if (F::Spectate.HasTarget())
		return false;

	return CALL_ORIGINAL(CTFFreezePanel_ShouldDraw, bool, rcx);
}

void __fastcall CTFFreezePanel_FireGameEvent(void* rcx, IGameEvent* event)
{
	UNLOAD_RETURN(CTFFreezePanel_FireGameEvent, void, rcx, event);

	if (F::Spectate.HasTarget())
		return;

	CALL_ORIGINAL(CTFFreezePanel_FireGameEvent, void, rcx, event);
}

bool __fastcall CTFPlayer_IsPlayerClass(void* rcx, int iClass)
{
	UNLOAD_RETURN(CTFPlayer_IsPlayerClass, bool, rcx, iClass);

	static const auto dwDesired = S::CDamageAccountPanel_DisplayDamageFeedback_IsPlayerClass_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	if (Vars::Misc::Sound::HitsoundAlways.Value && dwRetAddr == dwDesired)
		return false;

	return CALL_ORIGINAL(CTFPlayer_IsPlayerClass, bool, rcx, iClass);
}

bool __fastcall CTFPlayer_ShouldDraw(void* rcx)
{
	UNLOAD_RETURN(CTFPlayer_ShouldDraw, bool, rcx);

	if (F::Spectate.HasTarget() && !I::EngineClient->IsHLTV())
	{
		auto pLocal = H::Entities.GetLocal();
		auto pTarget = I::ClientEntityList->GetClientEntity(I::EngineClient->GetPlayerForUserID(F::Spectate.GetTarget()))->As<CTFPlayer>();
		if (pLocal && pLocal->IsAlive() && rcx == pLocal->GetClientRenderable())
			return true;
		else if (pTarget && pTarget->IsAlive() && rcx == pTarget->GetClientRenderable())
			return Vars::Visuals::Thirdperson::Enabled.Value;
	}

	return CALL_ORIGINAL(CTFPlayer_ShouldDraw, bool, rcx);
}

bool __fastcall CBasePlayer_ShouldDrawThisPlayer(void* rcx)
{
	UNLOAD_RETURN(CBasePlayer_ShouldDrawThisPlayer, bool, rcx);

	//static const auto dwDesired = S::CTFWeaponBase_PostDataUpdate_ShouldDrawThisPlayer_Call();
	static const auto dwUndesired = S::CBasePlayer_BuildFirstPersonMeathookTransformations_ShouldDrawThisPlayer_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	//if (dwRetAddr == dwDesired)
	//	return false; // breaks thirdperson jigglebones?

	if (F::Spectate.HasTarget() && !I::EngineClient->IsHLTV())
	{
		if (dwRetAddr == dwUndesired)
			return false;

		auto pLocal = H::Entities.GetLocal();
		auto pTarget = I::ClientEntityList->GetClientEntity(I::EngineClient->GetPlayerForUserID(F::Spectate.GetTarget()))->As<CTFPlayer>();
		if (pLocal && pLocal->IsAlive() && rcx == pLocal)
			return true;
		else if (pTarget && pTarget->IsAlive() && rcx == pTarget)
			return Vars::Visuals::Thirdperson::Enabled.Value;
	}

	return CALL_ORIGINAL(CBasePlayer_ShouldDrawThisPlayer, bool, rcx);
}

bool __fastcall CBasePlayer_ShouldDrawLocalPlayer(void* rcx)
{
	UNLOAD_RETURN(CBasePlayer_ShouldDrawLocalPlayer, bool, rcx);

	//static const auto dwDesired = S::CBaseCombatWeapon_CalcOverrideModelIndex_ShouldDrawLocalPlayer_Call();
	//const auto dwRetAddr = uintptr_t(_ReturnAddress());

	//if (dwRetAddr == dwDesired)
	//	return false;

	if (F::Spectate.HasTarget() && !I::EngineClient->IsHLTV())
	{
		auto pLocal = H::Entities.GetLocal();
		if (pLocal && pLocal->IsAlive())
			return true;
	}

	return CALL_ORIGINAL(CBasePlayer_ShouldDrawLocalPlayer, bool, rcx);
}

bool __fastcall CBaseCombatWeapon_ShouldDraw(void* rcx)
{
	UNLOAD_RETURN(CBaseCombatWeapon_ShouldDraw, bool, rcx);

	if (F::Spectate.HasTarget() && !I::EngineClient->IsHLTV())
	{
		auto pWeapon = H::Entities.GetWeapon();
		if (pWeapon && rcx == pWeapon->GetClientRenderable())
			return true;
	}

	return CALL_ORIGINAL(CBaseCombatWeapon_ShouldDraw, bool, rcx);
}

void __fastcall CViewRender_DrawViewModels(void* rcx, const CViewSetup& viewRender, bool drawViewmodel)
{
	UNLOAD_RETURN(CViewRender_DrawViewModels, void, rcx, viewRender, drawViewmodel);

	CALL_ORIGINAL(CViewRender_DrawViewModels, void, rcx, viewRender, F::Spectate.GetTarget() != -1 ? false : drawViewmodel);
}

void __fastcall CTFPlayer_UpdateStepSound(void* rcx, void* psurface, const Vec3& vecOrigin, const Vec3& vecVelocity)
{
	UNLOAD_RETURN(CTFPlayer_UpdateStepSound, void, rcx, psurface, std::ref(vecOrigin), std::ref(vecVelocity));

	static const auto dwDesired = S::CTFPlayer_FireEvent_UpdateStepSound_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	if (dwRetAddr == dwDesired && rcx == H::Entities.GetLocal())
		return;

	CALL_ORIGINAL(CTFPlayer_UpdateStepSound, void, rcx, psurface, std::ref(vecOrigin), std::ref(vecVelocity));
}

int __fastcall CTFPlayerInventory_GetMaxItemCount(void* rcx)
{
	UNLOAD_RETURN(CTFPlayerInventory_GetMaxItemCount, int, rcx);

	return Vars::Misc::Exploits::BackpackExpander.Value ? 4000 : CALL_ORIGINAL(CTFPlayerInventory_GetMaxItemCount, int, rcx);
}

void __fastcall CTFPlayerInventory_VerifyChangedLoadoutsAreValid(void* rcx)
{
	UNLOAD_RETURN(CTFPlayerInventory_VerifyChangedLoadoutsAreValid, void, rcx);

	if (!Vars::Misc::Exploits::EquipRegionUnlock.Value)
		CALL_ORIGINAL(CTFPlayerInventory_VerifyChangedLoadoutsAreValid, void, rcx);
}

uint32_t GenerateEquipRegionConflictMask(int iClass, int iUpToSlot, int iIgnoreSlot)
{
	UNLOAD_RETURN(GenerateEquipRegionConflictMask, uint32_t, iClass, iUpToSlot, iIgnoreSlot);

	return Vars::Misc::Exploits::EquipRegionUnlock.Value ? 0 : CALL_ORIGINAL(GenerateEquipRegionConflictMask, uint32_t, iClass, iUpToSlot, iIgnoreSlot);
}

void* __fastcall CTFInventoryManager_GetItemInLoadoutForClass(void* rcx, int iClass, int iSlot, CSteamID* pID)
{
	UNLOAD_RETURN(CTFInventoryManager_GetItemInLoadoutForClass, void*, rcx, iClass, iSlot, pID);

	static const auto dwDesired = S::CEquipSlotItemSelectionPanel_UpdateModelPanelsForSelection_GetItemInLoadoutForClass_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	return dwRetAddr == dwDesired && Vars::Misc::Exploits::EquipRegionUnlock.Value ? nullptr : CALL_ORIGINAL(CTFInventoryManager_GetItemInLoadoutForClass, void*, rcx, iClass, iSlot, pID);
}

static int s_iPlayerIndexGetTeam;
int __fastcall CTFPlayerPanel_GetTeam(void* rcx)
{
	UNLOAD_RETURN(CTFPlayerPanel_GetTeam, int, rcx);

	static const auto dwDesired = S::CTFTeamStatusPlayerPanel_Update_GetTeam_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	if (Vars::Visuals::UI::RevealScoreboard.Value && dwRetAddr == dwDesired)
	{
		if (auto pLocal = H::Entities.GetLocal())
			return pLocal->m_iTeamNum();
	}

	return CALL_ORIGINAL(CTFPlayerPanel_GetTeam, int, rcx);
}

bool __fastcall CTFTeamStatusPlayerPanel_Update(void* rcx)
{
	UNLOAD_RETURN(CTFTeamStatusPlayerPanel_Update, bool, rcx);

	s_iPlayerIndexGetTeam = *reinterpret_cast<int*>(uintptr_t(rcx) + 580);
	return CALL_ORIGINAL(CTFTeamStatusPlayerPanel_Update, bool, rcx);
}

void __fastcall VGui_Panel_SetFgColor(void* rcx, Color_t color)
{
	UNLOAD_RETURN(VGui_Panel_SetFgColor, void, rcx, color);

	if (!F::Groups.GroupsActive())
		return CALL_ORIGINAL(VGui_Panel_SetFgColor, void, rcx, color);

	if (reinterpret_cast<std::uintptr_t>(_ReturnAddress()) == S::VGui_Panel_SetFgColor_RetAddr())
	{ // i'd rather not try to change overheal color because its set in the hud schema and then i would have to implement a bunch of classes and shit
		if (auto pEntity = I::ClientEntityList->GetClientEntity(s_iPlayerIndexGetTeam)->As<CBaseEntity>())
		{
			auto pLocal = H::Entities.GetLocal();
			if (!pEntity || !pEntity->IsPlayer() || !pLocal)
				return CALL_ORIGINAL(VGui_Panel_SetFgColor, void, rcx, color);

			auto pPlayer = pEntity->As<CTFPlayer>();
			if (Vars::Visuals::UI::RevealScoreboard.Value || pPlayer->m_iTeamNum() == pLocal->m_iTeamNum())
			{
				if (Group_t* pGroup = {}; F::Groups.GetGroup(pEntity, pGroup))
					color = GetHealthColor(pPlayer, pGroup, false);
			}

		}
	}
	return CALL_ORIGINAL(VGui_Panel_SetFgColor, void, rcx, color);
}

void __fastcall VGui_Panel_SetBgColor(void* rcx, Color_t color)
{
	UNLOAD_RETURN(VGui_Panel_SetBgColor, void, rcx, color);

	static const auto dwDesired = S::CTFTeamStatusPlayerPanel_Update_SetBgColor_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	if (dwRetAddr == dwDesired && Vars::Visuals::UI::ScoreboardColors.Value)
	{
		Color_t tColor = GetScoreboardColor(s_iPlayerIndexGetTeam);
		if (tColor.a)
		{
			auto pResource = H::Entities.GetResource();
			if (pResource && !pResource->m_bAlive(s_iPlayerIndexGetTeam))
				tColor = tColor.Lerp({ 127, 127, 127, tColor.a }, 0.5f);

			color = tColor;
		}
	}

	CALL_ORIGINAL(VGui_Panel_SetBgColor, void, rcx, color);
}

bool __fastcall CTFPlayerShared_InCond(void* rcx, ETFCond nCond)
{
	UNLOAD_RETURN(CTFPlayerShared_InCond, bool, rcx, nCond);

	const auto dwZoomPlayer = S::CTFPlayer_ShouldDraw_InCond_Call();
	const auto dwZoomWearable = S::CTFWearable_ShouldDraw_InCond_Call();
	const auto dwZoomHudScope = S::CHudScope_ShouldDraw_InCond_Call();
	const auto dwTaunt = S::CTFPlayer_CreateMove_InCondTaunt_Call();
	const auto dwKart1 = S::CTFPlayer_CreateMove_InCondKart_Call();
	const auto dwKart2 = S::CTFInput_ApplyMouse_InCond_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	auto GetOuter = [&rcx]() -> CBaseEntity*
		{
			static const auto iShared = U::NetVars.GetNetVar("CTFPlayer", "m_Shared");
			static const auto iBombHeadStage = U::NetVars.GetNetVar("CTFPlayer", "m_nHalloweenBombHeadStage");
			static const auto iOffset = iBombHeadStage - iShared + 0x4;
			return *reinterpret_cast<CBaseEntity**>(uintptr_t(rcx) + iOffset);
		};

	switch (nCond)
	{
	case TF_COND_ZOOMED:
		if (dwRetAddr == dwZoomPlayer || dwRetAddr == dwZoomWearable || Vars::Visuals::Removals::Scope.Value && dwRetAddr == dwZoomHudScope)
			return false;
		break;
	case TF_COND_DISGUISED:
		if (Vars::Visuals::Removals::Disguises.Value && H::Entities.GetLocal() != GetOuter())
			return false;
		break;
	case TF_COND_TAUNTING:
		if (Vars::Misc::Automation::TauntControl.Value && dwRetAddr == dwTaunt)
			return false;
		if (Vars::Visuals::Removals::Taunts.Value && H::Entities.GetLocal() != GetOuter())
			return false;
		break;
	case TF_COND_HALLOWEEN_KART:
		if (Vars::Misc::Automation::KartControl.Value && (dwRetAddr == dwKart1 || dwRetAddr == dwKart2))
			return false;
		break;
	case TF_COND_FREEZE_INPUT:
		if (!CALL_ORIGINAL(CTFPlayerShared_InCond, bool, rcx, TF_COND_HALLOWEEN_KART) || Vars::Misc::Automation::KartControl.Value)
			return false;
	}

	return CALL_ORIGINAL(CTFPlayerShared_InCond, bool, rcx, nCond);
}

bool __fastcall CTFPlayerShared_IsCritBoosted(void* rcx)
{
	UNLOAD_RETURN(CTFPlayerShared_IsCritBoosted, bool, rcx);

	static bool bPrevForceState = false;

	const auto pLocal = H::Entities.GetLocal();
	const auto pWeapon = H::Entities.GetWeapon();
	if (!pLocal || !pWeapon || pLocal->IsAGhost())
		return CALL_ORIGINAL(CTFPlayerShared_IsCritBoosted, bool, rcx);

	const int iSlot = pWeapon->GetSlot();
	const bool bIsStreamingCrits = pWeapon->m_flCritTime() > TICKS_TO_TIME(pLocal->m_nTickBase());

	const bool bPressed = F::CritHack.IsForcingCrits(G::CurrentUserCmd) || bIsStreamingCrits;
	bool bNowForce = Vars::CritHack::CritVisualEffects.Value && bPressed;
	const bool bCanApplyEffect = !pLocal->IsCritBoosted() && !pLocal->deadflag()
		&& (F::CritHack.WeaponCanCrit(pWeapon)
			&& !(F::CritHack.IsCritBanned() && iSlot != SLOT_MELEE)
			&& F::CritHack.GetAvailableCrits() > 0 && U::ConVars.FindVar("tf_weapon_criticals")->GetInt())
		|| bIsStreamingCrits;

	const auto pLocalShared = pLocal->m_Shared();
	if (bNowForce && bCanApplyEffect && !bPrevForceState)
	{
		S::CTFPlayerShared_UpdateCritBoostEffect.Call<void>(pLocalShared, kCritBoost_ForceRefresh);
		bPrevForceState = true;
	}
	else if (bPrevForceState && (!bNowForce || !bCanApplyEffect))
	{
		S::CTFPlayerShared_UpdateCritBoostEffect.Call<void>(pLocalShared, kCritBoost_ForceRefresh);
		bPrevForceState = false;
	}
	if (!bNowForce || !bCanApplyEffect)
		return CALL_ORIGINAL(CTFPlayerShared_IsCritBoosted, bool, rcx);

	const auto dwRetAddr = uintptr_t(_ReturnAddress());
	static const auto dwCall = S::CTFPlayerShared_IsCritBoosted_CProxyModelGlowColor_OnBind_Call();
	if (dwRetAddr == dwCall)
	{
		const auto pShared = reinterpret_cast<CTFPlayer*>(rcx);
		if (pShared && pShared == pLocalShared)
			return true;
	}

	return CALL_ORIGINAL(CTFPlayerShared_IsCritBoosted, bool, rcx);
}

bool __fastcall CTFConditionList_InCond(void* rcx, ETFCond type)
{
	UNLOAD_RETURN(CTFConditionList_InCond, bool, rcx, type);

	if (!Vars::CritHack::CritVisualEffects.Value)
		return CALL_ORIGINAL(CTFConditionList_InCond, bool, rcx, type);

	const auto dwRetAddr = uintptr_t(_ReturnAddress());
	static const auto dwWeapon = S::CTFPlayerShared_InCond_UpdateCritBoostEffect_Call();
	if (dwRetAddr == dwWeapon)
	{
		const auto pLocal = H::Entities.GetLocal();
		if (!pLocal || pLocal && (uintptr_t)rcx != ((uintptr_t)pLocal->m_Shared() + 0x100)) // shared m_ConditionList (find by printing rcx)
			return CALL_ORIGINAL(CTFConditionList_InCond, bool, rcx, type);

		const auto pWeapon = pLocal->m_hActiveWeapon()->As<CTFWeaponBase>();
		if (!pWeapon)
			return CALL_ORIGINAL(CTFConditionList_InCond, bool, rcx, type);

		const int iSlot = pWeapon->GetSlot();
		const bool bPressed = F::CritHack.IsForcingCrits(G::CurrentUserCmd) || pWeapon->m_flCritTime() > TICKS_TO_TIME(pLocal->m_nTickBase());

		if (!bPressed || F::CritHack.IsCritBanned() && iSlot != SLOT_MELEE
			|| F::CritHack.GetAvailableCrits() <= 0 || !F::CritHack.WeaponCanCrit(pWeapon)
			|| pLocal->IsAGhost() || pLocal->deadflag()
			|| pLocal->IsCritBoosted() || !U::ConVars.FindVar("tf_weapon_criticals")->GetInt())
		{
			return CALL_ORIGINAL(CTFConditionList_InCond, bool, rcx, type);
		}

		return true;
	}

	return CALL_ORIGINAL(CTFConditionList_InCond, bool, rcx, type);
}

bool __fastcall CTFPlayerShared_IsPlayerDominated(void* rcx, int index)
{
	UNLOAD_RETURN(CTFPlayerShared_IsPlayerDominated, bool, rcx, index);

	static const auto dwDesired = S::CTFClientScoreBoardDialog_UpdatePlayerList_IsPlayerDominated_Call();
	static const auto dwJump = S::CTFClientScoreBoardDialog_UpdatePlayerList_Jump();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	const bool bResult = CALL_ORIGINAL(CTFPlayerShared_IsPlayerDominated, bool, rcx, index);

	if (Vars::Visuals::UI::RevealScoreboard.Value && dwRetAddr == dwDesired && !bResult)
		*static_cast<uintptr_t*>(_AddressOfReturnAddress()) = dwJump;

	return bResult;
}

bool __fastcall CTFPlayerShared_ShouldSuppressPrediction(void* rcx)
{
	UNLOAD_RETURN(CTFPlayerShared_ShouldSuppressPrediction, bool, rcx);

	return false;
}

void __fastcall CTFRagdoll_CreateTFRagdoll(void* rcx)
{
	UNLOAD_RETURN(CTFRagdoll_CreateTFRagdoll, void, rcx);

	if (Vars::Visuals::Removals::Ragdolls.Value)
		return;

	auto pRagdoll = reinterpret_cast<CTFRagdoll*>(rcx);
	if (Vars::Visuals::Effects::RagdollEffects.Value)
	{
		pRagdoll->m_bGib() = false;
		pRagdoll->m_bBurning() = Vars::Visuals::Effects::RagdollEffects.Value & Vars::Visuals::Effects::RagdollEffectsEnum::Burning;
		pRagdoll->m_bElectrocuted() = Vars::Visuals::Effects::RagdollEffects.Value & Vars::Visuals::Effects::RagdollEffectsEnum::Electrocuted;
		pRagdoll->m_bBecomeAsh() = Vars::Visuals::Effects::RagdollEffects.Value & Vars::Visuals::Effects::RagdollEffectsEnum::Ash;
		pRagdoll->m_bDissolving() = Vars::Visuals::Effects::RagdollEffects.Value & Vars::Visuals::Effects::RagdollEffectsEnum::Dissolve;
		pRagdoll->m_bGoldRagdoll() = Vars::Visuals::Effects::RagdollEffects.Value & Vars::Visuals::Effects::RagdollEffectsEnum::Gold;
		pRagdoll->m_bIceRagdoll() = Vars::Visuals::Effects::RagdollEffects.Value & Vars::Visuals::Effects::RagdollEffectsEnum::Ice;
	}
	pRagdoll->m_vecForce() *= Vars::Visuals::Effects::RagdollForce.Value;

	CALL_ORIGINAL(CTFRagdoll_CreateTFRagdoll, void, rcx);
}

bool __fastcall CTFRocketLauncher_CheckReloadMisfire(void* rcx)
{
	UNLOAD_RETURN(CTFRocketLauncher_CheckReloadMisfire, bool, rcx);

	auto pWeapon = reinterpret_cast<CTFWeaponBase*>(rcx);
	if (!SDK::AttribHookValue(0, "can_overload", pWeapon))
		return false;

	auto pOwner = pWeapon->m_hOwner()->As<CTFPlayer>();
	int iClip1 = pWeapon->m_iClip1();
	if (pWeapon->m_bRemoveable()) // just using this var since it's in the datamap and doesn't seem to be used on the client
	{
		if (iClip1 > 0)
		{
			pWeapon->CalcIsAttackCritical();
			return true;
		}
		else
			pWeapon->m_bRemoveable() = false;
	}
	else if (iClip1 >= pWeapon->GetMaxClip1() || iClip1 > 0 && pOwner && pOwner->GetAmmoCount(pWeapon->m_iPrimaryAmmoType()) == 0)
	{
		pWeapon->CalcIsAttackCritical();
		pWeapon->m_bRemoveable() = true;
		return true;
	}

	return false;
}

CBaseEntity* CTFRocketLauncher_FireProjectile(void* rcx, CTFPlayer* pPlayer)
{
	UNLOAD_RETURN(CTFRocketLauncher_FireProjectile, CBaseEntity*, rcx, pPlayer);

	auto pWeapon = reinterpret_cast<CTFWeaponBase*>(rcx);
	pWeapon->m_bRemoveable() = false;
	return CALL_ORIGINAL(CTFRocketLauncher_FireProjectile, CBaseEntity*, rcx, pPlayer);
}

// more accurate way to track this? either way doesn't matter too much
/*

void __fastcall CTFBat_Wood_LaunchBall(void* rcx)
{
	UNLOAD_RETURN(CTFBat_Wood_LaunchBall, void, rcx);

	auto pWeapon = reinterpret_cast<CTFWeaponBase*>(rcx);
	pWeapon->CalcIsAttackCritical();

	return CALL_ORIGINAL(CTFBat_Wood_LaunchBall, void, rcx);
}
*/

void __fastcall CBaseEntity_ApplyAbsVelocityImpulse(CBaseEntity* rcx, const Vector& inVecImpulse)
{
	UNLOAD_RETURN(CBaseEntity_ApplyAbsVelocityImpulse, void, rcx, std::ref(inVecImpulse));

	if (!rcx || !rcx->IsPlayer())
		return CALL_ORIGINAL(CBaseEntity_ApplyAbsVelocityImpulse, void, rcx, std::ref(inVecImpulse));

	float flImpulseScale = 1.f;
	auto pPlayer = rcx->As<CTFPlayer>();
	if (pPlayer->m_iClass() == TF_CLASS_SNIPER && pPlayer->InCond(TF_COND_AIMING))
		flImpulseScale = SDK::AttribHookValue(flImpulseScale, "mult_aiming_knockback_resistance", pPlayer);

	if (pPlayer->InCond(TF_COND_HALLOWEEN_TINY) && !pPlayer->InCond(TF_COND_HALLOWEEN_KART))
		flImpulseScale *= 2.f;

	Vector vecForce = inVecImpulse;
	if (pPlayer->InCond(TF_COND_PARACHUTE_DEPLOYED))
	{
		if (const auto pGameRules = I::TFGameRules())
		{
			float flHorizontalScale = pGameRules->m_bPlayingMannVsMachine() && pPlayer->m_bIsABot() ? 0.f : 1.5f;
			vecForce.x *= flHorizontalScale;
			vecForce.y *= flHorizontalScale;
		}
	}

	CALL_ORIGINAL(CBaseEntity_ApplyAbsVelocityImpulse, void, rcx, vecForce * flImpulseScale);
}

static bool m_bScattergunJump = false;
void __fastcall CTFScattergun_FireBullet(void* rcx, CTFPlayer* pPlayer)
{
	UNLOAD_RETURN(CTFScattergun_FireBullet, void, rcx, pPlayer);

	const auto pLocal = H::Entities.GetLocal();
	const auto pWeapon = reinterpret_cast<CBaseCombatWeapon*>(rcx);
	if (!pPlayer || !pWeapon)
		return; // this will crash anyways

	if (!pLocal || pPlayer->entindex() != pLocal->entindex())
		return CALL_ORIGINAL(CTFScattergun_FireBullet, void, rcx, pPlayer);

	const auto pNetChannel = I::EngineClient->GetNetChannelInfo();
	if (!pNetChannel)
		return CALL_ORIGINAL(CTFScattergun_FireBullet, void, rcx, pPlayer);

	bool bIsForceANature = pWeapon->m_iItemDefinitionIndex() == Scout_m_ForceANature || pWeapon->m_iItemDefinitionIndex() == Scout_m_FestiveForceANature;
	if (SDK::GetRoundState() == GR_STATE_PREROUND || m_bScattergunJump ||
		!bIsForceANature || pLocal->IsOnGround())
		return CALL_ORIGINAL(CTFScattergun_FireBullet, void, rcx, pPlayer);

	m_bScattergunJump = true;

	static float flLastJumpTime = 0.f;
	static Vec3 vJumpAngles = {};

	float flLatency = pNetChannel->GetLatency(FLOW_OUTGOING) + pNetChannel->GetLatency(FLOW_INCOMING);
	float flDifference = I::GlobalVars->curtime - flLastJumpTime;
	if (flDifference > flLatency)
	{
		vJumpAngles = pLocal->GetAbsAngles();
		flLastJumpTime = I::GlobalVars->curtime;
	}

	// use this instead of addcond because it has built-in timer
	S::CTFPlayerShared_StunPlayer.Call<void>(pLocal->m_Shared(), 0.3f, 1.f, TF_STUN_MOVEMENT | TF_STUN_MOVEMENT_FORWARD_ONLY, nullptr);

	Vec3 vLocalOrigin = pLocal->GetAbsOrigin();

	VMatrix mtxPlayer{};
	mtxPlayer.SetupMatrixOrgAngles(vLocalOrigin, vJumpAngles);

	Vec3 vAbsVelocity = pLocal->GetAbsVelocity();
	Vec3 vAbsVelocityAsPoint = vAbsVelocity + vLocalOrigin;
	Vec3 vLocalVelocity = mtxPlayer.VMul4x3Transpose(vAbsVelocityAsPoint);

	vLocalVelocity.x = -300;

	vAbsVelocityAsPoint = mtxPlayer.VMul4x3(vLocalVelocity);
	vAbsVelocity = vAbsVelocityAsPoint - vLocalOrigin;
	pLocal->SetAbsVelocity(vAbsVelocity);

	U::Hooks.m_CBaseEntity_ApplyAbsVelocityImpulse.fastcall<void>(pLocal, Vec3(0, 0, 50.f));
	pLocal->m_fFlags() &= ~FL_ONGROUND;

	CALL_ORIGINAL(CTFScattergun_FireBullet, void, rcx, pPlayer);
}

void __fastcall CTFGameMovement_SetGroundEntity(void* rcx, trace_t* pm)
{
	UNLOAD_RETURN(CTFGameMovement_SetGroundEntity, void, rcx, pm);

	CALL_ORIGINAL(CTFGameMovement_SetGroundEntity, void, rcx, pm);

	if (pm && pm->m_pEnt == H::Entities.GetLocal())
		m_bScattergunJump = false;
}

static int s_iCurrentSeed = -1;
void __fastcall CTFWeaponBase_CalcIsAttackCritical(void* rcx)
{
	UNLOAD_RETURN(CTFWeaponBase_CalcIsAttackCritical, void, rcx);

	auto pWeapon = reinterpret_cast<CTFWeaponBase*>(rcx);

	const auto nPreviousWeaponMode = pWeapon->m_iWeaponMode();
	pWeapon->m_iWeaponMode() = TF_WEAPON_PRIMARY_MODE;
	if (I::Prediction->m_bFirstTimePredicted)
	{
		CALL_ORIGINAL(CTFWeaponBase_CalcIsAttackCritical, void, rcx);
		s_iCurrentSeed = pWeapon->m_iCurrentSeed();
	}
	else // fixes minigun and flamethrower buggy crit sounds for the most part
	{
		float flOldCritTokenBucket = pWeapon->m_flCritTokenBucket();
		int nOldCritChecks = pWeapon->m_nCritChecks();
		int nOldCritSeedRequests = pWeapon->m_nCritSeedRequests();
		float flOldLastRapidFireCritCheckTime = pWeapon->m_flLastRapidFireCritCheckTime();
		float flOldCritTime = pWeapon->m_flCritTime();
		CALL_ORIGINAL(CTFWeaponBase_CalcIsAttackCritical, void, rcx);
		pWeapon->m_flCritTokenBucket() = flOldCritTokenBucket;
		pWeapon->m_nCritChecks() = nOldCritChecks;
		pWeapon->m_nCritSeedRequests() = nOldCritSeedRequests;
		pWeapon->m_flLastRapidFireCritCheckTime() = flOldLastRapidFireCritCheckTime;
		pWeapon->m_flCritTime() = flOldCritTime;
		pWeapon->m_iCurrentSeed() = s_iCurrentSeed; // make sure seed stays changed
	}
	pWeapon->m_iWeaponMode() = nPreviousWeaponMode;
}

bool __fastcall CTFWeaponBase_CanFireRandomCriticalShot(void* rcx, float flCritChance)
{ // not present on the client so it will always be a crit behind otherwise
	UNLOAD_RETURN(CTFWeaponBase_CanFireRandomCriticalShot, bool, rcx, flCritChance);

	int nRandomRangedCritDamage = F::CritHack.GetCritDamage();
	int nTotalDamage = F::CritHack.GetRangedDamage();
	if (!nTotalDamage)
		return true;

	auto pWeapon = reinterpret_cast<CTFWeaponBase*>(rcx);
	float flNormalizedDamage = nRandomRangedCritDamage / TF_DAMAGE_CRIT_MULTIPLIER;
	pWeapon->m_flObservedCritChance() = flNormalizedDamage / (flNormalizedDamage + (nTotalDamage - nRandomRangedCritDamage));

	return CALL_ORIGINAL(CTFWeaponBase_CanFireRandomCriticalShot, bool, rcx, flCritChance);
}

const char* CTFWeaponBase_GetShootSound(void* rcx, int iIndex)
{
	UNLOAD_RETURN(CTFWeaponBase_GetShootSound, const char*, rcx, iIndex);

	if (Vars::Misc::Sound::GiantWeaponSounds.Value)
	{
		auto pWeapon = H::Entities.GetWeapon();
		if (rcx == pWeapon)
		{	// credits: KGB
			int nOldTeam = pWeapon->m_iTeamNum();
			pWeapon->m_iTeamNum() = TF_TEAM_COUNT;
			auto sReturn = CALL_ORIGINAL(CTFWeaponBase_GetShootSound, const char*, rcx, iIndex);
			pWeapon->m_iTeamNum() = nOldTeam;

			switch (FNV1A::Hash32(sReturn))
			{
			case FNV1A::Hash32Const("Weapon_FlameThrower.Fire"): return "MVM.GiantPyro_FlameStart";
			case FNV1A::Hash32Const("Weapon_FlameThrower.FireLoop"): return "MVM.GiantPyro_FlameLoop";
			case FNV1A::Hash32Const("Weapon_GrenadeLauncher.Single"): return "MVM.GiantDemoman_Grenadeshoot";
			}

			return sReturn;
		}
	}

	return CALL_ORIGINAL(CTFWeaponBase_GetShootSound, const char*, rcx, iIndex);
}

void __fastcall CThirdPersonManager_Update(void* rcx)
{
	UNLOAD_RETURN(CThirdPersonManager_Update, void, rcx);

	return;
}

void __fastcall CViewRender_DrawUnderwaterOverlay(void* rcx)
{
	UNLOAD_RETURN(CViewRender_DrawUnderwaterOverlay, void, rcx);

	if (!Vars::Visuals::Removals::ScreenOverlays.Value || Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		CALL_ORIGINAL(CViewRender_DrawUnderwaterOverlay, void, rcx);
}

void __fastcall CViewRender_LevelInit(void* rcx)
{
	UNLOAD_RETURN(CViewRender_LevelInit, void, rcx);

	F::Materials.ReloadMaterials();
	F::Visuals.OverrideWorldTextures();

	F::Backtrack.Reset();
	F::Ticks.Reset();
	F::NoSpreadHitscan.Reset();
	F::CheaterDetection.Reset();
	F::Resolver.Reset();
#ifdef ANTIAUTOBALANCETESTING
	F::AntiAutobalance.Reset();
#endif
	F::Spectate.Reset();

	CALL_ORIGINAL(CViewRender_LevelInit, void, rcx);
}

void __fastcall CViewRender_PerformScreenOverlay(void* rcx, int x, int y, int w, int h)
{
	UNLOAD_RETURN(CViewRender_PerformScreenOverlay, void, rcx, x, y, w, h);

	if (!Vars::Visuals::Removals::ScreenOverlays.Value || Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		CALL_ORIGINAL(CViewRender_PerformScreenOverlay, void, rcx, x, y, w, h);
}

void __fastcall CViewRender_RenderView(void* rcx, const CViewSetup& view, ClearFlags_t nClearFlags, RenderViewInfo_t whatToDraw)
{
	UNLOAD_RETURN(CViewRender_RenderView, void, rcx, std::ref(view), nClearFlags, whatToDraw);

	CALL_ORIGINAL(CViewRender_RenderView, void, rcx, std::ref(view), nClearFlags, whatToDraw);
	if (G::Unload || Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return;

	F::CameraWindow.RenderView(rcx, view);
}

void __fastcall CWeaponMedigun_PrimaryAttack(CWeaponMedigun* rcx)
{
	UNLOAD_RETURN(CWeaponMedigun_PrimaryAttack, void, rcx);

	auto pOwner = rcx->m_hOwner()->As<CTFPlayer>();
	if (!pOwner || !pOwner->m_pCurrentCommand())
		return CALL_ORIGINAL(CWeaponMedigun_PrimaryAttack, void, rcx);

	// do pseudo lagcomp for the client
	std::unordered_map<CBaseEntity*, Restore_t> mRestore = {};

	auto pCmd = pOwner->m_pCurrentCommand();
	float flTargetTime = TICKS_TO_TIME(pCmd->tick_count - TIME_TO_TICKS(F::Backtrack.GetFakeInterp()));
	for (auto pEntity : H::Entities.GetGroup(EntityEnum::PlayerTeam))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		if (pPlayer == pOwner || !pPlayer->IsAlive() || pPlayer->IsAGhost())
			continue;

		std::vector<TickRecord*> vRecords = {};
		if (!F::Backtrack.GetRecords(pEntity, vRecords))
			continue;
		vRecords = F::Backtrack.GetValidRecords(vRecords);
		if (!vRecords.size())
			continue;

		for (auto pRecord : vRecords)
		{
			if (pRecord->m_flSimTime <= flTargetTime)
			{
				mRestore[pEntity] = { pEntity->GetAbsOrigin(), pEntity->m_vecMins(), pEntity->m_vecMaxs() };
				pEntity->SetAbsOrigin(pRecord->m_vOrigin);
				pEntity->m_vecMins() = pRecord->m_vMins;
				pEntity->m_vecMaxs() = pRecord->m_vMaxs;
				break;
			}
		}
	}

	CALL_ORIGINAL(CWeaponMedigun_PrimaryAttack, void, rcx);

	for (auto& [pEntity, tRestore] : mRestore)
	{
		pEntity->SetAbsOrigin(tRestore.m_vOrigin);
		pEntity->m_vecMins() = tRestore.m_vMins;
		pEntity->m_vecMaxs() = tRestore.m_vMaxs;
	}
	mRestore.clear();
}

void __fastcall DataTable_Warning(const char* pInMessage, ...)
{
	return;
}

HRESULT __stdcall Direct3DDevice9_Present(IDirect3DDevice9* pDevice, const RECT* pSource, const RECT* pDestination, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
	if (!G::Unload)
		F::Render.Render(pDevice);

	return U::Hooks.m_Direct3DDevice9_Present.stdcall<HRESULT>(pDevice, pSource, pDestination, hDestWindowOverride, pDirtyRegion);
}

HRESULT __stdcall Direct3DDevice9_Reset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	if(G::Unload)
		return U::Hooks.m_Direct3DDevice9_Reset.stdcall<HRESULT>(pDevice, pPresentationParameters);

	ImGui_ImplDX9_InvalidateDeviceObjects();
	const HRESULT Original = U::Hooks.m_Direct3DDevice9_Reset.stdcall<HRESULT>(pDevice, pPresentationParameters);
	ImGui_ImplDX9_CreateDeviceObjects();
	return Original;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LONG __stdcall WndProc::Func(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(G::Unload)
		return CallWindowProc(Original, hWnd, uMsg, wParam, lParam);

	if (F::Menu.m_bIsOpen)
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

		if ((ImGui::GetIO().WantTextInput || F::Menu.m_bInKeybind) && WM_KEYFIRST <= uMsg && uMsg <= WM_KEYLAST)
		{
			I::InputSystem->ResetInputState();
			return 1;
		}

		if (WM_MOUSEFIRST <= uMsg && uMsg <= WM_MOUSELAST)
			return 1;
	}

	return CallWindowProc(Original, hWnd, uMsg, wParam, lParam);
}

void __fastcall VGuiSurface_LockCursor(void* rcx)
{
	UNLOAD_RETURN(VGuiSurface_LockCursor, void, rcx);

	if (F::Menu.m_bIsOpen)
		return I::MatSystemSurface->UnlockCursor();

	CALL_ORIGINAL(VGuiSurface_LockCursor, void, rcx);
}

void __fastcall VGuiSurface_SetCursor(void* rcx, HCursor cursor)
{
	UNLOAD_RETURN(VGuiSurface_SetCursor, void, rcx, cursor);

	if (F::Menu.m_bIsOpen)
	{
		switch (F::Render.Cursor)
		{
		case 0: cursor = 2; break;
		case 1: cursor = 3; break;
		case 2: cursor = 12; break;
		case 3: cursor = 11; break;
		case 4: cursor = 10; break;
		case 5: cursor = 9; break;
		case 6: cursor = 8; break;
		case 7: cursor = 14; break;
		case 8: cursor = 13; break;
		}
	}

	CALL_ORIGINAL(VGuiSurface_SetCursor, void, rcx, cursor);
}

void WndProc::Initialize()
{
	hwWindow = SDK::GetTeamFortressWindow();

	Original = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Func)));
}

void WndProc::Unload()
{
	SetWindowLongPtr(hwWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Original));
}

void __fastcall DoEnginePostProcessing(int x, int y, int w, int h, bool bFlashlightIsOn, bool bPostVGui)
{
	UNLOAD_RETURN(DoEnginePostProcessing, void, x, y, w, h, bFlashlightIsOn, bPostVGui);
	
	if (!Vars::Visuals::Removals::PostProcessing.Value || Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		CALL_ORIGINAL(DoEnginePostProcessing, void, x, y, w, h, bFlashlightIsOn, bPostVGui);
}

void __fastcall DSP_Process(unsigned int idsp, int* pbfront, int* pbrear, int* pbcenter, int sampleCount)
{
	UNLOAD_RETURN(DSP_Process, void, idsp, pbfront, pbrear, pbcenter, sampleCount);

	if (!Vars::Misc::Sound::RemoveDSP.Value)
		CALL_ORIGINAL(DSP_Process, void, idsp, pbfront, pbrear, pbcenter, sampleCount);
}

void __fastcall FX_FireBullets(CTFWeaponBase* pWpn, int iPlayer, const Vec3& vecOrigin, const Vec3& vecAngles, int iWeapon, int iMode, int iSeed, float flSpread, float flDamage, bool bCritical)
{
	UNLOAD_RETURN(FX_FireBullets, void, pWpn, iPlayer, std::ref(vecOrigin), std::ref(vecAngles), iWeapon, iMode, iSeed, flSpread, flDamage, bCritical);

	static const auto dwDesired = S::CTFWeaponBaseGun_FireBullet_FireBullets_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	if (iPlayer != I::EngineClient->GetLocalPlayer())
		F::Backtrack.ReportShot(iPlayer);
	else if (Vars::Aimbot::General::NoSpread.Value && dwRetAddr == dwDesired)
		iSeed = F::NoSpreadHitscan.m_iSeed;

	return CALL_ORIGINAL(FX_FireBullets, void, pWpn, iPlayer, std::ref(vecOrigin), std::ref(vecAngles), iWeapon, iMode, iSeed, flSpread, flDamage, bCritical);
}

#ifdef SEEDPRED_DEBUG // define in header
MAKE_SIGNATURE(FX_FireBullets_Server, "server.dll", "48 89 5C 24 ? 4C 89 4C 24 ? 55 56 41 54", 0x0);
MAKE_SIGNATURE(CBasePlayer_ProcessUsercmds, "server.dll", "40 53 55 56 57 41 54 48 83 EC ? 4C 89 6C 24", 0x0);

void __fastcall FX_FireBullets_Server(CTFWeaponBase* pWpn, int iPlayer, const Vec3& vecOrigin, const Vec3& vecAngles, int iWeapon, int iMode, int iSeed, float flSpread, float flDamage, bool bCritical)
{
	UNLOAD_RETURN(FX_FireBullets_Server, void, pWpn, iPlayer, std::ref(vecOrigin), std::ref(vecAngles), iWeapon, iMode, iSeed, flSpread, flDamage, bCritical);

	if (Vars::Aimbot::General::NoSpread.Value)
		SDK::Output("FX_FireBullets", std::format("{}", iSeed).c_str(), { 0, 255, 0 });
	return CALL_ORIGINAL(FX_FireBullets_Server, void, pWpn, iPlayer, std::ref(vecOrigin), std::ref(vecAngles), iWeapon, iMode, iSeed, flSpread, flDamage, bCritical);
}

void __fastcall CBasePlayer_ProcessUsercmds(void* rcx, CUserCmd* cmds, int numcmds, int totalcmds, int dropped_packets, bool paused)
{
	UNLOAD_RETURN(CBasePlayer_ProcessUsercmds, void, rcx, cmds, numcmds, totalcmds, dropped_packets, paused);

	bool bInAttack = false;
	for (int i = totalcmds - 1; i >= 0; i--)
	{
		CUserCmd* pCmd = &cmds[totalcmds - 1 - i];
		if (pCmd && pCmd->buttons & IN_ATTACK)
			bInAttack = true;
	}
	if (bInAttack)
	{
		double dFloatTime = SDK::PlatFloatTime();
		float flTime = float(SDK::PlatFloatTime() * 1000.0);
		int iSeed = *reinterpret_cast<int*>((char*)&flTime) & 255;
		SDK::Output("ProcessUsercmds", std::format("{}: {}", dFloatTime, iSeed).c_str(), { 0, 255, 0, 100 });
	}

	return CALL_ORIGINAL(CBasePlayer_ProcessUsercmds, void, rcx, cmds, numcmds, totalcmds, dropped_packets, paused);
}
#endif

float __fastcall GetClientInterpAmount()
{
	UNLOAD_RETURN(GetClientInterpAmount, float);

	if (Vars::Visuals::Removals::Lerp.Value && !Vars::Visuals::Removals::Interpolation.Value)
		return CALL_ORIGINAL(GetClientInterpAmount, float);

	static const auto dwDesired1 = S::CNetGraphPanel_DrawTextFields_GetClientInterpAmount_Call1();
	static const auto dwDesired2 = S::CNetGraphPanel_DrawTextFields_GetClientInterpAmount_Call2();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	return dwRetAddr == dwDesired1 || dwRetAddr == dwDesired2 ? CALL_ORIGINAL(GetClientInterpAmount, float) : 0.f;
}

void __fastcall HostState_Shutdown()
{
	U::Core.m_bUnload = true;
	CALL_ORIGINAL(HostState_Shutdown, void);
}

void __fastcall HostState_Restart()
{
	U::Core.m_bUnload = true;
	CALL_ORIGINAL(HostState_Restart, void);
}

static bool s_bNoSkip = false;
static std::optional<bool> s_bStartSolid = std::nullopt;

void __fastcall IEngineTrace_TraceRay(void* rcx, const Ray_t& ray, unsigned int fMask, ITraceFilter* pTraceFilter, trace_t* pTrace)
{
	UNLOAD_RETURN(IEngineTrace_TraceRay, void, rcx, std::ref(ray), fMask, pTraceFilter, pTrace);

	s_bNoSkip = fMask & CONTENTS_NOSKIP;

	CALL_ORIGINAL(IEngineTrace_TraceRay, void, rcx, std::ref(ray), fMask, pTraceFilter, pTrace);
#ifdef DEBUG_TRACES
	if (Vars::Debug::VisualizeTraces.Value)
	{
		Vec3 vStart = ray.m_Start + ray.m_StartOffset;
		Vec3 vEnd = Vars::Debug::VisualizeTraceHits.Value ? pTrace->endpos : ray.m_Delta + vStart;
		G::LineStorage.emplace_back(std::pair<Vec3, Vec3>(vStart, vEnd), I::GlobalVars->curtime + 0.015f, Color_t(0, 0, 255), bool(GetAsyncKeyState(VK_MENU) & 0x8000));
		if (!ray.m_IsRay)
		{
			G::BoxStorage.emplace_back(ray.m_Start, ray.m_Extents * -1, ray.m_Extents, Vec3(), I::GlobalVars->curtime + 0.015f, Color_t(0, 0, 255), Color_t(0, 0, 0, 0), bool(GetAsyncKeyState(VK_MENU) & 0x8000));
			G::BoxStorage.emplace_back(vEnd - ray.m_StartOffset, ray.m_Extents * -1, ray.m_Extents, Vec3(), I::GlobalVars->curtime + 0.015f, Color_t(0, 0, 255), Color_t(0, 0, 0, 0), bool(GetAsyncKeyState(VK_MENU) & 0x8000));
		}
	}
#endif

	if (s_bStartSolid)
		pTrace->startsolid = s_bStartSolid.value(), s_bStartSolid = std::nullopt;

	s_bNoSkip = false;
}

void __fastcall CM_BoxTrace(const Ray_t& ray, int headnode, int brushmask, bool computeEndpt, trace_t& tr)
{
	UNLOAD_RETURN(CM_BoxTrace, void, std::ref(ray), headnode, brushmask, computeEndpt, std::ref(tr));

	CALL_ORIGINAL(CM_BoxTrace, void, std::ref(ray), headnode, brushmask, computeEndpt, std::ref(tr));

	if (s_bStartSolid)
		tr.startsolid = s_bStartSolid.value(), s_bStartSolid = std::nullopt;
	if (s_bNoSkip)
		s_bStartSolid = tr.startsolid, tr.startsolid = false;
}

void __fastcall CM_ClipBoxToBrush_True(TraceInfo_t* pTraceInfo, void* brush)
{
	UNLOAD_RETURN(CM_ClipBoxToBrush_True, void, pTraceInfo, brush);

	if (s_bStartSolid)
		pTraceInfo->m_trace.startsolid = s_bStartSolid.value(), s_bStartSolid = std::nullopt;

	CALL_ORIGINAL(CM_ClipBoxToBrush_True, void, pTraceInfo, brush);

	if (s_bNoSkip)
	{
		s_bStartSolid = pTraceInfo->m_trace.startsolid, pTraceInfo->m_trace.startsolid = false;
		if (!s_bStartSolid.value() && !pTraceInfo->m_trace.fractionleftsolid && pTraceInfo->m_trace.fraction != 1.f)
			pTraceInfo->m_trace.fractionleftsolid = pTraceInfo->m_trace.fraction - FLT_EPSILON;
	}
}

void __fastcall CM_ClipBoxToBrush_False(TraceInfo_t* pTraceInfo, void* brush)
{
	UNLOAD_RETURN(CM_ClipBoxToBrush_False, void, pTraceInfo, brush);

	if (s_bStartSolid)
		pTraceInfo->m_trace.startsolid = s_bStartSolid.value(), s_bStartSolid = std::nullopt;

	CALL_ORIGINAL(CM_ClipBoxToBrush_False, void, pTraceInfo, brush);

	if (s_bNoSkip)
	{
		s_bStartSolid = pTraceInfo->m_trace.startsolid, pTraceInfo->m_trace.startsolid = false;
		if (!s_bStartSolid.value() && !pTraceInfo->m_trace.fractionleftsolid && pTraceInfo->m_trace.fraction != 1.f)
			pTraceInfo->m_trace.fractionleftsolid = pTraceInfo->m_trace.fraction - FLT_EPSILON;
	}
}

bool __fastcall CEngineTrace_ClipTraceToTrace(void* rcx, trace_t& clipTrace, trace_t* pFinalTrace)
{
	UNLOAD_RETURN(CEngineTrace_ClipTraceToTrace, bool, rcx, std::ref(clipTrace), pFinalTrace);

	if (s_bStartSolid)
		pFinalTrace->startsolid = s_bStartSolid.value(), s_bStartSolid = std::nullopt;
	if (s_bNoSkip && (!clipTrace.fraction || clipTrace.fraction < pFinalTrace->fractionleftsolid))
		return false;

	return CALL_ORIGINAL(CEngineTrace_ClipTraceToTrace, bool, rcx, std::ref(clipTrace), pFinalTrace);
}

void __fastcall IEngineVGui_Paint(void* rcx, int iMode)
{
	UNLOAD_RETURN(IEngineVGui_Paint, void, rcx, iMode);

	if (iMode & PAINT_INGAMEPANELS && (!(Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())))
	{
		H::Draw.UpdateScreenSize();
		H::Draw.UpdateW2SMatrix();
		H::Draw.Start(true);
		if (auto pLocal = H::Entities.GetLocal())
		{
			F::CameraWindow.Draw();
			F::Visuals.DrawServerHitboxes(pLocal);
			F::Visuals.DrawAntiAim(pLocal);

			F::Visuals.DrawPickupTimers();
			F::ESP.Draw();
			F::Arrows.Draw(pLocal);
			F::Aimbot.Draw(pLocal);
			F::Radar.Run(pLocal);
#ifdef DEBUG_VACCINATOR
			F::AutoHeal.Draw(pLocal);
#endif
		}
		H::Draw.End();
	}

	CALL_ORIGINAL(IEngineVGui_Paint, void, rcx, iMode);

	if (iMode & PAINT_UIPANELS && (!(Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())))
	{
		H::Draw.UpdateScreenSize();
		H::Draw.Start();
		{
			if (auto pLocal = H::Entities.GetLocal())
			{
				F::NoSpreadHitscan.Draw(pLocal);
				F::PlayerConditions.Draw(pLocal);
				F::SpectatorList.Draw(pLocal);
				F::CritHack.Draw(pLocal);
				F::Ticks.Draw(pLocal);
				F::Visuals.DrawDebugInfo(pLocal);
				F::Backtrack.Draw(pLocal);
			}
			F::Notifications.Draw();
			F::Spotify.Draw();

			if (F::Menu.m_bIsOpen)
				F::Visuals.CatDraw(F::Menu.m_vWindowSize, F::Menu.m_vWindowPos);
		}
		H::Draw.End();
	}
}

ITexture* IMaterialSystem_FindTexture(void* rcx, char const* pTextureName, const char* pTextureGroupName, bool complain, int nAdditionalCreationFlags)
{
	UNLOAD_RETURN(IMaterialSystem_FindTexture, ITexture*, rcx, pTextureName, pTextureGroupName, complain, nAdditionalCreationFlags);

	auto pReturn = CALL_ORIGINAL(IMaterialSystem_FindTexture, ITexture*, rcx, pTextureName, pTextureGroupName, complain, nAdditionalCreationFlags);

	if (FNV1A::Hash32(Vars::Visuals::World::WorldTexture.Value.c_str()) == FNV1A::Hash32Const("Flat"))
	{
		if (!pReturn || pReturn->IsTranslucent() || !pTextureName || !pTextureGroupName)
			return pReturn;

		std::string_view sName = pTextureName;
		std::string_view sGroup = pTextureGroupName;
		if (!sGroup.starts_with(TEXTURE_GROUP_WORLD))
			return pReturn;

		Vec3 vColor; pReturn->GetLowResColorSample(0.5f, 0.5f, &vColor.x);
		Color_t tColor = { byte(vColor.x * 255), byte(vColor.y * 255), byte(vColor.z * 255), 255 };
		pReturn = I::MaterialSystem->CreateTextureFromBits(1, 1, 1, IMAGE_FORMAT_RGBA8888, 4, &tColor.r);
	}

	return pReturn;
}

void __fastcall IMatSystemSurface_OnScreenSizeChanged(void* rcx, int nOldWidth, int nOldHeight)
{
	UNLOAD_RETURN(IMatSystemSurface_OnScreenSizeChanged, void, rcx, nOldWidth, nOldHeight);

	CALL_ORIGINAL(IMatSystemSurface_OnScreenSizeChanged, void, rcx, nOldWidth, nOldHeight);

	H::Fonts.ReloadFonts();
	F::Materials.ReloadMaterials();
}

void __fastcall IPanel_PaintTraverse(void* rcx, VPANEL vguiPanel, bool forceRepaint, bool allowForce)
{
	UNLOAD_RETURN(IPanel_PaintTraverse, void, rcx, vguiPanel, forceRepaint, allowForce);

	if (!Vars::Visuals::UI::StreamerMode.Value)
		return CALL_ORIGINAL(IPanel_PaintTraverse, void, rcx, vguiPanel, forceRepaint, allowForce);

	switch (FNV1A::Hash32(I::Panel->GetName(vguiPanel)))
	{
	case FNV1A::Hash32Const("SteamFriendsList"):
	case FNV1A::Hash32Const("avatar"):
	case FNV1A::Hash32Const("RankPanel"):
	case FNV1A::Hash32Const("ModelContainer"):
	case FNV1A::Hash32Const("ServerLabelNew"):
		return;
	}

	CALL_ORIGINAL(IPanel_PaintTraverse, void, rcx, vguiPanel, forceRepaint, allowForce);
}

const char* __fastcall ISteamFriends_GetFriendPersonaName(void* rcx, CSteamID steamIDFriend)
{
	UNLOAD_RETURN(ISteamFriends_GetFriendPersonaName, const char*, rcx, steamIDFriend);

	static const auto dwDesired = S::GetPlayerNameForSteamID_GetFriendPersonaName_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	if (Vars::Visuals::UI::StreamerMode.Value && dwRetAddr == dwDesired)
	{
		if (I::SteamUser->GetSteamID() == steamIDFriend)
		{
			if (Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::Local)
				return "Local";
		}
		else if (I::SteamFriends->HasFriend(steamIDFriend, k_EFriendFlagImmediate))
		{
			if (Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::Friends)
				return "Friend";
		}
		else
		{
			if (Vars::Visuals::UI::StreamerMode.Value >= Vars::Visuals::UI::StreamerModeEnum::Party)
				return "Party";
		}
	}

	return CALL_ORIGINAL(ISteamFriends_GetFriendPersonaName, const char*, rcx, steamIDFriend);
}

int __fastcall ISteamNetworkingUtils_GetPingToDataCenter(void* rcx, SteamNetworkingPOPID popID, SteamNetworkingPOPID* pViaRelayPoP)
{
	UNLOAD_RETURN(ISteamNetworkingUtils_GetPingToDataCenter, int, rcx, popID, pViaRelayPoP);

	int iReturn = CALL_ORIGINAL(ISteamNetworkingUtils_GetPingToDataCenter, int, rcx, popID, pViaRelayPoP);
	if (!Vars::Misc::Queueing::ForceRegions.Value || iReturn < 0)
		return iReturn;

	char sPopID[5];
	PopIdName(popID, sPopID);
	if (auto uDatacenter = GetDatacenter(FNV1A::Hash32(sPopID)))
		return Vars::Misc::Queueing::ForceRegions.Value & uDatacenter ? 1 : 1000;

	return iReturn;
}

void __fastcall CTFPartyClient_RequestQueueForMatch(void* rcx, int eMatchGroup)
{
	UNLOAD_RETURN(CTFPartyClient_RequestQueueForMatch, void, rcx, eMatchGroup);

	I::TFGCClientSystem->SetPendingPingRefresh(true);
	I::TFGCClientSystem->PingThink();

	CALL_ORIGINAL(CTFPartyClient_RequestQueueForMatch, void, rcx, eMatchGroup);
}

void __fastcall IVModelRender_DrawModelExecute(void* rcx, const DrawModelState_t& pState, const ModelRenderInfo_t& pInfo, matrix3x4* pBoneToWorld)
{
	if (I::EngineVGui->IsGameUIVisible() || Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot()
		|| F::CameraWindow.m_bDrawing || !F::Materials.m_bLoaded || G::Unload)
		return CALL_ORIGINAL(IVModelRender_DrawModelExecute, void, rcx, std::ref(pState), pInfo, pBoneToWorld);

	if (Vars::Visuals::World::SimpleModels.Value)
		*const_cast<int*>(&pState.m_lod) = 5;

	if (F::Chams.m_bRendering)
		return F::Chams.RenderHandler(pState, pInfo, pBoneToWorld);
	if (F::Glow.m_bRendering)
		return F::Glow.RenderHandler(pState, pInfo, pBoneToWorld);

	if (F::Chams.m_mEntities.contains(pInfo.entity_index))
		return;

	auto pEntity = I::ClientEntityList->GetClientEntity(pInfo.entity_index);
	auto pRenderContext = I::MaterialSystem->GetRenderContext();
	if (pEntity && pRenderContext && pEntity->GetClassID() == ETFClassID::CTFViewModel)
	{
		F::Glow.RenderViewmodel(pState, pInfo, pBoneToWorld);
		if (F::Chams.RenderViewmodel(pState, pInfo, pBoneToWorld))
			return;
	}

	CALL_ORIGINAL(IVModelRender_DrawModelExecute, void, rcx, std::ref(pState), pInfo, pBoneToWorld);
}

static bool s_bDrawingViewmodel = false;
int __fastcall CBaseAnimating_DrawModel(void* rcx, int flags)
{
	static const auto dwDrawModel = S::CEconEntity_DrawOverriddenViewmodel_DrawModel_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	if (dwRetAddr != dwDrawModel || I::EngineVGui->IsGameUIVisible() || Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot()
		|| F::CameraWindow.m_bDrawing || !F::Materials.m_bLoaded || G::Unload)
		return CALL_ORIGINAL(CBaseAnimating_DrawModel, int, rcx, flags);

	s_bDrawingViewmodel = true;
	int iReturn = CALL_ORIGINAL(CBaseAnimating_DrawModel, int, rcx, flags);
	s_bDrawingViewmodel = false;
	return iReturn;
}

int __fastcall CBaseAnimating_InternalDrawModel(void* rcx, int flags)
{
	UNLOAD_RETURN(CBaseAnimating_InternalDrawModel, int, rcx, flags);

	if (!s_bDrawingViewmodel || !(flags & STUDIO_RENDER))
		return CALL_ORIGINAL(CBaseAnimating_InternalDrawModel, int, rcx, flags);

	auto pRenderContext = I::MaterialSystem->GetRenderContext();
	if (!pRenderContext)
		return CALL_ORIGINAL(CBaseAnimating_InternalDrawModel, int, rcx, flags);

	int iReturn;
	F::Glow.RenderViewmodel(rcx, flags);
	if (F::Chams.RenderViewmodel(rcx, flags, &iReturn))
		return iReturn;

	return CALL_ORIGINAL(CBaseAnimating_InternalDrawModel, int, rcx, 1);
}

void __fastcall IVModelRender_ForcedMaterialOverride(IVModelRender* rcx, IMaterial* mat, OverrideType_t type)
{
	UNLOAD_RETURN(IVModelRender_ForcedMaterialOverride, void, rcx, mat, type);

	if (F::Chams.m_bRendering || F::Glow.m_bRendering)
		return;

	CALL_ORIGINAL(IVModelRender_ForcedMaterialOverride, void, rcx, mat, type);
}

void __fastcall KeyValues_SetInt(void* rcx, const char* keyName, int value)
{
	UNLOAD_RETURN(KeyValues_SetInt, void, rcx, keyName, value);

	static const auto dwDesired = S::CTFClientScoreBoardDialog_UpdatePlayerList_SetInt_Call();
	static const auto dwJump = S::CTFClientScoreBoardDialog_UpdatePlayerList_Jump();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	CALL_ORIGINAL(KeyValues_SetInt, void, rcx, keyName, value);

	if (Vars::Visuals::UI::RevealScoreboard.Value && dwRetAddr == dwDesired && keyName && FNV1A::Hash32(keyName) == FNV1A::Hash32Const("nemesis"))
		*static_cast<uintptr_t*>(_AddressOfReturnAddress()) = dwJump;
}

int __fastcall NotificationQueue_Add(CEconNotification* pNotification)
{
	UNLOAD_RETURN(NotificationQueue_Add, int, pNotification);

	if (FNV1A::Hash32(pNotification->m_pText) == FNV1A::Hash32Const("TF_HasNewItems") && Vars::Misc::Automation::AcceptItemDrops.Value)
	{
		pNotification->Accept();
		pNotification->Trigger();
		pNotification->UpdateTick();
		pNotification->MarkForDeletion();
		return 0;
	}

	return CALL_ORIGINAL(NotificationQueue_Add, int, pNotification);
}

void __fastcall R_ComputeLightingOrigin(IClientRenderable* pRenderable, studiohdr_t* pStudioHdr, const matrix3x4& matrix, Vector& center)
{
	UNLOAD_RETURN(R_ComputeLightingOrigin, void, pRenderable, pStudioHdr, std::ref(matrix), std::ref(center));

	if (!I::EngineClient->IsInGame() || !I::EngineClient->IsConnected() || !H::Entities.GetLocal())
		return CALL_ORIGINAL(R_ComputeLightingOrigin, void, pRenderable, pStudioHdr, std::ref(matrix), std::ref(center));

	if (auto pEntity = reinterpret_cast<CBaseEntity*>(pRenderable))
	{
		if (auto pOwner = pEntity->m_hOwnerEntity().Get(); pOwner->IsPlayer())
		{
			center = pOwner->GetRenderCenter();
			return;
		}
		else if (pEntity->IsPlayer())
		{
			center = pEntity->GetRenderCenter();
			return;
		}
	}

	CALL_ORIGINAL(R_ComputeLightingOrigin, void, pRenderable, pStudioHdr, std::ref(matrix), std::ref(center));
}

void __fastcall R_DrawSkyBox(float zFar, int nDrawFlags)
{
	UNLOAD_RETURN(R_DrawSkyBox, void, zFar, nDrawFlags);

	if (FNV1A::Hash32(Vars::Visuals::World::SkyboxChanger.Value.c_str()) == FNV1A::Hash32Const("Off") || Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return CALL_ORIGINAL(R_DrawSkyBox, void, zFar, nDrawFlags);

	static auto sv_skyname = U::ConVars.FindVar("sv_skyname");
	std::string sOriginal = sv_skyname->GetString();
	sv_skyname->SetValue(Vars::Visuals::World::SkyboxChanger.Value.c_str());
	CALL_ORIGINAL(R_DrawSkyBox, void, zFar, nDrawFlags);
	sv_skyname->SetValue(sOriginal.c_str());
}

void __fastcall RecvProxy_SimulationTime(const CRecvProxyData* pData, void* pStruct, void* pOut)
{
	UNLOAD_RETURN(RecvProxy_SimulationTime, void, pData, pStruct, pOut);

	auto pEntity = reinterpret_cast<CBaseEntity*>(pStruct);
	if (!pEntity || !pEntity->IsPlayer() || pEntity->entindex() == I::EngineClient->GetLocalPlayer())
		return CALL_ORIGINAL(RecvProxy_SimulationTime, void, pData, pStruct, pOut);

	if (!pData->m_Value.m_Int) // fix setting invalid simtime every 100 ticks if choking
		return;

	int addt = pData->m_Value.m_Int;
	int t = GetNetworkBase(I::GlobalVars->tickcount, pEntity->entindex());
	t += addt;

	while (t < I::GlobalVars->tickcount - 127)
		t += 256;
	while (t > I::GlobalVars->tickcount + 127)
		t -= 256;

	pEntity->m_flSimulationTime() = TICKS_TO_TIME(t);
}

bool __fastcall TF_IsHolidayActive(int eHoliday)
{
	UNLOAD_RETURN(TF_IsHolidayActive, bool, eHoliday);

	static const auto dwDesired = S::CTFPlayer_FireEvent_IsHolidayActive_Call();
	const auto dwRetAddr = uintptr_t(_ReturnAddress());

	return dwRetAddr == dwDesired ? true : CALL_ORIGINAL(TF_IsHolidayActive, bool, eHoliday);
}

static int s_iPlayerIndexVGuiMenuBuilder;
static uint32_t s_uAccountID;
static const char* s_sPlayerName;
bool __fastcall CPlayerResource_IsFakePlayer(void* rcx, int index)
{
	UNLOAD_RETURN(CPlayerResource_IsFakePlayer, bool, rcx, index);

	const auto dwRetAddr = uintptr_t(_ReturnAddress());
	const auto dwDesired = S::CTFClientScoreBoardDialog_OnScoreBoardMouseRightRelease_IsFakePlayer_Call();

	if (dwRetAddr == dwDesired && Vars::Visuals::UI::ScoreboardUtility.Value)
		s_iPlayerIndex = index;

	return CALL_ORIGINAL(CPlayerResource_IsFakePlayer, bool, rcx, index);
}

void* __fastcall VGuiMenuBuilder_AddMenuItem(void* rcx, const char* pszButtonText, const char* pszCommand, const char* pszCategoryName)
{
	UNLOAD_RETURN(VGuiMenuBuilder_AddMenuItem, void*, rcx, pszButtonText, pszCommand, pszCategoryName);

	const auto dwRetAddr = uintptr_t(_ReturnAddress());
	const auto dwDesired1 = S::CTFClientScoreBoardDialog_OnScoreBoardMouseRightRelease_AddMenuItem_CallProfile();
	const auto dwDesired2 = S::CTFClientScoreBoardDialog_OnScoreBoardMouseRightRelease_AddMenuItem_CallSpectate();

	if (dwRetAddr == dwDesired1 && Vars::Visuals::UI::ScoreboardUtility.Value)
	{
		if (auto pResource = H::Entities.GetResource(); pResource && pResource->m_bValid(s_iPlayerIndex))
		{
			auto pReturn = CALL_ORIGINAL(VGuiMenuBuilder_AddMenuItem, void*, rcx, pszButtonText, pszCommand, pszCategoryName);

			s_uAccountID = pResource->m_iAccountID(s_iPlayerIndex);
			s_sPlayerName = pResource->GetName(s_iPlayerIndex);

			CALL_ORIGINAL(VGuiMenuBuilder_AddMenuItem, void*, rcx, "History", "history", "profile");
			CALL_ORIGINAL(VGuiMenuBuilder_AddMenuItem, void*, rcx, I::EngineClient->GetPlayerForUserID(F::Spectate.GetTarget(true)) == s_iPlayerIndex ? "Unspectate" : "Spectate", "specplayer", "profile");

			CALL_ORIGINAL(VGuiMenuBuilder_AddMenuItem, void*, rcx, std::format("Tags for {}", s_sPlayerName).c_str(), "listtags", "tags");
			for (auto it = F::PlayerUtils.m_vTags.begin(); it != F::PlayerUtils.m_vTags.end(); it++)
			{
				int iID = std::distance(F::PlayerUtils.m_vTags.begin(), it);
				auto& tTag = *it;
				if (!tTag.m_bAssignable)
					continue;

				bool bHasTag = F::PlayerUtils.HasTag(s_uAccountID, iID);
				CALL_ORIGINAL(VGuiMenuBuilder_AddMenuItem, void*, rcx, std::format("{} {}", bHasTag ? "Remove" : "Add", tTag.m_sName).c_str(), std::format("modifytag{}", iID).c_str(), "tags");
			}

			return pReturn;
		}
	}

	if (dwRetAddr == dwDesired2 && Vars::Visuals::UI::ScoreboardUtility.Value)
		return nullptr;

	return CALL_ORIGINAL(VGuiMenuBuilder_AddMenuItem, void*, rcx, pszButtonText, pszCommand, pszCategoryName);
}

void __fastcall CTFClientScoreBoardDialog_OnCommand(void* rcx, const char* command)
{
	UNLOAD_RETURN(CTFClientScoreBoardDialog_OnCommand, void, rcx, command);

	if (!Vars::Visuals::UI::ScoreboardUtility.Value || !command)
		return CALL_ORIGINAL(CTFClientScoreBoardDialog_OnCommand, void, rcx, command);

	auto uHash = FNV1A::Hash32(command);
	switch (uHash)
	{
	case FNV1A::Hash32Const("history"):
		I::SteamFriends->ActivateGameOverlayToWebPage(std::format("https://steamhistory.net/id/{}", CSteamID(s_uAccountID, k_EUniversePublic, k_EAccountTypeIndividual).ConvertToUint64()).c_str());
		break;
	case FNV1A::Hash32Const("listtags"):
		F::Output.TagsOnJoin(s_sPlayerName, s_uAccountID);
		break;
	case FNV1A::Hash32Const("specplayer"):
		if (auto pResource = H::Entities.GetResource(); pResource && Vars::Visuals::UI::ScoreboardUtility.Value)
		{
			F::Spectate.SetTarget(pResource->m_iUserID(s_iPlayerIndex));
			break;
		}
		[[fallthrough]];
	default:
		if (strstr(command, "modifytag"))
		{
			try
			{
				std::string sTag = command;
				sTag = sTag.replace(0, strlen("modifytag"), "");
				int iID = std::stoi(sTag);
				if (!F::PlayerUtils.HasTag(s_uAccountID, iID))
					F::PlayerUtils.AddTag(s_uAccountID, iID, true, s_sPlayerName);
				else
					F::PlayerUtils.RemoveTag(s_uAccountID, iID, true, s_sPlayerName);
			}
			catch (...) {}
		}
		CALL_ORIGINAL(CTFClientScoreBoardDialog_OnCommand, void, rcx, command);
	}
}

bool CHooks::Initialize()
{
	INIT_HOOK(bf_read_ReadString, S::bf_read_ReadString());
	INIT_HOOK(CAchievementMgr_CheckAchievementsEnabled, S::CAchievementMgr_CheckAchievementsEnabled());
	INIT_HOOK(CAttributeManager_AttribHookInt, S::CAttributeManager_AttribHookInt());
	INIT_HOOK(CBaseAnimating_Interpolate, S::CBaseAnimating_Interpolate());
	INIT_HOOK(CBaseAnimating_MaintainSequenceTransitions, S::CBaseAnimating_MaintainSequenceTransitions());
	INIT_HOOK(CBaseAnimating_SetSequence, S::CBaseAnimating_SetSequence());
	INIT_HOOK(CBaseAnimating_SetupBones, S::CBaseAnimating_SetupBones());
	INIT_HOOK(CBaseAnimating_UpdateClientSideAnimation, S::CBaseAnimating_UpdateClientSideAnimation());
	INIT_HOOK(CBaseEntity_AddVar, S::CBaseEntity_AddVar());
	INIT_HOOK(CBaseEntity_BaseInterpolatePart1, S::CBaseEntity_BaseInterpolatePart1());
	INIT_HOOK(CBaseEntity_EstimateAbsVelocity, S::CBaseEntity_EstimateAbsVelocity());
	INIT_HOOK(CBaseEntity_InterpolateServerEntities, S::CBaseEntity_InterpolateServerEntities());
	INIT_HOOK(CBaseEntity_ResetLatched, S::CBaseEntity_ResetLatched());
	INIT_HOOK(CBaseEntity_SetAbsVelocity, S::CBaseEntity_SetAbsVelocity());
	INIT_HOOK(CBaseEntity_WorldSpaceCenter, S::CBaseEntity_WorldSpaceCenter());
	//INIT_HOOK(CBaseHudChat_StartMessageMode, S::CBaseHudChat_StartMessageMode());
	INIT_HOOK(CBaseHudChatLine_InsertAndColorizeText, S::CBaseHudChatLine_InsertAndColorizeText());
	INIT_HOOK(CBasePlayer_CalcObserverView, S::CBasePlayer_CalcObserverView());
	INIT_HOOK(CBasePlayer_CalcView, S::CBasePlayer_CalcView());
	INIT_HOOK(CTFPlayer_HandleTaunting, S::CTFPlayer_HandleTaunting());
	INIT_HOOK(CThirdPersonManager_GetFinalCameraOffset, S::CThirdPersonManager_GetFinalCameraOffset());
	INIT_HOOK(CBaseViewModel_CalcViewModelView, S::CBaseViewModel_CalcViewModelView());
	INIT_HOOK(CBasePlayer_CalcViewModelView, S::CBasePlayer_CalcViewModelView());
	INIT_HOOK(CBasePlayer_ItemPostFrame, S::CBasePlayer_ItemPostFrame());
	INIT_HOOK(CBaseViewModel_ShouldFlipViewModel, S::CBaseViewModel_ShouldFlipViewModel());
	INIT_HOOK(Cbuf_ExecuteCommand, S::Cbuf_ExecuteCommand());
	INIT_HOOK(CClientModeShared_DoPostScreenSpaceEffects, U::Memory.GetVirtual(I::ClientModeShared, 39));
	INIT_HOOK(CClientModeShared_OverrideView, U::Memory.GetVirtual(I::ClientModeShared, 16));
	INIT_HOOK(CClientModeShared_ShouldDrawViewModel, U::Memory.GetVirtual(I::ClientModeShared, 24));
	INIT_HOOK(CClientState_GetClientInterpAmount, S::CClientState_GetClientInterpAmount());
	INIT_HOOK(CClientState_ProcessFixAngle, S::CClientState_ProcessFixAngle());
	INIT_HOOK(CGlowObjectManager_RenderGlowEffects, S::CGlowObjectManager_RenderGlowEffects());
	INIT_HOOK(CHLClient_CreateMove, U::Memory.GetVirtual(I::Client, 21));
	INIT_HOOK(CHLClient_DispatchUserMessage, U::Memory.GetVirtual(I::Client, 36));
	INIT_HOOK(CHLClient_FrameStageNotify, U::Memory.GetVirtual(I::Client, 35));
	INIT_HOOK(CHLClient_LevelShutdown, U::Memory.GetVirtual(I::Client, 7));
	INIT_HOOK(CHLTVCamera_CalcView, S::CHLTVCamera_CalcView());
	INIT_HOOK(CHLTVCamera_GetPrimaryTarget, S::CHLTVCamera_GetPrimaryTarget());
	INIT_HOOK(CHLTVCamera_GetMode, S::CHLTVCamera_GetMode());
	INIT_HOOK(CHudChat_GetClientColor, S::CHudChat_GetClientColor());
	INIT_HOOK(CHudCrosshair_GetDrawPosition, S::CHudCrosshair_GetDrawPosition());
	INIT_HOOK(CInput_GetUserCmd, U::Memory.GetVirtual(I::Input, 8));
	INIT_HOOK(CInput_ValidateUserCmd, S::CInput_ValidateUserCmd());
	INIT_HOOK(CInventoryManager_ShowItemsPickedUp, S::CInventoryManager_ShowItemsPickedUp());
	INIT_HOOK(CL_CheckForPureServerWhitelist, S::CL_CheckForPureServerWhitelist());
	INIT_HOOK(CL_Move, S::CL_Move());
	INIT_HOOK(CL_ProcessPacketEntities, S::CL_ProcessPacketEntities());
	INIT_HOOK(CL_ReadPackets, S::CL_ReadPackets());
	INIT_HOOK(ClientModeTFNormal_BIsFriendOrPartyMember, S::ClientModeTFNormal_BIsFriendOrPartyMember());
	INIT_HOOK(CMatchInviteNotification_OnTick, S::CMatchInviteNotification_OnTick());
	INIT_HOOK(CMaterial_Uncache, S::CMaterial_Uncache());
	INIT_HOOK(CNetChannel_SendDatagram, S::CNetChannel_SendDatagram());
	INIT_HOOK(CNetChannel_SendNetMsg, S::CNetChannel_SendNetMsg());
	INIT_HOOK(COPRenderSprites_Render, S::COPRenderSprites_Render());
	INIT_HOOK(CNewParticleEffect_DrawModel, S::CNewParticleEffect_DrawModel());
	INIT_HOOK(CNewParticleEffect_Deconstructor, S::CNewParticleEffect_Deconstructor());
	INIT_HOOK(COPRenderSprites_RenderSpriteCard, S::COPRenderSprites_RenderSpriteCard());
	INIT_HOOK(COPRenderSprites_RenderTwoSequenceSpriteCard, S::COPRenderSprites_RenderTwoSequenceSpriteCard());
	INIT_HOOK(CParticleCollection_Render, S::CParticleCollection_Render());
	INIT_HOOK(CParticleProperty_Create_Name, S::CParticleProperty_Create_Name());
	INIT_HOOK(CParticleProperty_AddControlPoint_Pointer, S::CParticleProperty_AddControlPoint_Pointer());
	//INIT_HOOK(CPhysicsObject_OutputDebugInfo, S::CPhysicsObject_OutputDebugInfo());
	INIT_HOOK(CPlayerResource_GetPlayerName, S::CPlayerResource_GetPlayerName());
#ifdef ANTIAUTOBALANCETESTING
	INIT_HOOK(CPrediction_PostEntityPacketReceived, S::CPrediction_PostEntityPacketReceived());
#endif
	INIT_HOOK(CPrediction_RunSimulation, S::CPrediction_RunSimulation());
	INIT_HOOK(CProxyAnimatedWeaponSheen_OnBind, S::CProxyAnimatedWeaponSheen_OnBind());
	INIT_HOOK(CRendering3dView_EnableWorldFog, S::CRendering3dView_EnableWorldFog());
	INIT_HOOK(CSequenceTransitioner_CheckForSequenceChange, S::CSequenceTransitioner_CheckForSequenceChange());
	INIT_HOOK(CSkyboxView_Enable3dSkyboxFog, S::CSkyboxView_Enable3dSkyboxFog());
	INIT_HOOK(CSniperDot_ClientThink, S::CSniperDot_ClientThink());
	INIT_HOOK(CSniperDot_GetRenderingPositions, S::CSniperDot_GetRenderingPositions());
	INIT_HOOK(CBasePlayer_EyePosition, S::CBasePlayer_EyePosition());
	INIT_HOOK(CTFPlayer_EyeAngles, S::CTFPlayer_EyeAngles());
	INIT_HOOK(CSoundEmitterSystem_EmitSound, S::CSoundEmitterSystem_EmitSound());
	INIT_HOOK(CBaseEntity_EmitSound, S::CBaseEntity_EmitSound());
	//INIT_HOOK(S_StartDynamicSound, S::S_StartDynamicSound());
	INIT_HOOK(S_StartSound, S::S_StartSound());
	//INIT_HOOK(CSpriteTrail_DrawModel, S::CSpriteTrail_DrawModel());
	INIT_HOOK(CStaticPropMgr_ComputePropOpacity, S::CStaticPropMgr_ComputePropOpacity());
	INIT_HOOK(CStaticPropMgr_DrawStaticProps, S::CStaticPropMgr_DrawStaticProps());
	INIT_HOOK(CStudioRender_SetColorModulation, U::Memory.GetVirtual(I::StudioRender, 27));
	INIT_HOOK(CStudioRender_SetAlphaModulation, U::Memory.GetVirtual(I::StudioRender, 28));
	//INIT_HOOK(CStudioRender_DrawModelStaticProp, U::Memory.GetVirtual(I::StudioRender, 30));
	INIT_HOOK(CTFBadgePanel_SetupBadge, S::CTFBadgePanel_SetupBadge());
	INIT_HOOK(CTFClientScoreBoardDialog_UpdatePlayerAvatar, S::CTFClientScoreBoardDialog_UpdatePlayerAvatar());
	INIT_HOOK(CTFMatchSummary_UpdatePlayerAvatar, S::CTFMatchSummary_UpdatePlayerAvatar());
	INIT_HOOK(CTFHudMannVsMachineScoreboard_UpdatePlayerAvatar, S::CTFHudMannVsMachineScoreboard_UpdatePlayerAvatar());
	INIT_HOOK(CTFHudMatchStatus_UpdatePlayerAvatar, S::CTFHudMatchStatus_UpdatePlayerAvatar());
	INIT_HOOK(SectionedListPanel_SetItemFgColor, S::SectionedListPanel_SetItemFgColor());
	INIT_HOOK(CTFGCClientSystem_UpdateAssignedLobby, S::CTFGCClientSystem_UpdateAssignedLobby());
	INIT_HOOK(CTFInput_ApplyMouse, S::CTFInput_ApplyMouse());
	INIT_HOOK(CTFInput_CAM_CapYaw, S::CTFInput_CAM_CapYaw());
	INIT_HOOK(CTFPlayer_AvoidPlayers, S::CTFPlayer_AvoidPlayers());
	INIT_HOOK(CTFPlayer_BRenderAsZombie, S::CTFPlayer_BRenderAsZombie());
	INIT_HOOK(CTFPlayer_BuildTransformations, S::CTFPlayer_BuildTransformations());
	INIT_HOOK(CTFPlayer_ClientAdjustVOPitch, S::CTFPlayer_ClientAdjustVOPitch());
	INIT_HOOK(CTFPlayer_DoAnimationEvent, S::CTFPlayer_DoAnimationEvent());
	INIT_HOOK(CTFPlayer_FireBullet, S::CTFPlayer_FireBullet());
	INIT_HOOK(CTFPlayer_GetMinFOV, S::CTFPlayer_GetMinFOV());
	INIT_HOOK(CTFPlayer_InSameDisguisedTeam, S::CTFPlayer_InSameDisguisedTeam());
	INIT_HOOK(CTFFreezePanel_ShouldDraw, S::CTFFreezePanel_ShouldDraw());
	INIT_HOOK(CTFFreezePanel_FireGameEvent, S::CTFFreezePanel_FireGameEvent());
	INIT_HOOK(CTFPlayer_IsPlayerClass, S::CTFPlayer_IsPlayerClass());
	INIT_HOOK(CTFPlayer_ShouldDraw, S::CTFPlayer_ShouldDraw());
	INIT_HOOK(CBasePlayer_ShouldDrawThisPlayer, S::CBasePlayer_ShouldDrawThisPlayer());
	INIT_HOOK(CBasePlayer_ShouldDrawLocalPlayer, S::CBasePlayer_ShouldDrawLocalPlayer());
	INIT_HOOK(CBaseCombatWeapon_ShouldDraw, S::CBaseCombatWeapon_ShouldDraw());
	INIT_HOOK(CViewRender_DrawViewModels, S::CViewRender_DrawViewModels());
	INIT_HOOK(CTFPlayer_UpdateStepSound, S::CTFPlayer_UpdateStepSound());
	INIT_HOOK(CTFPlayerInventory_GetMaxItemCount, S::CTFPlayerInventory_GetMaxItemCount());
	INIT_HOOK(CTFPlayerInventory_VerifyChangedLoadoutsAreValid, S::CTFPlayerInventory_VerifyChangedLoadoutsAreValid());
	INIT_HOOK(GenerateEquipRegionConflictMask, S::GenerateEquipRegionConflictMask());
	INIT_HOOK(CTFInventoryManager_GetItemInLoadoutForClass, S::CTFInventoryManager_GetItemInLoadoutForClass());
	INIT_HOOK(CTFPlayerPanel_GetTeam, S::CTFPlayerPanel_GetTeam());
	INIT_HOOK(CTFTeamStatusPlayerPanel_Update, S::CTFTeamStatusPlayerPanel_Update());
	INIT_HOOK(VGui_Panel_SetFgColor, S::VGui_Panel_SetFgColor());
	INIT_HOOK(VGui_Panel_SetBgColor, S::VGui_Panel_SetBgColor());
	INIT_HOOK(CTFPlayerShared_InCond, S::CTFPlayerShared_InCond());
	INIT_HOOK(CTFPlayerShared_IsCritBoosted, S::CTFPlayerShared_IsCritBoosted());
	INIT_HOOK(CTFConditionList_InCond, S::CTFConditionList_InCond());
	INIT_HOOK(CTFPlayerShared_IsPlayerDominated, S::CTFPlayerShared_IsPlayerDominated());
	INIT_HOOK(CTFPlayerShared_ShouldSuppressPrediction, S::CTFPlayerShared_ShouldSuppressPrediction());
	INIT_HOOK(CTFRagdoll_CreateTFRagdoll, S::CTFRagdoll_CreateTFRagdoll());
	INIT_HOOK(CTFRocketLauncher_CheckReloadMisfire, S::CTFRocketLauncher_CheckReloadMisfire());
	INIT_HOOK(CTFRocketLauncher_FireProjectile, S::CTFRocketLauncher_FireProjectile());
	//INIT_HOOK(CTFBat_Wood_LaunchBall, S::CTFBat_Wood_LaunchBall());
	INIT_HOOK(CBaseEntity_ApplyAbsVelocityImpulse, S::CBaseEntity_ApplyAbsVelocityImpulse());
	INIT_HOOK(CTFScattergun_FireBullet, S::CTFScattergun_FireBullet());
	INIT_HOOK(CTFGameMovement_SetGroundEntity, U::Memory.GetVirtual(I::GameMovement, 21));
	INIT_HOOK(CTFWeaponBase_CalcIsAttackCritical, S::CTFWeaponBase_CalcIsAttackCritical());
	INIT_HOOK(CTFWeaponBase_CanFireRandomCriticalShot, S::CTFWeaponBase_CanFireRandomCriticalShot());
	INIT_HOOK(CTFWeaponBase_GetShootSound, S::CTFWeaponBase_GetShootSound());
	INIT_HOOK(CThirdPersonManager_Update, S::CThirdPersonManager_Update());
	INIT_HOOK(CViewRender_DrawUnderwaterOverlay, S::CViewRender_DrawUnderwaterOverlay());
	INIT_HOOK(CViewRender_LevelInit, U::Memory.GetVirtual(I::ViewRender, 1));
	INIT_HOOK(CViewRender_PerformScreenOverlay, S::CViewRender_PerformScreenOverlay());
	INIT_HOOK(CViewRender_RenderView, U::Memory.GetVirtual(I::ViewRender, 6));
	INIT_HOOK(CWeaponMedigun_PrimaryAttack, S::CWeaponMedigun_PrimaryAttack());
	WndProc::Initialize();
	INIT_HOOK(Direct3DDevice9_Present, U::Memory.GetVirtual(I::DirectXDevice, 17));
	INIT_HOOK(Direct3DDevice9_Reset, U::Memory.GetVirtual(I::DirectXDevice, 16));
	INIT_HOOK(VGuiSurface_LockCursor, U::Memory.GetVirtual(I::MatSystemSurface, 62));
	INIT_HOOK(VGuiSurface_SetCursor, U::Memory.GetVirtual(I::MatSystemSurface, 51));
	INIT_HOOK(DoEnginePostProcessing, S::DoEnginePostProcessing());
	INIT_HOOK(DSP_Process, S::DSP_Process());
	INIT_HOOK(FX_FireBullets, S::FX_FireBullets());
#ifdef SEEDPRED_DEBUG
	INIT_HOOK(FX_FireBullets_Server, S::FX_FireBullets_Server());
	INIT_HOOK(CBasePlayer_ProcessUsercmds, S::CBasePlayer_ProcessUsercmds());
#endif
	INIT_HOOK(GetClientInterpAmount, S::GetClientInterpAmount());
	INIT_HOOK(HostState_Shutdown, S::HostState_Shutdown());
	INIT_HOOK(HostState_Restart, S::HostState_Restart());
	INIT_HOOK(IEngineTrace_TraceRay, U::Memory.GetVirtual(I::EngineTrace, 4));
	INIT_HOOK(CM_BoxTrace, S::CM_BoxTrace());
	INIT_HOOK(CM_ClipBoxToBrush_True, S::CM_ClipBoxToBrush_True());
	INIT_HOOK(CM_ClipBoxToBrush_False, S::CM_ClipBoxToBrush_False());
	INIT_HOOK(CEngineTrace_ClipTraceToTrace, S::CEngineTrace_ClipTraceToTrace());
	INIT_HOOK(IEngineVGui_Paint, U::Memory.GetVirtual(I::EngineVGui, 14));
	INIT_HOOK(IMaterialSystem_FindTexture, U::Memory.GetVirtual(I::MaterialSystem, 79));
	INIT_HOOK(IMatSystemSurface_OnScreenSizeChanged, U::Memory.GetVirtual(I::MatSystemSurface, 111));
	INIT_HOOK(IPanel_PaintTraverse, U::Memory.GetVirtual(I::Panel, 41));
	INIT_HOOK(ISteamFriends_GetFriendPersonaName, U::Memory.GetVirtual(I::SteamFriends, 7));
	INIT_HOOK(ISteamNetworkingUtils_GetPingToDataCenter, U::Memory.GetVirtual(I::SteamNetworkingUtils, 8));
	INIT_HOOK(CTFPartyClient_RequestQueueForMatch, S::CTFPartyClient_RequestQueueForMatch());
	INIT_HOOK(IVModelRender_DrawModelExecute, U::Memory.GetVirtual(I::ModelRender, 19));
	INIT_HOOK(CBaseAnimating_DrawModel, S::CBaseAnimating_DrawModel());
	INIT_HOOK(CBaseAnimating_InternalDrawModel, S::CBaseAnimating_InternalDrawModel());
	INIT_HOOK(IVModelRender_ForcedMaterialOverride, U::Memory.GetVirtual(I::ModelRender, 1));
	INIT_HOOK(KeyValues_SetInt, S::KeyValues_SetInt());
	INIT_HOOK(NotificationQueue_Add, S::NotificationQueue_Add());
	INIT_HOOK(R_ComputeLightingOrigin, S::R_ComputeLightingOrigin());
	INIT_HOOK(R_DrawSkyBox, S::R_DrawSkyBox());
	INIT_HOOK(RecvProxy_SimulationTime, S::RecvProxy_SimulationTime());
	INIT_HOOK(TF_IsHolidayActive, S::TF_IsHolidayActive());
	INIT_HOOK(CPlayerResource_IsFakePlayer, S::CPlayerResource_IsFakePlayer());
	INIT_HOOK(VGuiMenuBuilder_AddMenuItem, S::VGuiMenuBuilder_AddMenuItem());
	INIT_HOOK(CTFClientScoreBoardDialog_OnCommand, S::CTFClientScoreBoardDialog_OnCommand());

	return true;
}

#pragma warning(disable: 6031)
bool CHooks::Unload()
{
	UNLOAD_HOOK(bf_read_ReadString);
	UNLOAD_HOOK(CAchievementMgr_CheckAchievementsEnabled);
	UNLOAD_HOOK(CAttributeManager_AttribHookInt);
	UNLOAD_HOOK(CBaseAnimating_Interpolate);
	UNLOAD_HOOK(CBaseAnimating_MaintainSequenceTransitions);
	UNLOAD_HOOK(CBaseAnimating_SetSequence);
	UNLOAD_HOOK(CBaseAnimating_SetupBones);
	UNLOAD_HOOK(CBaseAnimating_UpdateClientSideAnimation);
	UNLOAD_HOOK(CBaseEntity_AddVar);
	UNLOAD_HOOK(CBaseEntity_BaseInterpolatePart1);
	UNLOAD_HOOK(CBaseEntity_EstimateAbsVelocity);
	UNLOAD_HOOK(CBaseEntity_InterpolateServerEntities);
	UNLOAD_HOOK(CBaseEntity_ResetLatched);
	UNLOAD_HOOK(CBaseEntity_SetAbsVelocity);
	UNLOAD_HOOK(CBaseEntity_WorldSpaceCenter);
	//UNLOAD_HOOK(CBaseHudChat_StartMessageMode);
	UNLOAD_HOOK(CBaseHudChatLine_InsertAndColorizeText);
	UNLOAD_HOOK(CBasePlayer_CalcObserverView);
	UNLOAD_HOOK(CBasePlayer_CalcView);
	UNLOAD_HOOK(CTFPlayer_HandleTaunting);
	UNLOAD_HOOK(CThirdPersonManager_GetFinalCameraOffset);
	UNLOAD_HOOK(CBaseViewModel_CalcViewModelView);
	UNLOAD_HOOK(CBasePlayer_CalcViewModelView);
	UNLOAD_HOOK(CBasePlayer_ItemPostFrame);
	UNLOAD_HOOK(CBaseViewModel_ShouldFlipViewModel);
	UNLOAD_HOOK(Cbuf_ExecuteCommand);
	UNLOAD_HOOK(CClientModeShared_DoPostScreenSpaceEffects);
	UNLOAD_HOOK(CClientModeShared_OverrideView);
	UNLOAD_HOOK(CClientModeShared_ShouldDrawViewModel);
	UNLOAD_HOOK(CClientState_GetClientInterpAmount);
	UNLOAD_HOOK(CClientState_ProcessFixAngle);
	UNLOAD_HOOK(CGlowObjectManager_RenderGlowEffects);
	UNLOAD_HOOK(CHLClient_CreateMove);
	UNLOAD_HOOK(CHLClient_DispatchUserMessage);
	UNLOAD_HOOK(CHLClient_FrameStageNotify);
	UNLOAD_HOOK(CHLClient_LevelShutdown);
	UNLOAD_HOOK(CHLTVCamera_CalcView);
	UNLOAD_HOOK(CHLTVCamera_GetPrimaryTarget);
	UNLOAD_HOOK(CHLTVCamera_GetMode);
	UNLOAD_HOOK(CHudChat_GetClientColor);
	UNLOAD_HOOK(CHudCrosshair_GetDrawPosition);
	UNLOAD_HOOK(CInput_GetUserCmd);
	UNLOAD_HOOK(CInput_ValidateUserCmd);
	UNLOAD_HOOK(CInventoryManager_ShowItemsPickedUp);
	UNLOAD_HOOK(CL_CheckForPureServerWhitelist);
	UNLOAD_HOOK(CL_Move);
	UNLOAD_HOOK(CL_ProcessPacketEntities);
	UNLOAD_HOOK(CL_ReadPackets);
	UNLOAD_HOOK(ClientModeTFNormal_BIsFriendOrPartyMember);
	UNLOAD_HOOK(CMatchInviteNotification_OnTick);
	UNLOAD_HOOK(CMaterial_Uncache);
	UNLOAD_HOOK(CNetChannel_SendDatagram);
	UNLOAD_HOOK(CNetChannel_SendNetMsg);
	UNLOAD_HOOK(COPRenderSprites_Render);
	UNLOAD_HOOK(CNewParticleEffect_DrawModel);
	UNLOAD_HOOK(CNewParticleEffect_Deconstructor);
	UNLOAD_HOOK(COPRenderSprites_RenderSpriteCard);
	UNLOAD_HOOK(COPRenderSprites_RenderTwoSequenceSpriteCard);
	UNLOAD_HOOK(CParticleCollection_Render);
	UNLOAD_HOOK(CParticleProperty_Create_Name);
	UNLOAD_HOOK(CParticleProperty_AddControlPoint_Pointer);
	//UNLOAD_HOOK(CPhysicsObject_OutputDebugInfo);
	UNLOAD_HOOK(CPlayerResource_GetPlayerName);
#ifdef ANTIAUTOBALANCETESTING
	UNLOAD_HOOK(CPrediction_PostEntityPacketReceived);
#endif
	UNLOAD_HOOK(CPrediction_RunSimulation);
	UNLOAD_HOOK(CProxyAnimatedWeaponSheen_OnBind);
	UNLOAD_HOOK(CRendering3dView_EnableWorldFog);
	UNLOAD_HOOK(CSequenceTransitioner_CheckForSequenceChange);
	UNLOAD_HOOK(CSkyboxView_Enable3dSkyboxFog);
	UNLOAD_HOOK(CSniperDot_ClientThink);
	UNLOAD_HOOK(CSniperDot_GetRenderingPositions);
	UNLOAD_HOOK(CBasePlayer_EyePosition);
	UNLOAD_HOOK(CTFPlayer_EyeAngles);
	UNLOAD_HOOK(CSoundEmitterSystem_EmitSound);
	UNLOAD_HOOK(CBaseEntity_EmitSound);
	//UNLOAD_HOOK(S_StartDynamicSound);
	UNLOAD_HOOK(S_StartSound);
	//UNLOAD_HOOK(CSpriteTrail_DrawModel);
	UNLOAD_HOOK(CStaticPropMgr_ComputePropOpacity);
	UNLOAD_HOOK(CStaticPropMgr_DrawStaticProps);
	UNLOAD_HOOK(CStudioRender_SetColorModulation);
	UNLOAD_HOOK(CStudioRender_SetAlphaModulation);
	//UNLOAD_HOOK(CStudioRender_DrawModelStaticProp);
	UNLOAD_HOOK(CTFBadgePanel_SetupBadge);
	UNLOAD_HOOK(CTFClientScoreBoardDialog_UpdatePlayerAvatar);
	UNLOAD_HOOK(CTFMatchSummary_UpdatePlayerAvatar);
	UNLOAD_HOOK(CTFHudMannVsMachineScoreboard_UpdatePlayerAvatar);
	UNLOAD_HOOK(CTFHudMatchStatus_UpdatePlayerAvatar);
	UNLOAD_HOOK(SectionedListPanel_SetItemFgColor);
	UNLOAD_HOOK(CTFGCClientSystem_UpdateAssignedLobby);
	UNLOAD_HOOK(CTFInput_ApplyMouse);
	UNLOAD_HOOK(CTFInput_CAM_CapYaw);
	UNLOAD_HOOK(CTFPlayer_AvoidPlayers);
	UNLOAD_HOOK(CTFPlayer_BRenderAsZombie);
	UNLOAD_HOOK(CTFPlayer_BuildTransformations);
	UNLOAD_HOOK(CTFPlayer_ClientAdjustVOPitch);
	UNLOAD_HOOK(CTFPlayer_DoAnimationEvent);
	UNLOAD_HOOK(CTFPlayer_FireBullet);
	UNLOAD_HOOK(CTFPlayer_GetMinFOV);
	UNLOAD_HOOK(CTFPlayer_InSameDisguisedTeam);
	UNLOAD_HOOK(CTFFreezePanel_ShouldDraw);
	UNLOAD_HOOK(CTFFreezePanel_FireGameEvent);
	UNLOAD_HOOK(CTFPlayer_IsPlayerClass);
	UNLOAD_HOOK(CTFPlayer_ShouldDraw);
	UNLOAD_HOOK(CBasePlayer_ShouldDrawThisPlayer);
	UNLOAD_HOOK(CBasePlayer_ShouldDrawLocalPlayer);
	UNLOAD_HOOK(CBaseCombatWeapon_ShouldDraw);
	UNLOAD_HOOK(CViewRender_DrawViewModels);
	UNLOAD_HOOK(CTFPlayer_UpdateStepSound);
	UNLOAD_HOOK(CTFPlayerInventory_GetMaxItemCount);
	UNLOAD_HOOK(CTFPlayerInventory_VerifyChangedLoadoutsAreValid);
	UNLOAD_HOOK(GenerateEquipRegionConflictMask);
	UNLOAD_HOOK(CTFInventoryManager_GetItemInLoadoutForClass);
	UNLOAD_HOOK(CTFPlayerPanel_GetTeam);
	UNLOAD_HOOK(CTFTeamStatusPlayerPanel_Update);
	UNLOAD_HOOK(VGui_Panel_SetFgColor);
	UNLOAD_HOOK(VGui_Panel_SetBgColor);
	UNLOAD_HOOK(CTFPlayerShared_InCond);
	UNLOAD_HOOK(CTFPlayerShared_IsCritBoosted);
	UNLOAD_HOOK(CTFConditionList_InCond);
	UNLOAD_HOOK(CTFPlayerShared_IsPlayerDominated);
	UNLOAD_HOOK(CTFPlayerShared_ShouldSuppressPrediction);
	UNLOAD_HOOK(CTFRagdoll_CreateTFRagdoll);
	UNLOAD_HOOK(CTFRocketLauncher_CheckReloadMisfire);
	UNLOAD_HOOK(CTFRocketLauncher_CheckReloadMisfire);
	UNLOAD_HOOK(CTFRocketLauncher_FireProjectile);
	//UNLOAD_HOOK(CTFBat_Wood_LaunchBall);
	UNLOAD_HOOK(CBaseEntity_ApplyAbsVelocityImpulse);
	UNLOAD_HOOK(CBaseEntity_ApplyAbsVelocityImpulse);
	UNLOAD_HOOK(CTFScattergun_FireBullet);
	UNLOAD_HOOK(CTFGameMovement_SetGroundEntity);
	UNLOAD_HOOK(CTFWeaponBase_CalcIsAttackCritical);
	UNLOAD_HOOK(CTFWeaponBase_CanFireRandomCriticalShot);
	UNLOAD_HOOK(CTFWeaponBase_GetShootSound);
	UNLOAD_HOOK(CThirdPersonManager_Update);
	UNLOAD_HOOK(CViewRender_DrawUnderwaterOverlay);
	UNLOAD_HOOK(CViewRender_LevelInit);
	UNLOAD_HOOK(CViewRender_PerformScreenOverlay);
	UNLOAD_HOOK(CViewRender_RenderView);
	UNLOAD_HOOK(CWeaponMedigun_PrimaryAttack);
	UNLOAD_HOOK(Direct3DDevice9_Present);
	UNLOAD_HOOK(Direct3DDevice9_Reset);
	UNLOAD_HOOK(VGuiSurface_LockCursor);
	UNLOAD_HOOK(VGuiSurface_SetCursor);
	UNLOAD_HOOK(DoEnginePostProcessing);
	UNLOAD_HOOK(DSP_Process);
	UNLOAD_HOOK(FX_FireBullets);
#ifdef SEEDPRED_DEBUG
	UNLOAD_HOOK(FX_FireBullets_Server);
	UNLOAD_HOOK(CBasePlayer_ProcessUsercmds);
#endif
	UNLOAD_HOOK(GetClientInterpAmount);
	UNLOAD_HOOK(HostState_Shutdown);
	UNLOAD_HOOK(HostState_Restart);
	UNLOAD_HOOK(IEngineTrace_TraceRay);
	UNLOAD_HOOK(CM_BoxTrace);
	UNLOAD_HOOK(CM_ClipBoxToBrush_True);
	UNLOAD_HOOK(CM_ClipBoxToBrush_False);
	UNLOAD_HOOK(CEngineTrace_ClipTraceToTrace);
	UNLOAD_HOOK(IEngineVGui_Paint);
	UNLOAD_HOOK(IMaterialSystem_FindTexture);
	UNLOAD_HOOK(IMatSystemSurface_OnScreenSizeChanged);
	UNLOAD_HOOK(IPanel_PaintTraverse);
	UNLOAD_HOOK(ISteamFriends_GetFriendPersonaName);
	UNLOAD_HOOK(ISteamNetworkingUtils_GetPingToDataCenter);
	UNLOAD_HOOK(CTFPartyClient_RequestQueueForMatch);
	UNLOAD_HOOK(IVModelRender_DrawModelExecute);
	UNLOAD_HOOK(CBaseAnimating_DrawModel);
	UNLOAD_HOOK(CBaseAnimating_InternalDrawModel);
	UNLOAD_HOOK(IVModelRender_ForcedMaterialOverride);
	UNLOAD_HOOK(KeyValues_SetInt);
	UNLOAD_HOOK(NotificationQueue_Add);
	UNLOAD_HOOK(R_ComputeLightingOrigin);
	UNLOAD_HOOK(R_DrawSkyBox);
	UNLOAD_HOOK(RecvProxy_SimulationTime);
	UNLOAD_HOOK(TF_IsHolidayActive);
	UNLOAD_HOOK(CPlayerResource_IsFakePlayer);
	UNLOAD_HOOK(VGuiMenuBuilder_AddMenuItem);
	UNLOAD_HOOK(CTFClientScoreBoardDialog_OnCommand);

	WndProc::Unload();

	return true;
}