#include "Ticks.h"

#include "../PacketManip/AntiAim/AntiAim.h"
#include "../EnginePrediction/EnginePrediction.h"
#include "../Aimbot/AutoRocketJump/AutoRocketJump.h"
#include "../Backtrack/Backtrack.h"

void CTicks::Reset()
{
	m_bSpeedhack = m_bDoubletap = m_bRecharge = m_bWarp = false;
	m_iShiftedTicks = m_iShiftedGoal = 0;
}

void CTicks::Recharge(CTFPlayer* pLocal)
{
	if (!m_bGoalReached)
		return;

	bool bPassive = m_bRecharge = false;

	static float flPassiveTime = 0.f;
	flPassiveTime = std::max(flPassiveTime - TICK_INTERVAL, -TICK_INTERVAL);
	if (Vars::Doubletap::PassiveRecharge.Value && 0.f >= flPassiveTime)
	{
		bPassive = true;
		flPassiveTime += 1.f / Vars::Doubletap::PassiveRecharge.Value;
	}

	if (m_iDeficit)
	{
		bPassive = true;
		m_iDeficit--, m_iShiftedTicks--;
	}

	if (!Vars::Doubletap::RechargeTicks.Value && !bPassive
		|| m_bDoubletap || m_bWarp || m_iShiftedTicks == m_iMaxShift || m_bSpeedhack)
		return;

	m_bRecharge = true;
	m_iShiftedGoal = m_iShiftedTicks + 1;
}

void CTicks::Warp()
{
	if (!m_bGoalReached)
		return;

	m_bWarp = false;
	if (!Vars::Doubletap::Warp.Value
		|| !m_iShiftedTicks || m_bDoubletap || m_bRecharge || m_bSpeedhack)
		return;

	m_bWarp = true;
	m_iShiftedGoal = std::max(m_iShiftedTicks - Vars::Doubletap::WarpRate.Value + 1, 0);
}

void CTicks::Doubletap(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (!m_bGoalReached)
		return;

	if (!Vars::Doubletap::Doubletap.Value
		|| m_iWait || m_bWarp || m_bRecharge || m_bSpeedhack)
		return;

	int iTicks = std::min(m_iShiftedTicks + 1, 22);
	auto pWeapon = H::Entities.GetWeapon();
	if (!(iTicks >= Vars::Doubletap::TickLimit.Value || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1))
		return;

	bool bAttacking = G::PrimaryWeaponType == EWeaponType::MELEE ? pCmd->buttons & IN_ATTACK : G::Attacking;
	if (!G::CanPrimaryAttack && !G::Reloading || !bAttacking && !m_bDoubletap || F::AutoRocketJump.IsRunning())
		return;

	m_bDoubletap = true;
	m_iShiftedGoal = std::max(m_iShiftedTicks - Vars::Doubletap::TickLimit.Value + 1, 0);
	if (Vars::Doubletap::AntiWarp.Value)
		m_bAntiWarp = pLocal->m_hGroundEntity();
}

void CTicks::Speedhack()
{
	m_bSpeedhack = Vars::Speedhack::Enabled.Value;
	if (!m_bSpeedhack)
		return;

	m_bDoubletap = m_bWarp = m_bRecharge = false;
}

static Vec3 s_vVelocity = {};
static int s_iMaxTicks = 0;
void CTicks::AntiWarp(CTFPlayer* pLocal, float flYaw, float& flForwardMove, float& flSideMove, int iTicks)
{
	s_iMaxTicks = std::max(iTicks + 1, s_iMaxTicks);

	Vec3 vAngles; Math::VectorAngles(s_vVelocity, vAngles);
	vAngles.y = flYaw - vAngles.y;
	Vec3 vForward; Math::AngleVectors(vAngles, &vForward);
	vForward *= s_vVelocity.Length2D();

	if (iTicks > std::max(s_iMaxTicks - 8, 3))
		flForwardMove = -vForward.x, flSideMove = -vForward.y;
	else if (iTicks > 3)
		flForwardMove = flSideMove = 0.f;
	else
		flForwardMove = vForward.x, flSideMove = vForward.y;
}
void CTicks::AntiWarp(CTFPlayer* pLocal, float flYaw, float& flForwardMove, float& flSideMove)
{
	AntiWarp(pLocal, flYaw, flForwardMove, flSideMove, GetTicks());
}
void CTicks::AntiWarp(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (m_bAntiWarp)
		AntiWarp(pLocal, pCmd->viewangles.y, pCmd->forwardmove, pCmd->sidemove);
	else
	{
		s_vVelocity = pLocal->m_vecVelocity();
		s_iMaxTicks = 0;
	}
}

bool CTicks::ValidWeapon(CTFWeaponBase* pWeapon)
{
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_PDA:
	case TF_WEAPON_PDA_ENGINEER_BUILD:
	case TF_WEAPON_PDA_ENGINEER_DESTROY:
	case TF_WEAPON_PDA_SPY:
	case TF_WEAPON_PDA_SPY_BUILD:
	case TF_WEAPON_BUILDER:
	case TF_WEAPON_INVIS:
	case TF_WEAPON_GRAPPLINGHOOK:
	case TF_WEAPON_JAR_MILK:
	case TF_WEAPON_LUNCHBOX:
	case TF_WEAPON_BUFF_ITEM:
	case TF_WEAPON_ROCKETPACK:
	case TF_WEAPON_JAR_GAS:
	case TF_WEAPON_LASER_POINTER:
	case TF_WEAPON_MEDIGUN:
	case TF_WEAPON_SNIPERRIFLE:
	case TF_WEAPON_SNIPERRIFLE_DECAP:
	case TF_WEAPON_SNIPERRIFLE_CLASSIC:
	case TF_WEAPON_COMPOUND_BOW:
	case TF_WEAPON_JAR:
		return false;
	}

	return true;
}

MAKE_SIGNATURE(Host_ShouldRun, "engine.dll", "48 83 EC ? 48 8B 05 ? ? ? ? 83 78 ? ? 74 ? 48 8B 05", 0x0);
MAKE_SIGNATURE(net_time, "engine.dll", "F2 0F 10 05 ? ? ? ? 66 0F 2F 05 ? ? ? ? 72", 0x0);
MAKE_SIGNATURE(host_frametime_unbounded, "engine.dll", "F3 0F 10 05 ? ? ? ? F3 0F 11 45 ? F3 0F 11 4D ? 89 45", 0x0);
MAKE_SIGNATURE(host_frametime_stddeviation, "engine.dll", "F3 0F 10 0D ? ? ? ? 48 8D 54 24 ? 0F 57 C0 48 89 44 24 ? 8B 05", 0x0);
MAKE_SIGNATURE(Con_NXPrintf, "engine.dll", "48 89 54 24 ? 4C 89 44 24 ? 4C 89 4C 24 ? 53 57 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B D9", 0x0);

void CTicks::SendMoveFunc()
{
	byte data[4000];

	int nextcommandnr = I::ClientState->lastoutgoingcommand + I::ClientState->chokedcommands + 1;

	CLC_Move moveMsg;
	moveMsg.m_DataOut.StartWriting(data, sizeof(data));

	int nCommands = 1 + I::ClientState->chokedcommands;
	moveMsg.m_nNewCommands = std::clamp(nCommands, 0, MAX_NEW_COMMANDS);
	int nExtraCommands = nCommands - moveMsg.m_nNewCommands;
	moveMsg.m_nBackupCommands = std::clamp(nExtraCommands, 2, MAX_BACKUP_COMMANDS);

	int numcmds = moveMsg.m_nNewCommands + moveMsg.m_nBackupCommands;

	if (!m_bSpeedhack)
	{
		int iAllowedNewCommands = std::max(m_iMaxUsrCmdProcessTicks - m_iShiftedTicks, 0);
		int iCmdCount = moveMsg.m_nNewCommands + moveMsg.m_nBackupCommands - 3;
		if (iCmdCount > iAllowedNewCommands)
		{
			SDK::Output("clc_Move", std::format("{:d} sent <{:d} | {:d}>, max was {:d}.", iCmdCount + 3, moveMsg.m_nNewCommands, moveMsg.m_nBackupCommands, iAllowedNewCommands).c_str(), { 255, 0, 0, 255 });
			m_iDeficit = iCmdCount - iAllowedNewCommands;
		}
	}

	int from = -1;
	bool bOK = true;
	for (int to = nextcommandnr - numcmds + 1; to <= nextcommandnr; to++)
	{
		bool isnewcmd = to >= (nextcommandnr - moveMsg.m_nNewCommands + 1);

		bOK = bOK && I::Client->WriteUsercmdDeltaToBuffer(&moveMsg.m_DataOut, from, to, isnewcmd);
		from = to;
	}

	if (bOK)
	{
		if (nExtraCommands)
			I::ClientState->m_NetChannel->m_nChokedPackets -= nExtraCommands;

		I::ClientState->m_NetChannel->SendNetMsg(moveMsg);
	}
}

void CTicks::MoveFunc(float accumulated_extra_samples, bool bFinalTick)
{
	m_iShiftedTicks--;
	if (m_iWait > 0)
		m_iWait--;

	int iTicks = std::min(m_iShiftedTicks + 1, 22);
	auto pWeapon = H::Entities.GetWeapon();
	if (!(iTicks >= Vars::Doubletap::TickLimit.Value || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1))
		m_iWait = -1;

	m_bGoalReached = bFinalTick && m_iShiftedTicks == m_iShiftedGoal;

	if (!I::ClientState->IsConnected() || !S::Host_ShouldRun.Call<bool>())
		return;

	G::SendPacket = true;

	if (I::DemoPlayer->IsPlayingBack())
	{
		if (!I::ClientState->ishltv && !I::ClientState->isreplay)
			return;

		G::SendPacket = false;
	}

	static auto host_limitlocal = U::ConVars.FindVar("host_limitlocal");
	double net_time = *reinterpret_cast<double*>(U::Memory.RelToAbs(S::net_time(), 4));
	if ((!I::ClientState->m_NetChannel->IsLoopback() || host_limitlocal->GetInt()) &&
		(net_time < I::ClientState->m_flNextCmdTime || !I::ClientState->m_NetChannel->CanPacket() || !bFinalTick))
		G::SendPacket = false;

	if (I::ClientState->IsActive())
	{
		int nextcommandnr = I::ClientState->lastoutgoingcommand + I::ClientState->chokedcommands + 1;

		static auto engine_no_focus_sleep = U::ConVars.FindVar("engine_no_focus_sleep");
		if (engine_no_focus_sleep->GetInt())
			engine_no_focus_sleep->SetValue(0);

		I::Client->CreateMove(nextcommandnr, TICK_INTERVAL - accumulated_extra_samples, !I::ClientState->IsPaused());

		if(I::DemoRecorder->IsRecording())
			I::DemoRecorder->RecordUserInput(nextcommandnr);

		if(G::SendPacket)
			SendMoveFunc();
		else
		{
			I::ClientState->m_NetChannel->SetChoked();
			I::ClientState->chokedcommands++;
		}
	}

	if (!G::SendPacket)
		return;

	bool hasProblem = I::ClientState->m_NetChannel->IsTimingOut() && !I::DemoPlayer->IsPlayingBack();
	if (hasProblem)
	{
		struct con_nprint_s
		{
			int		index;
			float	time_to_live;
			float	color[3];
			bool	fixed_width_font;
		} np;

		np.time_to_live = 1.f;
		np.index = 2;
		np.fixed_width_font = false;
		np.color[0] = 1.f;
		np.color[1] = 0.2f;
		np.color[2] = 0.2f;

		float flTimeOut = I::ClientState->m_NetChannel->GetTimeoutSeconds();
		float flRemainingTime = flTimeOut - I::ClientState->m_NetChannel->GetTimeSinceLastReceived();
		S::Con_NXPrintf.Call<void>(&np, "WARNING:  Connection Problem"); 
		np.index = 3;
		S::Con_NXPrintf.Call<void>(&np, "Auto-disconnect in %.1f seconds", flRemainingTime);

		I::ClientState->ForceFullUpdate();
	}

	if (I::ClientState->IsActive())
	{
		float host_frametime_unbounded = *reinterpret_cast<float*>(U::Memory.RelToAbs(S::host_frametime_unbounded(), 4));
		float host_frametime_stddeviation = *reinterpret_cast<float*>(U::Memory.RelToAbs(S::host_frametime_stddeviation(), 4));
		NET_Tick mymsg(I::ClientState->m_nDeltaTick, host_frametime_unbounded, host_frametime_stddeviation);
		I::ClientState->m_NetChannel->SendNetMsg(mymsg);
	}

	I::ClientState->lastoutgoingcommand = I::ClientState->m_NetChannel->SendDatagram(NULL);
	I::ClientState->chokedcommands = 0;

	if (I::ClientState->IsActive())
	{
		static auto cl_cmdrate = U::ConVars.FindVar("cl_cmdrate");
		float commandInterval = 1.0f / cl_cmdrate->GetFloat();
		float maxDelta = std::min(TICK_INTERVAL, commandInterval);
		float delta = std::clamp((float)(net_time - I::ClientState->m_flNextCmdTime), 0.0f, maxDelta);
		I::ClientState->m_flNextCmdTime = net_time + commandInterval - delta;
	}
	else
		I::ClientState->m_flNextCmdTime = net_time + 0.2f;
}

void CTicks::Move(float accumulated_extra_samples, bool bFinalTick)
{
	MoveManage();

	if (I::ClientState->IsActive())
	{
		static auto engine_no_focus_sleep = U::ConVars.FindVar("engine_no_focus_sleep");
		if (engine_no_focus_sleep->GetInt())
			engine_no_focus_sleep->SetValue(0);

		const bool bHasProblem = I::ClientState->m_NetChannel->IsTimingOut() && !I::EngineClient->IsPlayingDemo();
		if (bHasProblem)
			I::ClientState->m_nDeltaTick = -1;
	}

	while (m_iShiftedTicks > m_iMaxShift)
		MoveFunc(accumulated_extra_samples, false);
	m_iShiftedTicks = std::max(m_iShiftedTicks, 0) + 1;

	if (m_bSpeedhack)
	{
		m_iShiftedTicks = Vars::Speedhack::Amount.Value;
		m_iShiftedGoal = 0;
	}

	m_iShiftedGoal = std::clamp(m_iShiftedGoal, 0, m_iMaxShift);
	if (m_iShiftedTicks > m_iShiftedGoal) // normal use/doubletap/teleport
	{
		m_iShiftStart = m_iShiftedTicks - 1;
		m_bShifted = false;

		while (m_iShiftedTicks > m_iShiftedGoal)
		{
			m_bShifting = m_bShifted = m_bShifted || m_iShiftedTicks - 1 != m_iShiftedGoal;
			MoveFunc(accumulated_extra_samples, m_iShiftedTicks - 1 == m_iShiftedGoal);
		}

		m_bShifting = m_bAntiWarp = m_bTimingUnsure = false;
		if (m_bWarp)
			m_iDeficit = 0;

		m_bDoubletap = m_bWarp = false;
	}
	else // else recharge, run once if we have any choked ticks
	{
		if (I::ClientState->chokedcommands)
			MoveFunc(accumulated_extra_samples, bFinalTick);
	}
}

void CTicks::MoveManage()
{
	auto pLocal = H::Entities.GetLocal();
	if (!pLocal)
		return;

	Recharge(pLocal);
	Warp();
	Speedhack();
	
	if (!m_bRecharge)
		m_iWait = std::max(m_iWait, 0);
	if (auto pWeapon = H::Entities.GetWeapon())
	{
		switch (pWeapon->GetWeaponID())
		{
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_CANNON:
			if (!G::CanSecondaryAttack)
				m_iWait = Vars::Doubletap::TickLimit.Value;
			break;
		default:
			if (!ValidWeapon(pWeapon))
				m_iWait = -1;
			else if (G::Attacking || !G::CanPrimaryAttack && !G::Reloading)
				m_iWait = Vars::Doubletap::TickLimit.Value;
		}
	}
	else
		m_iWait = -1;

	static auto sv_maxusrcmdprocessticks = U::ConVars.FindVar("sv_maxusrcmdprocessticks");
	m_iMaxUsrCmdProcessTicks = sv_maxusrcmdprocessticks->GetInt();
	if (Vars::Misc::Game::AntiCheatCompatibility.Value)
		m_iMaxUsrCmdProcessTicks = std::min(m_iMaxShift, 8);
	m_iMaxShift = m_iMaxUsrCmdProcessTicks - std::max(m_iMaxUsrCmdProcessTicks - Vars::Doubletap::RechargeLimit.Value, 0) - (F::AntiAim.YawOn() ? F::AntiAim.AntiAimTicks() : 0);
	m_iMaxShift = std::max(m_iMaxShift, 1);
}

void CTicks::CreateMove(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	Doubletap(pLocal, pCmd);
	AntiWarp(pLocal, pCmd);
	ManagePacket(pCmd);

	SaveShootPos(pLocal);
	SaveShootAngle(pCmd);

	if (m_bDoubletap && m_iShiftedTicks == m_iShiftStart && pWeapon && pWeapon->IsInReload())
		m_bTimingUnsure = true;
}

void CTicks::ManagePacket(CUserCmd* pCmd)
{
	if (!m_bDoubletap && !m_bWarp && !m_bSpeedhack)
	{
		static bool bWasSet = false;
		bool bCanChoke = F::Ticks.CanChoke(true); // failsafe
		if (G::PSilentAngles && bCanChoke)
			G::SendPacket = false, bWasSet = true;
		else if (bWasSet || !bCanChoke)
			G::SendPacket = true, bWasSet = false;

		bool bShouldShift = m_iShiftedTicks && m_iShiftedTicks + I::ClientState->chokedcommands >= m_iMaxUsrCmdProcessTicks;
		if (!G::SendPacket && bShouldShift)
			m_iShiftedGoal = std::max(m_iShiftedGoal - 1, 0);
	}
	else
	{
		if ((m_bSpeedhack || m_bWarp) && G::Attacking == 1)
		{
			G::SendPacket = true;
			return;
		}

		G::SendPacket = m_iShiftedGoal == m_iShiftedTicks;
		if (I::ClientState->chokedcommands >= 21) // prevent overchoking
			G::SendPacket = true;
	}
}

void CTicks::Start(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	Vec2 vOriginalMove; int iOriginalButtons;
	if (m_bPredictAntiwarp = m_bAntiWarp || GetTicks(H::Entities.GetWeapon()) && Vars::Doubletap::AntiWarp.Value && pLocal->m_hGroundEntity())
	{
		vOriginalMove = { pCmd->forwardmove, pCmd->sidemove };
		iOriginalButtons = pCmd->buttons;

		AntiWarp(pLocal, pCmd->viewangles.y, pCmd->forwardmove, pCmd->sidemove);
	}

	F::EnginePrediction.Start(pLocal, pCmd);

	if (m_bPredictAntiwarp)
	{
		pCmd->forwardmove = vOriginalMove.x, pCmd->sidemove = vOriginalMove.y;
		pCmd->buttons = iOriginalButtons;
	}
}

void CTicks::End(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	if (m_bPredictAntiwarp && !m_bAntiWarp && !G::Attacking)
	{
		F::EnginePrediction.End(pLocal, pCmd);
		F::EnginePrediction.Start(pLocal, pCmd);
	}
}

bool CTicks::CanChoke(bool bCanShift, int iMaxTicks)
{
	bool bCanChoke = I::ClientState->chokedcommands < 21;
	if (bCanChoke && !bCanShift)
		bCanChoke = m_iShiftedTicks + I::ClientState->chokedcommands < iMaxTicks;
	return bCanChoke;
}
bool CTicks::CanChoke(bool bCanShift)
{
	return CanChoke(bCanShift, m_iMaxUsrCmdProcessTicks);
}

int CTicks::GetTicks(CTFWeaponBase* pWeapon)
{
	if (m_bDoubletap && m_iShiftedGoal < m_iShiftedTicks)
		return m_iShiftedTicks - m_iShiftedGoal;

	if (!Vars::Doubletap::Doubletap.Value
		|| m_iWait || m_bWarp || m_bRecharge || m_bSpeedhack || F::AutoRocketJump.IsRunning())
		return 0;

	int iTicks = std::min(m_iShiftedTicks + 1, 22);
	if (!(iTicks >= Vars::Doubletap::TickLimit.Value || pWeapon && GetShotsWithinPacket(pWeapon, iTicks) > 1))
		return 0;

	return std::min(Vars::Doubletap::TickLimit.Value - 1, m_iMaxShift);
}

int CTicks::GetShotsWithinPacket(CTFWeaponBase* pWeapon, int iTicks)
{
	iTicks = std::min(m_iMaxShift + 1, iTicks);

	int iDelay = 1;
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_MINIGUN:
	case TF_WEAPON_PIPEBOMBLAUNCHER:
	case TF_WEAPON_CANNON:
		iDelay = 2;
	}

	return 1 + (iTicks - iDelay) / std::ceilf(pWeapon->GetFireRate() / TICK_INTERVAL);
}

int CTicks::GetMinimumTicksNeeded(CTFWeaponBase* pWeapon)
{
	int iDelay = 1;
	switch (pWeapon->GetWeaponID())
	{
	case TF_WEAPON_MINIGUN:
	case TF_WEAPON_PIPEBOMBLAUNCHER:
	case TF_WEAPON_CANNON:
		iDelay = 2;
	}

	return (GetShotsWithinPacket(pWeapon) - 1) * std::ceilf(pWeapon->GetFireRate() / TICK_INTERVAL) + iDelay;
}

void CTicks::SaveShootPos(CTFPlayer* pLocal)
{
	if (m_iShiftedTicks == m_iShiftStart)
		m_vShootPos = pLocal->GetShootPos();
}
Vec3 CTicks::GetShootPos()
{
	return m_vShootPos;
}

void CTicks::SaveShootAngle(CUserCmd* pCmd)
{
	static auto sv_maxusrcmdprocessticks_holdaim = U::ConVars.FindVar("sv_maxusrcmdprocessticks_holdaim");

	if (G::SendPacket)
		m_bShootAngle = false;
	else if (!m_bShootAngle && G::Attacking == 1 && sv_maxusrcmdprocessticks_holdaim->GetBool())
		m_vShootAngle = pCmd->viewangles, m_bShootAngle = true;
}
Vec3* CTicks::GetShootAngle()
{
	if (m_bShootAngle && I::ClientState->chokedcommands)
		return &m_vShootAngle;
	return nullptr;
}

bool CTicks::IsTimingUnsure()
{	// actually knowing when we'll shoot would be better than this, but this is fine for now
	return m_bTimingUnsure || m_bSpeedhack /*|| m_bWarp*/;
}

static inline Color_t cathookFade(Color_t colorA, Color_t colorB, float time, float timescale)
{
	float percentage = fabsf(sin(time * timescale));
	Color_t newColor;

	newColor.r = (colorB.r - colorA.r) * percentage + colorA.r;
	newColor.g = (colorB.g - colorA.g) * percentage + colorA.g;
	newColor.b = (colorB.b - colorA.b) * percentage + colorA.b;
	newColor.a = (colorB.a - colorA.a) * percentage + colorA.a;
	return newColor;
}

void CTicks::Draw(CTFPlayer* pLocal)
{
	if (!(Vars::Menu::Indicators.Value & Vars::Menu::IndicatorsEnum::Ticks) || !pLocal->IsAlive())
		return;

	const DragBox_t dtPos = Vars::Menu::TicksDisplay.Value;
	const auto& fFont = H::Fonts.GetFont(FONT_INDICATORS);

	if (!m_bSpeedhack)
	{
		int iAntiAimTicks = F::AntiAim.YawOn() ? F::AntiAim.AntiAimTicks() : 0;

		int iTicks = std::clamp(m_iShiftedTicks + std::max(I::ClientState->chokedcommands - iAntiAimTicks, 0), 0, m_iMaxUsrCmdProcessTicks);
		int iMax = std::max(m_iMaxUsrCmdProcessTicks - iAntiAimTicks, 0);

		float flRatio = float(iTicks) / float(iMax);

		int iSizeX = H::Draw.Scale(85, Scale_Round), iSizeY = H::Draw.Scale(8, Scale_Round);
		int iPosX = dtPos.x - iSizeX / 2, iPosY = dtPos.y + fFont.m_nTall + H::Draw.Scale(4) + 1;

		Color_t tBarNitroRightGradient, tBarAterisLeftGradient = Vars::Menu::Theme::Accent.Value;
		Color_t tBarRijinVTwoLeftGradient = Vars::Colors::PrimaryBarColor.Value;
		Color_t tBarRijinVTwoRightGradient = Vars::Colors::SecondaryBarColor.Value;
		Color_t tBarNitroLeftGradient, BarRijinVTwoBackground, BarSeOwnedDEFilled = Vars::Menu::Theme::Background.Value;
		Color_t tBarRijinVOneFilled = { 78, 125, 32, 255 };
		Color_t tBarLBoxFilled = { 0, 255, 0, 255 };
		Color_t tBarCathookPartial = { 255, 120, 0, 100 };
		Color_t tBarCathookFilled = { 0, 255, 0, 100 };
		Color_t tBarAterisRightGradient = { 20, 20, 20, 50 };
		Color_t tBarDeadFlagLeftGradient = { 165, 167, 209, 255 };
		Color_t tBarDeadFlagRightGradient = { 238, 217, 223, 255 };


		if (Vars::Colors::PrimaryBarColor.Value.a != 0) {
			tBarNitroRightGradient = tBarRijinVTwoLeftGradient = tBarLBoxFilled = tBarRijinVOneFilled = tBarCathookFilled = BarSeOwnedDEFilled = tBarAterisLeftGradient = tBarDeadFlagLeftGradient = Vars::Colors::PrimaryBarColor.Value;
		}
		if (Vars::Colors::SecondaryBarColor.Value.a != 0) {
			tBarNitroLeftGradient = tBarRijinVTwoRightGradient = tBarCathookPartial = tBarDeadFlagRightGradient = tBarAterisRightGradient = Vars::Colors::SecondaryBarColor.Value;
		}

		switch (Vars::Menu::TickDisplayStyle.Value)
		{
		case Vars::Menu::TickDisplayStyleEnum::Nitro:
		{
			H::Draw.StringOutlined(fFont, dtPos.x, dtPos.y, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOP, std::format("Ticks {} / {}", iTicks, iMax).c_str());
			H::Draw.FillRect(iPosX, iPosY, iSizeX, iSizeY, Vars::Menu::Theme::Background.Value);
			if (flRatio)
			{
				H::Draw.StartClipping(iPosX + 1, iPosY + 1, iSizeX * flRatio - 2, iSizeY - 2);
				H::Draw.GradientRect(iPosX + 1, iPosY + 1, iSizeX - 2, iSizeY - 2, tBarNitroLeftGradient, tBarNitroRightGradient, true);
				H::Draw.EndClipping();
			}
			break;
		}
		case Vars::Menu::TickDisplayStyleEnum::Cathook:
		{
			const auto& fCathookFont = H::Fonts.GetFont(FONT_ESP);
			int iSize = Vars::Menu::CathookBarSize.Value;
			int barBGSizeX = iSize * 2.0f;
			int barBGSizeY = iSize / 5.0f;

			int iPosX = dtPos.x - iSizeX / 2, iPosY = dtPos.y + 5;
			H::Draw.FillRect(iPosX - 5.0f, iPosY - 5.0f, barBGSizeX + 10.0f, barBGSizeY + 10.0f, { 96, 96, 96, 150 });

			Color_t color = { 255, 120, 0, 255 };
			if (iTicks == 0)
				color = { 128, 128, 128, 255 };
			else if (iTicks == iMax)
				color = { 0, 255, 0, 255 };
			std::string text = std::format("Shiftable ticks: {}", iTicks);
			H::Draw.StringOutlined(fCathookFont, 10, Vars::Menu::CathookTextYPos.Value - H::Draw.GetTextSize(text.c_str(), fCathookFont).y - H::Draw.Scale(1, Scale_Round), color, { 0, 0, 0, 255 }, ALIGN_LEFT, text.c_str());

			if (flRatio)
			{
				Color_t barCathook = tBarCathookPartial;
				if (iTicks == iMax)
					barCathook = tBarCathookFilled;
				H::Draw.StartClipping(iPosX, iPosY, iSize * 2.0f * flRatio, iSize / 5.0f);
				H::Draw.FillRect(iPosX, iPosY, iSize * 2.0f * flRatio, iSize / 5.0f, barCathook);
				H::Draw.EndClipping();
			}
			break;
		}
		case Vars::Menu::TickDisplayStyleEnum::Text:
			H::Draw.StringOutlined(fFont, iPosX, iPosY, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_CENTER, std::format("Ticks {} / {}", iTicks, m_iMaxShift).c_str());
			break;
		case Vars::Menu::TickDisplayStyleEnum::SEOwnedDE:
		{
			static int nBarW = 80;
			static int nBarH = 4;
			int iPosX = dtPos.x - nBarW / 2, iPosY = dtPos.y + nBarH;

			H::Draw.FillRect(iPosX - 1, iPosY - 1, nBarW + 2, nBarH + 2, Vars::Menu::Theme::Background.Value);
			if (iTicks > 0)
			{
				const Color_t tBarColor = BarSeOwnedDEFilled;
				const Color_t tBarColorDim = { tBarColor.r, tBarColor.g, tBarColor.b, 25 };

				H::Draw.GradientRect(iPosX, iPosY, nBarW * flRatio, nBarH, tBarColorDim, tBarColor, false);
				H::Draw.FillRectOutline(iPosX, iPosY, nBarW * flRatio, nBarH, tBarColor);
			}

			if (m_iWait && ValidWeapon(H::Entities.GetWeapon()))
			{
				H::Draw.FillRect(iPosX - 1, iPosY + 7, 82, 6, Vars::Menu::Theme::Background.Value);
				constexpr Color_t tBarColor = { 241, 196, 15, 255 };
				constexpr Color_t tBarColorDim = { tBarColor.r, tBarColor.g, tBarColor.b, 25 };

				const float flWaitRatio = float(m_iWait) / float(iMax);

				H::Draw.GradientRect(iPosX, iPosY + nBarH + 4, nBarW * flWaitRatio, nBarH, tBarColorDim, tBarColor, false);
				H::Draw.FillRectOutline(iPosX, iPosY + nBarH + 4, nBarW * flWaitRatio, nBarH, tBarColor);
			}
			break;
		}
		case Vars::Menu::TickDisplayStyleEnum::RijiNv1:
		{
			Color_t BarRijin = { 23, 23, 23, 215 };

			if (flRatio)
				BarRijin = { 0, 0, 0, 255 };

			H::Draw.FillRect(iPosX - 1, iPosY + 7, 112, 7, BarRijin);

			if (flRatio)
			{				
				H::Draw.FillRectOutline(iPosX, iPosY + 8, 110.f * flRatio, 5, tBarRijinVOneFilled);
			}
			break;
		}
		case Vars::Menu::TickDisplayStyleEnum::RijiNv2:
		{
			const auto& fIndicatorFont = H::Fonts.GetFont(FONT_RIJIN_DT);

			constexpr int iBarW = 150;
			constexpr int iBarH = 15;
			iPosX = dtPos.x - iBarW / 2, iPosY = dtPos.y + fIndicatorFont.m_nTall + 6;

			H::Draw.FillRect(iPosX, iPosY, iBarW, iBarH, Vars::Menu::Theme::Background.Value);

			const auto pWeapon = H::Entities.GetWeapon();
			const bool bValidWeapon = pWeapon && ValidWeapon(pWeapon);

			float flTargetRatio = 0.f;
			if (!m_bSpeedhack)
				flTargetRatio = flRatio;

			static float flInterpRatio = 0.f;
			flInterpRatio += (flTargetRatio - flInterpRatio) * 0.03f;
			if (flInterpRatio < 0.01f)
				flInterpRatio = 0.f;

			int iFillW = std::clamp(int(flInterpRatio * iBarW), 0, iBarW);
			if (iFillW)
			{
				H::Draw.GradientRect(iPosX, iPosY, iFillW, iBarH, tBarRijinVTwoLeftGradient, tBarRijinVTwoRightGradient, true);

				float flPulseAlpha = std::clamp(std::sin(I::GlobalVars->curtime * 1.5f) * 100.f + 155.f, 0.f, 255.f);
				H::Draw.FillRect(iPosX, iPosY, iFillW, iBarH, tBarRijinVTwoLeftGradient.Alpha(flPulseAlpha));
			}

			H::Draw.LineRect(iPosX, iPosY, iBarW, iBarH, Vars::Menu::Theme::Accent.Value);

			static Color_t tStatusColor = Color_t(255, 255, 255, 255);
			std::string sStatusText;
			if (!bValidWeapon)
			{
				const bool bFlashOn = int(I::GlobalVars->curtime * 2) % 2 == 0;
				tStatusColor = bFlashOn ? Color_t(255, 255, 255, 255) : Color_t(207, 51, 42, 255);
				sStatusText = "WEAPON CANT DT";
			}
			else if (iTicks == iMax)
			{
				sStatusText = "READY";
				tStatusColor = { 10, 188, 105, 255 };
			}
			else if (Vars::Doubletap::RechargeTicks.Value)
			{
				sStatusText = "CHARGING";
				tStatusColor = { 255, 168, 29, 255 };
			}
			else if (!iTicks)
			{
				sStatusText = "NO CHARGE";
				tStatusColor = { 207, 51, 42, 255 };
			}
			else if (iTicks < iMax && !Vars::Doubletap::Warp.Value)
			{
				sStatusText = "NOT ENOUGH CHARGE";
				tStatusColor = { 207, 51, 42, 255 };
			}
			else
			{
				sStatusText = "DT IMPOSSIBLE";
				tStatusColor = { 207, 51, 42, 255 };
			}

			const int iLabelY = iPosY - fIndicatorFont.m_nTall + 3;
			constexpr Color_t tOutline = Color_t(0, 0, 0, 255);
			const int iTextWidth = H::Draw.GetTextSize(sStatusText.c_str(), fIndicatorFont).x;

			H::Draw.StringOutlined(fIndicatorFont, iPosX + 1, iLabelY, { 255,255,255,255 }, tOutline, ALIGN_LEFT, "CHARGE");
			H::Draw.StringOutlined(fIndicatorFont, iPosX + iBarW - iTextWidth - 1, iLabelY, tStatusColor, tOutline, ALIGN_LEFT, sStatusText.c_str());
			break;
		}
		case Vars::Menu::TickDisplayStyleEnum::LMAOBox:
		{
			constexpr Color_t tBlack = { 0, 0, 0, 255 };
			const int iSize = Vars::Menu::LBoxDTBarSize.Value;
			const float barBGSizeX = 70 * iSize;
			const float barBGSizeY = 3 * iSize;

			int iPosX = dtPos.x - barBGSizeX / 2, iPosY = dtPos.y + 5;

			auto pWeapon = H::Entities.GetWeapon();
			if (!pWeapon)
				return;

			if (!ValidWeapon(pWeapon))
					return;

			// draw background & outline
			H::Draw.FillRectOutline(iPosX - 1, iPosY - 1, barBGSizeX, barBGSizeY + 2, tBlack, Vars::Menu::Theme::Accent.Value);

			H::Draw.FillRectOutline(iPosX, iPosY, std::min(barBGSizeX * flRatio, barBGSizeX - 2), barBGSizeY, tBarLBoxFilled);
			if (m_bRecharge)
				H::Draw.StringOutlined(fFont, iPosX + barBGSizeX / 2, iPosY - fFont.m_nTall, Vars::Menu::Theme::Accent.Value, tBlack, ALIGN_CENTER, "CHARGING");
			break;
		}
		case Vars::Menu::TickDisplayStyleEnum::Ateris:
		{
			constexpr int iBarW = 180;
			constexpr int iBarH = 15;
			iPosX = dtPos.x - iBarW / 2, iPosY = dtPos.y + fFont.m_nTall + 6;

			H::Draw.FillRectOutline(iPosX, iPosY, iBarW, iBarH, Vars::Menu::Theme::Background.Value);

			Color_t tGradientStart = tBarAterisLeftGradient;
			Color_t tGradientEnd = tBarAterisRightGradient;

			const auto pWeapon = H::Entities.GetWeapon();
			const bool bValidWeapon = pWeapon && ValidWeapon(pWeapon);

			const auto& fIndicatorFont = H::Fonts.GetFont(FONT_ATERIS);

			float flTargetRatio = 0.f;
			if (!m_bSpeedhack)
				flTargetRatio = flRatio;

			static float flInterpRatio = 0.f;
			flInterpRatio += (flTargetRatio - flInterpRatio) * 0.03f;
			if (flInterpRatio < 0.01f)
				flInterpRatio = 0.f;

			tGradientStart.a = std::lerp(20.f, 255.f, flInterpRatio);

			const int iFillW = std::min(int(flInterpRatio * iBarW) + 10, iBarW);
			H::Draw.GradientRect(iPosX, iPosY, iFillW, iBarH / 2, tGradientStart, tGradientEnd, false, true);
			H::Draw.GradientRect(iPosX, iPosY + iBarH / 2, iFillW, iBarH / 2, tGradientEnd, tGradientStart, false, true);
			H::Draw.LineRect(iPosX, iPosY, iFillW, iBarH, Vars::Menu::Theme::Accent.Value);

			static Color_t tStatusColor = Color_t(255, 255, 255, 255);
			std::string sStatusText;
			if (!bValidWeapon || (m_iWait && Vars::Fakelag::Fakelag.Value))
			{
				tStatusColor = { 255, 100, 100, 255 };
				sStatusText = "DT IMPOSSIBLE";
			}
			else if (iTicks < iMax)
			{
				tStatusColor = { 255, 100, 100, 255 };
				sStatusText = "LOW CHARGE";
			}
			else if (m_iWait)
			{
				tStatusColor = { 255, 168, 29, 255 };
				sStatusText = "WAIT";
			}
			else if (m_bRecharge)
			{
				tStatusColor = { 255, 168, 29, 255 };
				sStatusText = "RECHARGING";
			}
			else
			{
				tStatusColor = { 10, 188, 105, 255 };
				sStatusText = "CHARGED";
			}

			const int iLabelY = iPosY - fIndicatorFont.m_nTall + 3;
			constexpr Color_t tOutline = Color_t(0, 0, 0, 255);
			const int iTextWidth = H::Draw.GetTextSize(sStatusText.c_str(), fIndicatorFont).x;

			H::Draw.StringOutlined(fIndicatorFont, iPosX + 1, iLabelY, { 255,255,255,255 }, tOutline, ALIGN_LEFT, "TICK SHIFTING");
			H::Draw.StringOutlined(fIndicatorFont, iPosX + iBarW - iTextWidth - 1, iLabelY, tStatusColor, tOutline, ALIGN_LEFT, sStatusText.c_str());
			break;
		}
		case Vars::Menu::TickDisplayStyleEnum::Deadflag:
		{
			constexpr int nBarW = 100;
			constexpr int nBarH = 4;

			iPosX -= 9;

			H::Draw.GradientRect(iPosX, iPosY, nBarW, nBarH, { 35, 35, 35, 255 }, { 20, 20, 20, 255 }, false);
			H::Draw.GradientRect(iPosX, iPosY, nBarW * flRatio, nBarH, tBarDeadFlagLeftGradient, tBarDeadFlagRightGradient, true);
			H::Draw.LineRect(iPosX - 1, iPosY - 1, nBarW + 2, nBarH + 2, { 20, 20, 20, 255 });

			Color_t tStatusColor{};
			std::string sStatusText;

			const auto pWeapon = H::Entities.GetWeapon();
			if (pWeapon && !ValidWeapon(pWeapon))
			{
				tStatusColor = { 191, 70, 70, 255 };
				sStatusText = "(RapidFire) Weapon Not Supported";
			}
			else if (m_bRecharge)
			{
				tStatusColor = { 191, 121, 50, 255 };
				sStatusText = std::format("(Recharging) {}/{}", m_iShiftedTicks, iMax);
			}
			else if (iTicks < iMax)
			{
				tStatusColor = { 191, 70, 70, 255 };
				sStatusText = std::format("(RapidFire) Too Expensive {} < {}", m_iShiftedTicks, iMax);
			}
			else if (m_iWait)
			{
				tStatusColor = { 191, 121, 50, 255 };
				sStatusText = std::format("(RapidFire) Wait {}/{}", iMax - m_iWait - 1, iMax);
			}
			else
			{
				tStatusColor = { 60, 160, 110, 255 };
				sStatusText = "(RapidFire) Ready";
			}

			const auto& fIndicatorFont = H::Fonts.GetFont(FONT_ATERIS);
			H::Draw.StringOutlined(fIndicatorFont, iPosX + nBarW / 2, iPosY + fIndicatorFont.m_nTall + 2, tStatusColor, { 0, 0, 0, 255 }, ALIGN_CENTER, sStatusText.c_str());
		}
		}
	}
	else
		H::Draw.StringOutlined(fFont, dtPos.x, dtPos.y + 2, Vars::Menu::Theme::Active.Value, Vars::Menu::Theme::Background.Value, ALIGN_TOP, std::format("Speedhack x{}", Vars::Speedhack::Amount.Value).c_str());
}
