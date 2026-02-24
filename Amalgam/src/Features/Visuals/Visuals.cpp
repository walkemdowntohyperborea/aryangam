#include "Visuals.h"

#include "../Aimbot/Aimbot.h"
#include "../Aimbot/AimbotProjectile/AimbotProjectile.h"
#include "../Ticks/Ticks.h"
#include "../Backtrack/Backtrack.h"
#include "../PacketManip/AntiAim/AntiAim.h"
#include "../Simulation/ProjectileSimulation/ProjectileSimulation.h"
#include "CameraWindow/CameraWindow.h"
#include "../Players/PlayerUtils.h"
#include "../Spectate/Spectate.h"
#include "Groups/Groups.h"
#include "../CritHack/CritHack.h"
#include "Icons.h"

#include <queue>
#include <unordered_set>

MAKE_SIGNATURE(CBaseAnimating_DrawServerHitboxes, "server.dll", "44 88 44 24 ? 53 48 81 EC", 0x0);
MAKE_SIGNATURE(UTIL_PlayerByIndex, "server.dll", "48 83 EC ? 8B D1 85 C9 7E ? 48 8B 05", 0x0);

static std::vector<Vec3> SplashTrace(Vec3 vOrigin, float flRadius, Vec3 vNormal = { 0, 0, 1 }, bool bTrace = true, int iSegments = 100)
{
	if (!flRadius)
		return {};

	Vec3 vAngles = Math::VectorAngles(vNormal);
	Vec3 vRight, vUp; Math::AngleVectors(vAngles, nullptr, &vRight, &vUp);

	std::vector<Vec3> vPoints = {};
	for (float i = 0.f; i < iSegments; i++)
	{
		Vec3 vPoint = vOrigin + (vRight * cos(2 * PI * i / iSegments) + vUp * sin(2 * PI * i / iSegments)) * flRadius;
		if (bTrace)
		{
			CGameTrace trace = {};
			CTraceFilterWorldAndPropsOnly filter = {};
			SDK::Trace(vOrigin, vPoint, MASK_SHOT, &filter, &trace);
			vPoint = trace.endpos;
		}
		vPoints.push_back(vPoint);
	}
	vPoints.push_back(vPoints.front());

	return vPoints;
}

void CVisuals::ProjectileTrace(CTFPlayer* pPlayer, CTFWeaponBase* pWeapon, const bool bInterp)
{
	if (bInterp)
		F::CameraWindow.m_bShouldDraw = false;
	if (bInterp ? !Vars::Visuals::Simulation::TrajectoryPath.Value && !Vars::Visuals::Simulation::ProjectileCamera.Value : !Vars::Visuals::Simulation::ShotPath.Value)
		return;

	Vec3 vAngles = bInterp ? I::EngineClient->GetViewAngles() : G::CurrentUserCmd->viewangles;
	int iFlags = bInterp ? ProjSimEnum::Trace | ProjSimEnum::InitCheck | ProjSimEnum::Interp : ProjSimEnum::Trace | ProjSimEnum::InitCheck;
	if (F::Spectate.HasTarget() && bInterp && pPlayer && pPlayer->m_hObserverTarget())
	{
		bool bThirdperson = pPlayer->m_iObserverMode() != OBS_MODE_FIRSTPERSON;
		pPlayer = I::ClientEntityList->GetClientEntity(I::EngineClient->GetPlayerForUserID(F::Spectate.GetTarget()))->As<CTFPlayer>();
		if (!pPlayer || pPlayer->IsDormant())
			return;

		pWeapon = pPlayer->m_hActiveWeapon()->As<CTFWeaponBase>();
		if (!pWeapon || pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER)
			return;

		pPlayer->m_vecViewOffset() = pPlayer->GetViewOffset();
		if(bThirdperson)
			vAngles = pPlayer->GetEyeAngles();
	}
	else if (!pPlayer || !pWeapon || pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER)
		return;

	ProjectileInfo tProjInfo = {};
	if (!F::ProjSim.GetInfo(pPlayer, pWeapon, vAngles, tProjInfo, iFlags, (bInterp && Vars::Aimbot::Projectile::AutoRelease.Value) ? Vars::Aimbot::Projectile::AutoRelease.Value / 100 : -1.f)
		|| !F::ProjSim.Initialize(tProjInfo))
		return;

	CGameTrace trace = {};
	CTraceFilterCollideable filter = {}; 
	filter.pSkip = pPlayer;
	int nMask = MASK_SOLID;
	F::ProjSim.SetupTrace(filter, nMask, pWeapon, 0, bInterp);

	Vec3* pNormal = nullptr;

	SDK::TraceHull(F::ProjSim.GetOrigin(), F::ProjSim.GetOrigin(), tProjInfo.m_vHull * -1, tProjInfo.m_vHull, nMask, &filter, &trace);
	if (trace.startsolid)
		return;

	int iTicks = TIME_TO_TICKS(std::min(tProjInfo.m_flLifetime, 10.f));
	for (int n = 1; n <= iTicks; n++)
	{
		Vec3 Old = F::ProjSim.GetOrigin();
		F::ProjSim.RunTick(tProjInfo);
		Vec3 New = F::ProjSim.GetOrigin();

		SDK::TraceHull(Old, New, tProjInfo.m_vHull * -1, tProjInfo.m_vHull, nMask, &filter, &trace);
		F::ProjSim.SetupTrace(filter, nMask, pWeapon, n, bInterp);
		if (trace.DidHit())
		{
			pNormal = &trace.plane.normal;
			break;
		}
	}
	if (tProjInfo.m_vPath.empty())
		return;

	tProjInfo.m_vPath.push_back(trace.endpos);

	std::vector<Vec3> vPoints = {};
	if ((bInterp ? Vars::Visuals::Simulation::TrajectoryPath.Value : Vars::Visuals::Simulation::ShotPath.Value) && Vars::Visuals::Simulation::SplashRadius.Value & Vars::Visuals::Simulation::SplashRadiusEnum::Simulation)
	{
		float flRadius = 0.f;
		switch (pWeapon->GetWeaponID())
		{
		case TF_WEAPON_ROCKETLAUNCHER:
		case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
		case TF_WEAPON_PARTICLE_CANNON:
			if (Vars::Visuals::Simulation::SplashRadius.Value & Vars::Visuals::Simulation::SplashRadiusEnum::Rockets)
				flRadius = TF_ROCKET_RADIUS;
			break;
		case TF_WEAPON_PIPEBOMBLAUNCHER:
			if (Vars::Visuals::Simulation::SplashRadius.Value & Vars::Visuals::Simulation::SplashRadiusEnum::Stickies)
				flRadius = TF_ROCKET_RADIUS;
			break;
		case TF_WEAPON_GRENADELAUNCHER:
			if (Vars::Visuals::Simulation::SplashRadius.Value & Vars::Visuals::Simulation::SplashRadiusEnum::Pipes)
				flRadius = TF_ROCKET_RADIUS;
			break;
		case TF_WEAPON_FLAREGUN:
		case TF_WEAPON_FLAREGUN_REVENGE:
			if (Vars::Visuals::Simulation::SplashRadius.Value & Vars::Visuals::Simulation::SplashRadiusEnum::ScorchShot && pWeapon->As<CTFFlareGun>()->GetFlareGunType() == FLAREGUN_SCORCHSHOT)
				flRadius = TF_FLARE_DET_RADIUS;
		}

		if (flRadius)
		{
			Vec3 vEndPos = trace.endpos;
			flRadius = SDK::AttribHookValue(flRadius, "mult_explosion_radius", pWeapon);
			switch (pWeapon->GetWeaponID())
			{
			case TF_WEAPON_ROCKETLAUNCHER:
			case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
			case TF_WEAPON_PARTICLE_CANNON:
				if (pNormal)
					vEndPos += *pNormal;
				if (pPlayer->InCond(TF_COND_BLASTJUMPING) && SDK::AttribHookValue(1.f, "rocketjump_attackrate_bonus", pWeapon) != 1.f)
					flRadius *= 0.8f;
			}
			H::Draw.RenderWireframeSphere(trace.endpos, flRadius, 36, 36, Vars::Colors::SplashRadius.Value, false);
			H::Draw.RenderWireframeSphere(trace.endpos, flRadius, 36, 36, Vars::Colors::SplashRadiusIgnoreZ.Value, true);
		}
	}

	if (bInterp)
	{
		if (Vars::Visuals::Simulation::ProjectileCamera.Value && !I::EngineVGui->IsGameUIVisible() && pPlayer->m_vecOrigin().DistTo(trace.endpos) > 500.f)
		{
			CGameTrace cameraTrace = {};

			auto vAngles = Math::CalcAngle(trace.startpos, trace.endpos);
			Vec3 vForward; Math::AngleVectors(vAngles, &vForward);
			SDK::Trace(trace.endpos, trace.endpos - vForward * 500.f, MASK_SOLID, &filter, &cameraTrace);

			F::CameraWindow.m_bShouldDraw = true;
			F::CameraWindow.m_vCameraOrigin = cameraTrace.endpos;
			F::CameraWindow.m_vCameraAngles = vAngles;
		}

		if (Vars::Visuals::Simulation::TrajectoryPath.Value)
		{
			H::Draw.RenderPath(tProjInfo.m_vPath, Vars::Colors::TrajectoryPath.Value, false, Vars::Visuals::Simulation::TrajectoryPath.Value);
			H::Draw.RenderPath(tProjInfo.m_vPath, Vars::Colors::TrajectoryPathIgnoreZ.Value, true, Vars::Visuals::Simulation::TrajectoryPath.Value);

			if (Vars::Visuals::Simulation::Box.Value && pNormal)
			{
				const float flSize = std::max(tProjInfo.m_vHull.Min(), 1.f);
				const Vec3 vSize = { 1.f, flSize, flSize };
				Vec3 vAngles = Math::VectorAngles(*pNormal);

				H::Draw.RenderWireframeBox(trace.endpos, vSize * -1, vSize, vAngles, Vars::Colors::TrajectoryPath.Value);
				H::Draw.RenderWireframeBox(trace.endpos, vSize * -1, vSize, vAngles, Vars::Colors::TrajectoryPathIgnoreZ.Value, true);
			}

			if (!vPoints.empty())
			{
				H::Draw.RenderPath(vPoints, Vars::Colors::SplashRadius.Value, false, Vars::Visuals::Simulation::StyleEnum::Line);
				H::Draw.RenderPath(vPoints, Vars::Colors::SplashRadiusIgnoreZ.Value, true, Vars::Visuals::Simulation::StyleEnum::Line);
			}
		}
	}
	else if (Vars::Visuals::Simulation::ShotPath.Value)
	{
		G::BoxStorage.clear();
		G::PathStorage.clear();
		G::ProjectilePathStorage.clear();

		if (Vars::Colors::ShotPathIgnoreZ.Value.a)
			G::ProjectilePathStorage.emplace_back(tProjInfo.m_vPath, -float(tProjInfo.m_vPath.size()) - TIME_TO_TICKS(F::Backtrack.GetReal()), Vars::Colors::ShotPathIgnoreZ.Value, Vars::Visuals::Simulation::ShotPath.Value);
		if (Vars::Colors::ShotPath.Value.a)
			G::ProjectilePathStorage.emplace_back(tProjInfo.m_vPath, -float(tProjInfo.m_vPath.size()) - TIME_TO_TICKS(F::Backtrack.GetReal()), Vars::Colors::ShotPath.Value, Vars::Visuals::Simulation::ShotPath.Value, true);

		if (Vars::Visuals::Simulation::Box.Value && pNormal)
		{
			const float flSize = std::max(tProjInfo.m_vHull.x, 1.f);
			const Vec3 vSize = { 1.f, flSize, flSize };
			Vec3 vAngles = Math::VectorAngles(*pNormal);

			if (Vars::Colors::ShotPathIgnoreZ.Value.a)
				G::BoxStorage.emplace_back(trace.endpos, vSize * -1, vSize, vAngles, I::GlobalVars->curtime + TICKS_TO_TIME(tProjInfo.m_vPath.size()) + F::Backtrack.GetReal(), Vars::Colors::ShotPathIgnoreZ.Value, Color_t(0, 0, 0, 0));
			if (Vars::Colors::ShotPath.Value.a)
				G::BoxStorage.emplace_back(trace.endpos, vSize * -1, vSize, vAngles, I::GlobalVars->curtime + TICKS_TO_TIME(tProjInfo.m_vPath.size()) + F::Backtrack.GetReal(), Vars::Colors::ShotPath.Value, Color_t(0, 0, 0, 0), true);
		}

		if (!vPoints.empty())
		{
			if (Vars::Colors::SplashRadiusIgnoreZ.Value.a)
				G::ProjectilePathStorage.emplace_back(vPoints, I::GlobalVars->curtime + TICKS_TO_TIME(tProjInfo.m_vPath.size()) + F::Backtrack.GetReal(), Vars::Colors::SplashRadiusIgnoreZ.Value, Vars::Visuals::Simulation::StyleEnum::Line);
			if (Vars::Colors::SplashRadius.Value.a)
				G::ProjectilePathStorage.emplace_back(vPoints, I::GlobalVars->curtime + TICKS_TO_TIME(tProjInfo.m_vPath.size()) + F::Backtrack.GetReal(), Vars::Colors::SplashRadius.Value, Vars::Visuals::Simulation::StyleEnum::Line, true);
		}
	}
}

void CVisuals::SplashRadius(CTFPlayer* pLocal)
{
	if (!Vars::Visuals::Simulation::SplashRadius.Value)
		return;

	for (auto pEntity : H::Entities.GetGroup(EntityEnum::WorldProjectile))
	{
		bool bShouldDraw = false;
		CTFPlayer* pOwner = nullptr;
		CTFWeaponBase* pWeapon = nullptr;

		switch (pEntity->GetClassID())
		{
		case ETFClassID::CTFWeaponBaseGrenadeProj:
		case ETFClassID::CTFWeaponBaseMerasmusGrenade:
		case ETFClassID::CTFGrenadePipebombProjectile:
			bShouldDraw = Vars::Visuals::Simulation::SplashRadius.Value & (pEntity->As<CTFGrenadePipebombProjectile>()->HasStickyEffects() ? Vars::Visuals::Simulation::SplashRadiusEnum::Stickies : Vars::Visuals::Simulation::SplashRadiusEnum::Pipes);
			break;
		case ETFClassID::CTFProjectile_Rocket:
		case ETFClassID::CTFProjectile_SentryRocket:
		case ETFClassID::CTFProjectile_EnergyBall:
			bShouldDraw = Vars::Visuals::Simulation::SplashRadius.Value & Vars::Visuals::Simulation::SplashRadiusEnum::Rockets;
			break;
		case ETFClassID::CTFProjectile_Flare:
			if (Vars::Visuals::Simulation::SplashRadius.Value & Vars::Visuals::Simulation::SplashRadiusEnum::ScorchShot)
			{
				pWeapon = pEntity->As<CTFProjectile_Flare>()->m_hLauncher()->As<CTFWeaponBase>();
				bShouldDraw = pWeapon && pWeapon->As<CTFFlareGun>()->GetFlareGunType() == FLAREGUN_SCORCHSHOT;
			}
		}
		if (!bShouldDraw)
			continue;

		switch (pEntity->GetClassID())
		{
		case ETFClassID::CTFWeaponBaseGrenadeProj:
		case ETFClassID::CTFWeaponBaseMerasmusGrenade:
		case ETFClassID::CTFGrenadePipebombProjectile:
			pOwner = pEntity->As<CTFGrenadePipebombProjectile>()->m_hThrower()->As<CTFPlayer>();
			break;
		case ETFClassID::CTFProjectile_Rocket:
		case ETFClassID::CTFProjectile_SentryRocket:
		case ETFClassID::CTFProjectile_EnergyBall:
		case ETFClassID::CTFProjectile_Flare:
			pOwner = pEntity->m_hOwnerEntity()->As<CTFPlayer>();
		}
		if (!pOwner || !pOwner->IsPlayer())
			continue;
		else if (pOwner->entindex() != I::EngineClient->GetLocalPlayer())
		{
			if (!(Vars::Visuals::Simulation::SplashRadius.Value & Vars::Visuals::Simulation::SplashRadiusEnum::Priority && F::PlayerUtils.IsPrioritized(pOwner->entindex()))
				&& !(Vars::Visuals::Simulation::SplashRadius.Value & Vars::Visuals::Simulation::SplashRadiusEnum::Friends && H::Entities.IsFriend(pOwner->entindex()))
				&& !(Vars::Visuals::Simulation::SplashRadius.Value & Vars::Visuals::Simulation::SplashRadiusEnum::Party && H::Entities.InParty(pOwner->entindex()))
				&& !(pOwner->m_iTeamNum() != pLocal->m_iTeamNum() ? Vars::Visuals::Simulation::SplashRadius.Value & Vars::Visuals::Simulation::SplashRadiusEnum::Enemy : Vars::Visuals::Simulation::SplashRadius.Value & Vars::Visuals::Simulation::SplashRadiusEnum::Team))
				continue;
		}
		else if (!(Vars::Visuals::Simulation::SplashRadius.Value & Vars::Visuals::Simulation::SplashRadiusEnum::Local))
			continue;

		float flRadius = F::AimbotProjectile.GetSplashRadius(pEntity, pWeapon, pOwner);
		H::Draw.RenderWireframeSphere(pEntity->GetAbsOrigin(), flRadius, 36, 36, Vars::Colors::SplashRadius.Value, false);
		H::Draw.RenderWireframeSphere(pEntity->GetAbsOrigin(), flRadius, 36, 36, Vars::Colors::SplashRadiusIgnoreZ.Value, true);
	}
}

void CVisuals::DrawAntiAim(CTFPlayer* pLocal)
{
	if (!pLocal->IsAlive() || pLocal->IsAGhost() || !I::Input->CAM_IsThirdPerson())
		return;

	if (F::AntiAim.AntiAimOn() && Vars::Debug::AntiAimLines.Value)
	{
		const auto& vOrigin = pLocal->GetAbsOrigin();

		Vec3 vScreen1, vScreen2;
		if (SDK::W2S(vOrigin, vScreen1))
		{
			if (SDK::W2S(vOrigin + Math::RotatePoint({ 50, 0, 0 }, {}, { 0, F::AntiAim.vRealAngles.y, 0 }), vScreen2))
				H::Draw.Line(vScreen1.x, vScreen1.y, vScreen2.x, vScreen2.y, { 0, 255, 0, 255 });
			if (SDK::W2S(vOrigin + Math::RotatePoint({ 50, 0, 0 }, {}, { 0, F::AntiAim.vFakeAngles.y, 0 }), vScreen2))
				H::Draw.Line(vScreen1.x, vScreen1.y, vScreen2.x, vScreen2.y, { 255, 0, 0, 255 });
		}

		for (auto& vPair : F::AntiAim.vEdgeTrace)
		{
			if (SDK::W2S(vPair.first, vScreen1) && SDK::W2S(vPair.second, vScreen2))
				H::Draw.Line(vScreen1.x, vScreen1.y, vScreen2.x, vScreen2.y, { 255, 255, 255, 255 });
		}
	}
}

void CVisuals::DrawPickupTimers()
{
	if (!F::Groups.GroupsActive())
		return;

	for (auto it = m_vPickups.begin(); it != m_vPickups.end();)
	{
		auto& tPickup = *it;

		Group_t* pGroup = nullptr;
		float flTime = tPickup.m_flTime - I::GlobalVars->curtime;
		if (!F::Groups.GetGroup(tPickup.m_iType, pGroup) || !pGroup->m_bPickupTimer || flTime < 0.f)
		{
			it = m_vPickups.erase(it);
			continue;
		}

		Vec3 vScreen;
		if(SDK::W2S(tPickup.m_vLocation, vScreen))
			H::Draw.StringOutlined(H::Fonts.GetFont(FONT_ESP), vScreen.x, vScreen.y, pGroup->m_tColor, Vars::Menu::Theme::Background.Value, ALIGN_CENTER, std::format("{:.1f}s", flTime).c_str());

		it++;
	}
}

#define PAIR(x) { x, #x }
void CVisuals::DrawDebugInfo(CTFPlayer* pLocal)
{
	if (Vars::Debug::Info.Value)
	{
		auto pWeapon = H::Entities.GetWeapon();
		auto pCmd = !I::EngineClient->IsPlayingDemo() ? G::LastUserCmd : I::Input->GetUserCmd(I::ClientState->lastoutgoingcommand);

		int x = 10, y = 10;
		const auto& fFont = H::Fonts.GetFont(FONT_INDICATORS);
		const int nTall = fFont.m_nTall + H::Draw.Scale(1);
		y -= nTall;

		if (pCmd)
		{
			H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("View: ({:.3f}, {:.3f}, {:.3f})", pCmd->viewangles.x, pCmd->viewangles.y, pCmd->viewangles.z).c_str());
			H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Move: ({}, {}, {})", pCmd->forwardmove, pCmd->sidemove, pCmd->upmove).c_str());
			H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Buttons: {:#034b} ({})", pCmd->buttons,
				[&]()
				{
					std::string sReturn = "";
					if (pCmd->buttons)
					{
						static std::vector<std::pair<int, const char*>> vFlags = {
							PAIR(IN_ATTACK),
							PAIR(IN_ATTACK2),
							PAIR(IN_ATTACK3),
							PAIR(IN_FORWARD),
							PAIR(IN_BACK),
							PAIR(IN_MOVELEFT),
							PAIR(IN_MOVERIGHT),
							PAIR(IN_JUMP),
							PAIR(IN_DUCK),
							PAIR(IN_RELOAD),
							PAIR(IN_LEFT),
							PAIR(IN_RIGHT),
							PAIR(IN_SCORE),
							/*
							PAIR(IN_USE),
							PAIR(IN_CANCEL),
							PAIR(IN_RUN),
							PAIR(IN_ALT1),
							PAIR(IN_ALT2),
							PAIR(IN_SPEED),
							PAIR(IN_WALK),
							PAIR(IN_ZOOM),
							PAIR(IN_WEAPON1),
							PAIR(IN_WEAPON2),
							PAIR(IN_BULLRUSH),
							PAIR(IN_GRENADE1),
							PAIR(IN_GRENADE2),
							*/
						};

						for (int i = 0; i < vFlags.size(); i++)
						{
							auto& paFlag = vFlags[i];
							if (pCmd->buttons & paFlag.first)
							{
								if (!sReturn.empty())
									sReturn += " | ";
								sReturn += paFlag.second;
							}
						}
					}
					return sReturn.empty() ? "NONE" : sReturn;
				}()
					).c_str());
			H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Tickcount: {}, Command: {}", pCmd->tick_count, pCmd->command_number).c_str());
		}
		Vec3 vOrigin = pLocal->m_vecOrigin();
		H::Draw.StringOutlined(fFont, x, y += nTall * 2, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Origin: ({:.3f}, {:.3f}, {:.3f})", vOrigin.x, vOrigin.y, vOrigin.z).c_str());
		Vec3 vVelocity = pLocal->m_vecVelocity();
		H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Velocity: {:.3f} ({:.3f}, {:.3f}, {:.3f})", vVelocity.Length(), vVelocity.x, vVelocity.y, vVelocity.z).c_str());
		H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Tickbase: {}", pLocal->m_nTickBase()).c_str());
		//H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Choke: {}, {}", G::Choking, I::ClientState->chokedcommands).c_str());
		//H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Ticks: {}, {}", F::Ticks.m_iShiftedTicks, F::Ticks.m_iShiftedGoal).c_str());
		//H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Round state: {}, {}, {}", SDK::GetRoundState(), SDK::GetWinningTeam(), I::EngineClient->IsPlayingDemo()).c_str());
		//H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Entities: {} ({}, {})", I::ClientEntityList->GetMaxEntities(), I::ClientEntityList->GetHighestEntityIndex(), I::ClientEntityList->NumberOfEntities(false)).c_str());

		/*
		if (pWeapon)
		{
			float flTime = TICKS_TO_TIME(pLocal->m_nTickBase());
			float flPrimaryAttack = pWeapon->m_flNextPrimaryAttack();
			float flSecondaryAttack = pWeapon->m_flNextSecondaryAttack();
			float flAttack = pLocal->m_flNextAttack();

			H::Draw.StringOutlined(fFont, x, y += nTall * 2, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Weapon: {}, {}", pWeapon->GetSlot(), pWeapon->GetWeaponID()).c_str());
			H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Attacking: {}", G::Attacking).c_str());
			H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("CanPrimaryAttack: {} ([{:.3f} | {:.3f}] <= {:.3f})", G::CanPrimaryAttack, flPrimaryAttack, flAttack, flTime).c_str());
			H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("CanSecondaryAttack: {} ([{:.3f} | {:.3f}] <= {:.3f})", G::CanSecondaryAttack, flSecondaryAttack, flAttack, flTime).c_str());
			H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Attack: {:.3f}, {:.3f}; {:.3f}", flTime - flPrimaryAttack, flTime - flSecondaryAttack, flTime - flAttack).c_str());
			H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Reload: {} ({} || {} != 0)", G::Reloading, pWeapon->m_bInReload(), pWeapon->m_iReloadMode()).c_str());
			H::Draw.StringOutlined(fFont, x, y += nTall, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOPLEFT, std::format("Throw: {}, Smack: {}", G::Throwing, pWeapon->m_flSmackTime()).c_str());
		}
		*/
	}
}
#undef PAIR

std::vector<DrawBox_t> CVisuals::GetHitboxes(matrix3x4* aBones, CBaseAnimating* pEntity, std::vector<int> vHitboxes, int iTarget)
{
	if (!Vars::Colors::BoneHitboxEdge.Value.a && !Vars::Colors::BoneHitboxFace.Value.a && !Vars::Colors::BoneHitboxEdgeIgnoreZ.Value.a && !Vars::Colors::BoneHitboxFaceIgnoreZ.Value.a
		&& !Vars::Colors::TargetHitboxEdge.Value.a && !Vars::Colors::TargetHitboxFace.Value.a && !Vars::Colors::TargetHitboxEdgeIgnoreZ.Value.a && !Vars::Colors::TargetHitboxFaceIgnoreZ.Value.a)
		return {};

	std::vector<DrawBox_t> vBoxes = {};

	auto pModel = pEntity->GetModel();
	if (!pModel) return vBoxes;
	auto pHDR = I::ModelInfoClient->GetStudiomodel(pModel);
	if (!pHDR) return vBoxes;
	auto pSet = pHDR->pHitboxSet(pEntity->m_nHitboxSet());
	if (!pSet) return vBoxes;

	if (vHitboxes.empty())
	{
		for (int nHitbox = 0; nHitbox < pSet->numhitboxes; nHitbox++)
			vHitboxes.push_back(nHitbox);
	}

	for (int nHitbox : vHitboxes)
	{
		auto pBox = pSet->pHitbox(nHitbox);
		if (!pBox) continue;

		bool bTargeted = nHitbox == iTarget;
		Vec3 vAngle; Math::MatrixAngles(aBones[pBox->bone], vAngle);
		Vec3 vOrigin; Math::GetMatrixOrigin(aBones[pBox->bone], vOrigin);
		Vec3 vMins = pBox->bbmin * pEntity->m_flModelScale();
		Vec3 vMaxs = pBox->bbmax * pEntity->m_flModelScale();

		Color_t tEdge = bTargeted ? Vars::Colors::TargetHitboxEdge.Value : Vars::Colors::BoneHitboxEdge.Value;
		Color_t tFace = bTargeted ? Vars::Colors::TargetHitboxFace.Value : Vars::Colors::BoneHitboxFace.Value;
		if (tEdge.a || tFace.a)
			vBoxes.emplace_back(vOrigin, vMins, vMaxs, vAngle, I::GlobalVars->curtime + Vars::Visuals::Hitbox::DrawDuration.Value, tEdge, tFace);

		tEdge = bTargeted ? Vars::Colors::TargetHitboxEdgeIgnoreZ.Value : Vars::Colors::BoneHitboxEdgeIgnoreZ.Value;
		tFace = bTargeted ? Vars::Colors::TargetHitboxFaceIgnoreZ.Value : Vars::Colors::BoneHitboxFaceIgnoreZ.Value;
		if (tEdge.a || tFace.a)
			vBoxes.emplace_back(vOrigin, vMins, vMaxs, vAngle, I::GlobalVars->curtime + Vars::Visuals::Hitbox::DrawDuration.Value, tEdge, tFace, true);

		if (Vars::Debug::Info.Value)
		{
			float flBoneScale = Vars::Aimbot::Hitscan::BoneSizeMinimumScale.Value;
			float flBoneSubtract = Vars::Aimbot::Hitscan::BoneSizeSubtract.Value;

			if (F::AimbotGlobal.ShouldMultipoint(pEntity, nHitbox, Vars::Aimbot::Hitscan::MultipointHitboxes.Value))
				flBoneScale = std::max(flBoneScale, Vars::Aimbot::Hitscan::MultipointScale.Value / 100.f);

			Vec3 vCheckMins = (pBox->bbmin + flBoneSubtract / pEntity->m_flModelScale()) * flBoneScale * pEntity->m_flModelScale();
			Vec3 vCheckMaxs = (pBox->bbmax - flBoneSubtract / pEntity->m_flModelScale()) * flBoneScale * pEntity->m_flModelScale();

			Color_t tEdge = bTargeted ? Vars::Colors::TargetHitboxEdge.Value : Vars::Colors::BoneHitboxEdge.Value;
			Color_t tFace = bTargeted ? Vars::Colors::TargetHitboxFace.Value : Vars::Colors::BoneHitboxFace.Value;
			if (tEdge.a || tFace.a)
				vBoxes.emplace_back(vOrigin, vCheckMins, vCheckMaxs, vAngle, I::GlobalVars->curtime + Vars::Visuals::Hitbox::DrawDuration.Value, tEdge, tFace);

			tEdge = bTargeted ? Vars::Colors::TargetHitboxEdgeIgnoreZ.Value : Vars::Colors::BoneHitboxEdgeIgnoreZ.Value;
			tFace = bTargeted ? Vars::Colors::TargetHitboxFaceIgnoreZ.Value : Vars::Colors::BoneHitboxFaceIgnoreZ.Value;
			if (tEdge.a || tFace.a)
				vBoxes.emplace_back(vOrigin, vCheckMins, vCheckMaxs, vAngle, I::GlobalVars->curtime + Vars::Visuals::Hitbox::DrawDuration.Value, tEdge, tFace, true);
		}
	}

	return vBoxes;
}

void CVisuals::DrawEffects()
{
	for (auto& tLine : G::LineStorage)
	{
		if (tLine.m_flTime < I::GlobalVars->curtime)
			continue;

		H::Draw.RenderLine(tLine.m_vOrigin.first, tLine.m_vOrigin.second, tLine.m_tColor, tLine.m_bZBuffer);
	}
	for (auto& tPath : G::PathStorage)
	{
		if (tPath.m_flTime >= 0.f && tPath.m_flTime < I::GlobalVars->curtime)
			continue;
		
		H::Draw.RenderPath(tPath.m_vPath, tPath.m_tColor, tPath.m_bZBuffer, tPath.m_iStyle, tPath.m_flTime, Vars::Visuals::Simulation::SeparatorSpacing.Value, Vars::Visuals::Simulation::SeparatorLength.Value);
	}
	for (auto& tBox : G::BoxStorage)
	{
		if (tBox.m_flTime < I::GlobalVars->curtime)
			continue;

		H::Draw.RenderBox(tBox.m_vOrigin, tBox.m_vMins, tBox.m_vMaxs, tBox.m_vAngles, tBox.m_tColorFace, tBox.m_bZBuffer);
		H::Draw.RenderWireframeBox(tBox.m_vOrigin, tBox.m_vMins, tBox.m_vMaxs, tBox.m_vAngles, tBox.m_tColorEdge, tBox.m_bZBuffer);
	}
	for (auto& tBox : G::SphereStorage)
	{
		if (tBox.m_flTime < I::GlobalVars->curtime)
			continue;

		H::Draw.RenderSphere(tBox.m_vOrigin, tBox.m_flRadius, tBox.m_nTheta, tBox.m_nPhi, tBox.m_tColorFace, tBox.m_bZBuffer);
		H::Draw.RenderWireframeSphere(tBox.m_vOrigin, tBox.m_flRadius, tBox.m_nTheta, tBox.m_nPhi, tBox.m_tColorEdge, tBox.m_bZBuffer);
	}
	for (auto& tBox : G::SweptStorage)
	{
		if (tBox.m_flTime < I::GlobalVars->curtime)
			continue;

		H::Draw.RenderWireframeSweptBox(tBox.m_vOrigin.first, tBox.m_vOrigin.second, tBox.m_vMins, tBox.m_vMaxs, tBox.m_vAngles, tBox.m_tColor, tBox.m_bZBuffer);
	}
	for(auto& tProjectilePath : G::ProjectilePathStorage)
	{
		if (tProjectilePath.m_flTime < 0.f || tProjectilePath.m_flTime < I::GlobalVars->curtime)
			continue;

		H::Draw.RenderPath(tProjectilePath.m_vPath, tProjectilePath.m_tColor, tProjectilePath.m_bZBuffer, tProjectilePath.m_iStyle, tProjectilePath.m_flTime, Vars::Visuals::Simulation::SeparatorSpacing.Value, Vars::Visuals::Simulation::SeparatorLength.Value);
	}
	if (auto& tPath = F::Aimbot.m_tPath; tPath.m_flTime)
	{
		H::Draw.RenderPath(tPath.m_vPath, Vars::Colors::RealPath.Value, true, tPath.m_iStyle, tPath.m_flTime);
		H::Draw.RenderPath(tPath.m_vPath, Vars::Colors::RealPathIgnoreZ.Value, false, tPath.m_iStyle, tPath.m_flTime);
	}
}

void CVisuals::DrawServerHitboxes(CTFPlayer* pLocal)
{
	if (!Vars::Debug::DrawServerHitboxes.Value)
		return;

	if (I::Input->CAM_IsThirdPerson() && pLocal->IsAlive())
	{
		if (auto pPlayer2 = S::UTIL_PlayerByIndex.Call<void*>(pLocal->entindex()))
			S::CBaseAnimating_DrawServerHitboxes.Call<void>(pPlayer2, 0.f, true);
	}

	for (auto& pEntity : H::Entities.GetGroup(EntityEnum::PlayerAll))
	{
		auto pPlayer = pEntity->As<CTFPlayer>();
		if (pPlayer->entindex() == I::EngineClient->GetLocalPlayer() || !pPlayer->IsAlive())
			continue;

		if (auto pPlayer2 = S::UTIL_PlayerByIndex.Call<void*>(pPlayer->entindex()))
			S::CBaseAnimating_DrawServerHitboxes.Call<void>(pPlayer2, 0.f, true);
	}
}



void CVisuals::FOV(CTFPlayer* pLocal, CViewSetup* pView)
{
	static auto fov_desired = U::ConVars.FindVar("fov_desired");
	bool bZoomed = pLocal->InCond(TF_COND_ZOOMED);

	float flRegularFOV = fov_desired->GetFloat();
	float flZoomFOV = TF_WEAPON_ZOOM_FOV;

	float flRegularOverride = Vars::Visuals::UI::FieldOfView.Value;
	float flZoomOverride = Vars::Visuals::UI::ZoomFieldOfView.Value;

	if (flRegularOverride || flZoomOverride)
	{
		pView->fov = !bZoomed ? (flRegularOverride ? flRegularOverride : flRegularFOV) : (flZoomOverride ? flZoomOverride : flZoomFOV);
		if (!I::Prediction->InPrediction() && pLocal->m_flFOVRate() && !pLocal->InCond(TF_COND_HALLOWEEN_KART))
		{
			float flDeltaTime = (TICKS_TO_TIME(pLocal->m_nFinalPredictedTick()) - pLocal->m_flFOVTime() + TICKS_TO_TIME(I::GlobalVars->interpolation_amount)) / pLocal->m_flFOVRate();
			if (flDeltaTime < 1.f)
			{
				float flFrom = flRegularOverride && pLocal->m_iFOVStart() == flRegularFOV ? flRegularOverride
					: flZoomOverride && pLocal->m_iFOVStart() == flZoomFOV ? flZoomOverride
					: pLocal->m_iFOVStart();
				float flTo = pView->fov;
				if (flFrom != flTo)
					pView->fov = Math::SimpleSplineRemapVal(flDeltaTime, 0.f, 1.f, flFrom, flTo);
			}
		}
	}

	pLocal->m_iFOV() = pView->fov;
	pLocal->m_iDefaultFOV() = std::max(flRegularOverride, flRegularFOV);
}

void CVisuals::ThirdPerson(CTFPlayer* pLocal, CViewSetup* pView)
{
	if (!pLocal->IsAlive() || F::Spectate.HasTarget())
		return I::Input->CAM_ToFirstPerson();

	const bool bForce = pLocal->IsTaunting() || pLocal->IsAGhost() || pLocal->InCond(TF_COND_HALLOWEEN_KART) || pLocal->InCond(TF_COND_STUNNED) && pLocal->m_iStunFlags() & (TF_STUN_CONTROLS | TF_STUN_LOSER_STATE);	//if (bForce)
	//	return;

	if (Vars::Visuals::Thirdperson::Enabled.Value || bForce)
		I::Input->CAM_ToThirdPerson();
	else
		I::Input->CAM_ToFirstPerson();
	pLocal->ThirdPersonSwitch();

	static auto cam_ideallag = U::ConVars.FindVar("cam_ideallag");
	cam_ideallag->SetValue(0.f);

	if (I::Input->CAM_IsThirdPerson())
	{	// thirdperson offset
		Vec3 vForward, vRight, vUp; Math::AngleVectors(pView->angles, &vForward, &vRight, &vUp);

		Vec3 vOffset;
		float flScale = Vars::Visuals::Thirdperson::Scale.Value ? pLocal->m_flModelScale() : 1.f;
		vOffset += vRight * Vars::Visuals::Thirdperson::Right.Value * flScale;
		vOffset += vUp * Vars::Visuals::Thirdperson::Up.Value * flScale;
		vOffset -= vForward * Vars::Visuals::Thirdperson::Distance.Value * flScale;

		Vec3 vOrigin = pLocal->GetEyePosition(); //pView->origin
		Vec3 vStart = vOrigin;
		Vec3 vEnd = vOrigin + vOffset;

		if (Vars::Visuals::Thirdperson::Collide.Value)
		{
			float flHull = 9.f * flScale;
			Vec3 vMins = { -flHull, -flHull, -flHull }, vMaxs = { flHull, flHull, flHull };

			CGameTrace trace = {};
			CTraceFilterWorldAndPropsOnly filter = {};
			SDK::TraceHull(vStart, vEnd, vMins, vMaxs, MASK_SOLID, &filter, &trace);
			vEnd = trace.endpos;
		}

		pView->origin = vEnd;
	}
}

void CVisuals::Event(IGameEvent* pEvent, uint32_t uHash)
{
	switch (uHash)
	{
	case FNV1A::Hash32Const("player_hurt"):
	{
		bool bBones = Vars::Visuals::Hitbox::BonesEnabled.Value & Vars::Visuals::Hitbox::BonesEnabledEnum::OnHit;
		bool bBounds = Vars::Visuals::Hitbox::BoundsEnabled.Value & Vars::Visuals::Hitbox::BoundsEnabledEnum::OnHit;
		if (!bBones && !bBounds)
			break;

		if (I::EngineClient->GetPlayerForUserID(pEvent->GetInt("attacker")) != I::EngineClient->GetLocalPlayer())
			break;

		int iVictim = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid"));
		auto pEntity = I::ClientEntityList->GetClientEntity(iVictim)->As<CBaseAnimating>();
		if (!pEntity || iVictim == I::EngineClient->GetLocalPlayer())
			break;

		switch (G::PrimaryWeaponType)
		{
		case EWeaponType::HITSCAN:
		{
			if (!bBones)
				break;

			auto pBones = F::Backtrack.GetBones(pEntity);
			if (!pBones)
				break;

			auto vBoxes = GetHitboxes(pBones, pEntity);
			G::BoxStorage.insert(G::BoxStorage.end(), vBoxes.begin(), vBoxes.end());

			break;
		}
		case EWeaponType::PROJECTILE:
		case EWeaponType::MELEE:
		{
			if (!bBounds)
				break;

			if (Vars::Colors::BoundHitboxEdge.Value.a || Vars::Colors::BoundHitboxFace.Value.a)
				G::BoxStorage.emplace_back(pEntity->m_vecOrigin(), pEntity->m_vecMins(), pEntity->m_vecMaxs(), Vec3(), I::GlobalVars->curtime + Vars::Visuals::Hitbox::DrawDuration.Value, Vars::Colors::BoundHitboxEdge.Value, Vars::Colors::BoundHitboxFace.Value, true);
			if (Vars::Colors::BoundHitboxEdgeIgnoreZ.Value.a || Vars::Colors::BoundHitboxFaceIgnoreZ.Value.a)
				G::BoxStorage.emplace_back(pEntity->m_vecOrigin(), pEntity->m_vecMins(), pEntity->m_vecMaxs(), Vec3(), I::GlobalVars->curtime + Vars::Visuals::Hitbox::DrawDuration.Value, Vars::Colors::BoundHitboxEdgeIgnoreZ.Value, Vars::Colors::BoundHitboxFaceIgnoreZ.Value);
		}
		}

		break;
	}
	case FNV1A::Hash32Const("item_pickup"):
	{
		auto pEntity = I::ClientEntityList->GetClientEntity(I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid")))->As<CTFPlayer>();
		if (!pEntity || !pEntity->IsPlayer())
			return;

		int iType = 0;
		{
			const char* sItemName = pEvent->GetString("item");
			if (std::strstr(sItemName, "medkit"))
				iType = TargetsEnum::Health;
			else if (std::strstr(sItemName, "ammopack"))
				iType = TargetsEnum::Ammo;
		}

		Group_t* pGroup = nullptr;
		if (!F::Groups.GetGroup(iType, pGroup, pEntity) || !pGroup->m_bPickupTimer)
			break;
		
		m_vPickups.emplace_back(iType, I::GlobalVars->curtime + 10.f, pEntity->m_vecOrigin());
		break;
	}
	case FNV1A::Hash32Const("client_disconnect"):
	case FNV1A::Hash32Const("client_beginconnect"):
	case FNV1A::Hash32Const("game_newmap"):
	case FNV1A::Hash32Const("teamplay_round_start"):
		m_vPickups.clear();

		G::LineStorage.clear();
		G::BoxStorage.clear();
		G::PathStorage.clear();
		G::SphereStorage.clear();
		G::SweptStorage.clear();
		G::ProjectilePathStorage.clear();
	}
}

void CVisuals::OverrideWorldTextures()
{
	auto uHash = FNV1A::Hash32(Vars::Visuals::World::WorldTexture.Value.c_str());
	switch (uHash)
	{
	case FNV1A::Hash32Const("Default"):
	case FNV1A::Hash32Const("Flat"):
		return;
	}
	

	KeyValues* kv = new KeyValues("LightmappedGeneric");
	if (!kv)
		return;

	switch (uHash)
	{
	case FNV1A::Hash32Const("Dev"):
		kv->SetString("$basetexture", "dev/dev_measuregeneric01b");
		break;
	case FNV1A::Hash32Const("Camo"):
		kv->SetString("$basetexture", "patterns/paint_strokes");
		break;
	case FNV1A::Hash32Const("Black"):
		kv->SetString("$basetexture", "patterns/combat/black");
		break;
	case FNV1A::Hash32Const("White"):
		kv->SetString("$basetexture", "patterns/combat/white");
		break;
	case FNV1A::Hash32Const("Gray"):
		kv->SetString("$basetexture", "vgui/white_additive");
		kv->SetString("$color2", "[0.12 0.12 0.15]");
		break;
	default:
		kv->SetString("$basetexture", Vars::Visuals::World::WorldTexture.Value.c_str());
	}

	for (auto h = I::MaterialSystem->FirstMaterial(); h != I::MaterialSystem->InvalidMaterial(); h = I::MaterialSystem->NextMaterial(h))
	{
		auto pMaterial = I::MaterialSystem->GetMaterial(h);
		if (!pMaterial || pMaterial->IsErrorMaterial() || !pMaterial->IsPrecached() || pMaterial->IsTranslucent() || pMaterial->IsAlphaTested() || pMaterial->IsSpriteCard())
			continue;

		std::string_view sName = pMaterial->GetName();
		std::string_view sGroup = pMaterial->GetTextureGroupName();
		if (!sGroup.starts_with(TEXTURE_GROUP_WORLD)
			|| sName.find("sky") != std::string_view::npos
			|| sName.find("water") != std::string_view::npos)
			continue;

		pMaterial->SetShaderAndParams(kv);
	}
}

static inline void ApplyModulation(Color_t tColor, bool bSky = false)
{
	for (auto h = I::MaterialSystem->FirstMaterial(); h != I::MaterialSystem->InvalidMaterial(); h = I::MaterialSystem->NextMaterial(h))
	{
		auto pMaterial = I::MaterialSystem->GetMaterial(h);
		if (!pMaterial || pMaterial->IsErrorMaterial() || !pMaterial->IsPrecached())
			continue;

		std::string_view sName = pMaterial->GetName();
		std::string_view sGroup = pMaterial->GetTextureGroupName();
		if (!bSky)
		{
			if (!sGroup.starts_with(TEXTURE_GROUP_WORLD)
				|| sName.find("sky") != std::string_view::npos)
				continue;
		}
		else
		{
			if (!sGroup.starts_with(TEXTURE_GROUP_WORLD)
				|| sName.find("sky") == std::string_view::npos)
				continue;
		}

		pMaterial->ColorModulate(tColor.r / 255.f, tColor.g / 255.f, tColor.b / 255.f);
	}
}

void CVisuals::Modulate()
{
	const bool bScreenshot = Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot();
	const bool bWorldModulation = Vars::Visuals::World::Modulations.Value & Vars::Visuals::World::ModulationsEnum::World && !bScreenshot;
	const bool bSkyModulation = Vars::Visuals::World::Modulations.Value & Vars::Visuals::World::ModulationsEnum::Sky && !bScreenshot;

	bool bSetChanged, bColorChanged, bSkyChanged, bConnection;
	{
		static bool bStaticWorld = false, bStaticSky = false;
		const bool bLastWorld = bStaticWorld, bLastSky = bStaticSky;
		const bool bCurrWorld = bStaticWorld = bWorldModulation, bCurrSky = bStaticSky = bSkyModulation;
		bSetChanged = bCurrWorld != bLastWorld || bCurrSky != bLastSky;
	}
	{
		static Color_t tStaticWorld = {}, tStaticSky = {};
		const Color_t tLastWorld = tStaticWorld, tLastSky = tStaticSky;
		const Color_t tCurrWorld = tStaticWorld = Vars::Colors::WorldModulation.Value, tCurrSky = tStaticSky = Vars::Colors::SkyModulation.Value;
		bColorChanged = tCurrWorld != tLastWorld || tCurrSky != tLastSky;
	}
	{
		static uint32_t uStaticHash = 0;
		const uint32_t uLastHash = uStaticHash;
		const uint32_t uCurrHash = uStaticHash = FNV1A::Hash32(Vars::Visuals::World::SkyboxChanger.Value.c_str());
		bSkyChanged = uCurrHash != uLastHash;
	}
	{
		static bool bStaticConnected = false;
		const bool bLastConnected = bStaticConnected;
		const bool bCurrConnected = bStaticConnected = I::EngineClient->IsConnected() && I::EngineClient->IsInGame();
		bConnection = bCurrConnected == bLastConnected;
	}

	if (bSetChanged || bColorChanged || bSkyChanged || !bConnection)
	{
		bWorldModulation ? ApplyModulation(Vars::Colors::WorldModulation.Value) : ApplyModulation({ 255, 255, 255, 255 });
		bSkyModulation ? ApplyModulation(Vars::Colors::SkyModulation.Value, true) : ApplyModulation({ 255, 255, 255, 255 }, true);
	}
}


void CVisuals::RestoreWorldModulation()
{
	ApplyModulation({ 255, 255, 255, 255 });
	ApplyModulation({ 255, 255, 255, 255 }, true);
}

static inline int GetCat(int nFrame)
{
	if (nFrame < 0 || nFrame > 3)
		return 0;

	static const int arrFrames[4] =
	{
		H::Draw.CreateTextureFromArray(Icons::cat_0, 12, 12),
		H::Draw.CreateTextureFromArray(Icons::cat_1, 12, 12),
		H::Draw.CreateTextureFromArray(Icons::cat_2, 12, 12),
		H::Draw.CreateTextureFromArray(Icons::cat_3, 12, 12)
	};

	return arrFrames[nFrame];
}

static inline int GetCat2(int nFrame)
{
	if (nFrame < 0 || nFrame > 3)
		return 0;

	static const int arrFrames[4] =
	{
		H::Draw.CreateTextureFromArray(Icons::cat2_0, 12, 12),
		H::Draw.CreateTextureFromArray(Icons::cat2_1, 12, 12),
		H::Draw.CreateTextureFromArray(Icons::cat2_2, 12, 12),
		H::Draw.CreateTextureFromArray(Icons::cat2_3, 12, 12)
	};

	return arrFrames[nFrame];
}

static inline int GetCatSleep(int nFrame)
{
	if (nFrame < 0 || nFrame > 3)
		return 0;

	static const int arrFrames[4] =
	{
		H::Draw.CreateTextureFromArray(Icons::cat_sleep0, 16, 8),
		H::Draw.CreateTextureFromArray(Icons::cat_sleep1, 16, 8),
		H::Draw.CreateTextureFromArray(Icons::cat_sleep2, 16, 8),
		H::Draw.CreateTextureFromArray(Icons::cat_sleep3, 16, 8)
	};

	return arrFrames[nFrame];
}

static inline int GetCatRun(int nFrame)
{
	if (nFrame < 0 || nFrame > 7)
		return 0;

	static const int arrFrames[8] =
	{
		H::Draw.CreateTextureFromArray(Icons::cat_run0, 20, 13),
		H::Draw.CreateTextureFromArray(Icons::cat_run1, 20, 13),
		H::Draw.CreateTextureFromArray(Icons::cat_run2, 20, 13),
		H::Draw.CreateTextureFromArray(Icons::cat_run3, 20, 13),
		H::Draw.CreateTextureFromArray(Icons::cat_run4, 20, 13),
		H::Draw.CreateTextureFromArray(Icons::cat_run5, 20, 13),
		H::Draw.CreateTextureFromArray(Icons::cat_run6, 20, 13),
		H::Draw.CreateTextureFromArray(Icons::cat_run7, 20, 13)
	};

	return arrFrames[nFrame];
}

void CVisuals::CatDraw(ImVec2 vWindowSize, ImVec2 vWindowPos)
{
	// draw cats
	{
		static bool bIsRunning = false;
		static float flLastRollTime = I::EngineClient->Time();
		static float flProgress = -30.0f;

		if (int(flProgress) > vWindowSize.x + 30)
		{
			bIsRunning = false;
			flProgress = -30.0f;
		}

		if (!bIsRunning && I::EngineClient->Time() - flLastRollTime > 1.0f)
		{
			flLastRollTime = I::EngineClient->Time();
			bIsRunning = I::UniformRandomStream->RandomInt(0, 50) == 50;
		}

		if (bIsRunning)
		{
			flProgress += 75.0f * I::GlobalVars->frametime;

			static float flLastFrameUpdateTime = I::EngineClient->Time();
			static int nFrame = 0;

			if (I::EngineClient->Time() - flLastFrameUpdateTime > 0.08f)
			{
				flLastFrameUpdateTime = I::EngineClient->Time();
				nFrame++;

				if (nFrame > 7)
					nFrame = 0;
			}

			H::Draw.StartClipping(vWindowPos.x, 0, vWindowSize.x - 140.f, H::Draw.m_nScreenH);

			int iOffset = 0;
			if (nFrame == 1 || nFrame == 2 || nFrame == 3 || nFrame == 5 || nFrame == 6)
				iOffset = 1;

			H::Draw.TextureSimple(
				vWindowPos.x + flProgress,
				vWindowPos.y - (13 + iOffset),
				20,
				13,
				GetCatRun(nFrame),
				ALIGN_TOPLEFT
			);
			H::Draw.EndClipping();
		}
	}
	//cats idle
	{
		//idle left
		{
			static float flLastFrameUpdateTime = I::EngineClient->Time();
			static int nFrame = 0;

			if (I::EngineClient->Time() - flLastFrameUpdateTime > 0.2f)
			{
				flLastFrameUpdateTime = I::EngineClient->Time();
				nFrame++;

				if (nFrame > 3)
					nFrame = 0;
			}

			H::Draw.TextureSimple(vWindowPos.x + 5, vWindowPos.y - 12, 12, 12, GetCat(nFrame), ALIGN_TOPLEFT);
		}

		//idle right
		{
			static float flLastFrameUpdateTime = I::EngineClient->Time();
			static int nFrame = 0;

			if (I::EngineClient->Time() - flLastFrameUpdateTime > 0.25f)
			{
				flLastFrameUpdateTime = I::EngineClient->Time();
				nFrame++;

				if (nFrame > 3)
					nFrame = 0;
			}

			H::Draw.TextureSimple(vWindowPos.x + 45, vWindowPos.y - 12, 12, 12, GetCat2(nFrame), ALIGN_TOPLEFT);
		}

		//sleep
		{
			static float flLastFrameUpdateTime = I::EngineClient->Time();
			static int nFrame = 0;

			if (I::EngineClient->Time() - flLastFrameUpdateTime > 0.3f)
			{
				flLastFrameUpdateTime = I::EngineClient->Time();
				nFrame++;

				if (nFrame > 3)
					nFrame = 0;
			}

			H::Draw.TextureSimple(vWindowPos.x + 25, vWindowPos.y - 8, 12, 8, GetCatSleep(nFrame), ALIGN_TOPLEFT);
		}
	}
}

void CVisuals::CreateMove(CTFPlayer* pLocal, CTFWeaponBase* pWeapon)
{
	if (Vars::Visuals::Simulation::ShotPath.Value && G::Attacking == 1 && !F::Aimbot.m_bRan)
	{
		switch (pWeapon->GetWeaponID())
		{
		case TF_WEAPON_BAT_WOOD:
		case TF_WEAPON_BAT_GIFTWRAP:
			if (!G::Throwing)
				break;
			[[fallthrough]];
		default:
			F::Visuals.ProjectileTrace(pLocal, pWeapon, false);
		}
	}

	if (Vars::Visuals::Effects::SpellFootsteps.Value && (F::Ticks.m_bDoubletap || F::Ticks.m_bWarp))
		pLocal->FireEvent(pLocal->GetAbsOrigin(), QAngle(), 7001, nullptr);
	
	static uint32_t iOldMedigunBeam = 0, iOldMedigunCharge = 0;
	uint32_t iNewMedigunBeam = FNV1A::Hash32(Vars::Visuals::Effects::MedigunBeam.Value.c_str()), iNewMedigunCharge = FNV1A::Hash32(Vars::Visuals::Effects::MedigunCharge.Value.c_str());
	if (iOldMedigunBeam != iNewMedigunBeam || iOldMedigunCharge != iNewMedigunCharge)
	{
		if (pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_MEDIGUN)
		{
			auto pMedigun = pWeapon->As<CWeaponMedigun>();
			pMedigun->UpdateVisibility();
			pMedigun->StopChargeEffect(false);
			pMedigun->ManageChargeEffect();
		}

		iOldMedigunBeam = iNewMedigunBeam;
		iOldMedigunCharge = iNewMedigunCharge;
	}

	static float flStaticRatio = 0.f;
	float flOldRatio = flStaticRatio;
	float flNewRatio = flStaticRatio = Vars::Visuals::UI::AspectRatio.Value;

	static auto r_aspectratio = U::ConVars.FindVar("r_aspectratio");
	if (flNewRatio != flOldRatio)
		r_aspectratio->SetValue(flNewRatio);
}