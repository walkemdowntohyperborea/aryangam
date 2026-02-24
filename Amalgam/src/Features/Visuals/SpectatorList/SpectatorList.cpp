#include "SpectatorList.h"

#include "../../Players/PlayerUtils.h"
#include "../../Spectate/Spectate.h"

bool CSpectatorList::GetSpectators(CTFPlayer* pTarget, bool bAllowDead)
{
	m_vSpectators.clear();

	auto pResource = H::Entities.GetResource();
	if (!pResource)
		return false;

	int iTarget = pTarget->entindex();
	for(int n = 1; n <= I::EngineClient->GetMaxClients(); n++)
	{
		auto pPlayer = I::ClientEntityList->GetClientEntity(n)->As<CTFPlayer>();
		bool bLocal = n == I::EngineClient->GetLocalPlayer();

		if (pResource->m_bValid(n) && !pResource->IsFakePlayer(n)
			&& pResource->m_iTeam(I::EngineClient->GetLocalPlayer()) != TEAM_SPECTATOR && pResource->m_iTeam(n) == TEAM_SPECTATOR)
		{
			m_vSpectators.emplace_back(F::PlayerUtils.GetPlayerName(n, pResource->GetName(n)), "possible", -1.f, false, n);
			continue;
		}

		if (pTarget->entindex() == n || pResource->IsFakePlayer(n)
			|| !pPlayer || !pPlayer->IsPlayer() || pPlayer->IsAlive()
			|| pTarget->IsDormant() != pPlayer->IsDormant()
			|| pResource->m_iTeam(iTarget) != pResource->m_iTeam(n))
		{
			if (m_mRespawnCache.contains(n))
				m_mRespawnCache.erase(n);
			continue;
		}

		int iObserverTarget = !pPlayer->IsDormant() ? pPlayer->m_hObserverTarget().GetEntryIndex() : iTarget;
		int iObserverMode = pPlayer->m_iObserverMode();
		if (bLocal && F::Spectate.HasTarget())
		{
			iObserverTarget = F::Spectate.m_hOriginalTarget.GetEntryIndex();
			iObserverMode = F::Spectate.m_iOriginalMode;
		}

		if(iObserverTarget != iTarget || bLocal && !I::EngineClient->IsPlayingDemo() && !F::Spectate.HasTarget())
		{
			if (m_mRespawnCache.contains(n))
				m_mRespawnCache.erase(n);
			continue;
		}

		const char* sMode = "possible";
		if (!pPlayer->IsDormant())
		{
			switch (iObserverMode)
			{
			case OBS_MODE_FIRSTPERSON: sMode = "1st"; break;
			case OBS_MODE_THIRDPERSON: sMode = "3rd"; break;
			case OBS_MODE_DEATHCAM: if (bAllowDead) sMode = "death"; else continue; break;
			case OBS_MODE_FREEZECAM: if (bAllowDead) sMode = "freeze"; else continue; break;
			default: continue;
			}
		}

		float flRespawnTime = 0.f, flRespawnIn = -1.f;
		bool bRespawnTimeIncreased = false;
		if (pPlayer->IsInValidTeam())
		{
			flRespawnTime = pResource->m_flNextRespawnTime(n);
			flRespawnIn = std::max(floorf(flRespawnTime - TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick)), 0.f);
			if (!m_mRespawnCache.contains(n))
				m_mRespawnCache[n] = flRespawnTime;
			else if (m_mRespawnCache[n] + 0.5f < flRespawnTime)
				bRespawnTimeIncreased = true;
		}

		m_vSpectators.emplace_back(F::PlayerUtils.GetPlayerName(n, pResource->GetName(n)), sMode, flRespawnIn, bRespawnTimeIncreased, n);
	}

	return !m_vSpectators.empty();
}

void CSpectatorList::Draw(CTFPlayer* pLocal)
{
	if (!(Vars::Menu::Indicators.Value & Vars::Menu::IndicatorsEnum::Spectators))
	{
		m_mRespawnCache.clear();
		return;
	}

	auto pTarget = pLocal;
	switch (pLocal->m_iObserverMode())
	{
	case OBS_MODE_FIRSTPERSON:
	case OBS_MODE_THIRDPERSON:
		pTarget = pLocal->m_hObserverTarget()->As<CTFPlayer>();
	}
	if (!pTarget || !pTarget->IsPlayer())
		return;

	int x = Vars::Menu::SpectatorsDisplay.Value.x;
	int y = Vars::Menu::SpectatorsDisplay.Value.y + 8;
	int iconOffset = 0;
	const auto& fFont = H::Fonts.GetFont(FONT_INDICATORS);
	const int nTall = fFont.m_nTall + H::Draw.Scale(3);

	auto pResource = H::Entities.GetResource();
	int iIndex = pTarget->entindex();
	const char* sName = pTarget != pLocal ? F::PlayerUtils.GetPlayerName(iIndex, pResource->GetName(iIndex)) : "You";
	switch (Vars::Menu::IndicatorDisplayStyle.Value)
	{
	case Vars::Menu::IndicatorDisplayStyleEnum::Ateris:
	{
		Color_t tOutlineColor = Vars::Menu::Theme::Background.Value.Lerp({ 127, 127, 127 }, 2.f / 9, LerpEnum::NoAlpha);
		int iRectHeight = std::ceil(H::Draw.GetTextSize("(1st)", fFont).y);
		if (m_vSpectators.size() > 1)
			iRectHeight += (int)m_vSpectators.size() * nTall;

		// spectators text / background
		H::Draw.FillRectOutline(x, y, 200, 15 + iRectHeight, Vars::Menu::Theme::Background.Value, tOutlineColor);
		H::Draw.StringOutlined(fFont, x + 6, y + 2, Vars::Menu::Theme::Active.Value, { 0, 0, 0,255 }, ALIGN_TOPLEFT, "spectators");

		// separator line
		H::Draw.Line(x, y + 20, x + 200, y + 20, Vars::Menu::Theme::Accent.Value);

		// draw info background
		{
			Color_t tColor = Vars::Menu::Theme::Background.Value.Lerp({ 127, 127, 127 }, 1.5f / 9, LerpEnum::NoAlpha);
			H::Draw.FillRectOutline(x + 3, y + 26, 194, iRectHeight - 14, tColor, tOutlineColor);
		}

		// draw spectators
		if(GetSpectators(pTarget, true))
		{
			Color_t tColor = Vars::Menu::Theme::Active.Value;
			static bool first = true;
			first = true;
			for (auto& tSpectator : m_vSpectators)
			{
				if(!first)
					y += nTall;
				H::Draw.StringOutlined(fFont, x + 6, y + 27, tColor, { 0, 0, 0,255 }, ALIGN_TOPLEFT, tSpectator.m_sName.c_str());

				std::string sMode = std::format("({})", tSpectator.m_sMode);
				H::Draw.StringOutlined(fFont, x + 192 - H::Draw.GetTextSize(sMode.c_str(), fFont).x, y + 27, tColor, {0, 0, 0,255}, ALIGN_TOPLEFT, sMode.c_str());
				
				first = false;
			}
		}
		break;
	}
	case Vars::Menu::IndicatorDisplayStyleEnum::RijiNv2:
	{
		break;
	}
	case Vars::Menu::IndicatorDisplayStyleEnum::Nitro:
	{
		if (!GetSpectators(pTarget))
			return;

		x -= H::Draw.Scale(45);

		H::Draw.StringOutlined(fFont, x, y, Vars::Menu::Theme::Accent.Value, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, std::format("Spectating {}:", sName).c_str());
		for (auto& tSpectator : m_vSpectators)
		{
			y += nTall;

			Color_t tColor = Vars::Menu::Theme::Active.Value;

			if (H::Entities.IsFriend(tSpectator.m_iIndex))
				tColor = F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(FRIEND_TAG)].m_tColor;
			else if (H::Entities.InParty(tSpectator.m_iIndex))
				tColor = F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(PARTY_TAG)].m_tColor;
			else if (H::Entities.IsF2P(tSpectator.m_iIndex))
				tColor = F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(F2P_TAG)].m_tColor;
			else if (tSpectator.m_bRespawnTimeIncreased)
				tColor = F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(CHEATER_TAG)].m_tColor;
			else if (FNV1A::Hash32(tSpectator.m_sMode) == FNV1A::Hash32Const("1st"))
				tColor = tColor.Lerp({ 255, 150, 0, 255 }, 0.5f);

			if (tSpectator.m_flRespawnIn != -1.f)
				H::Draw.StringOutlined(fFont, x + iconOffset, y, tColor, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, std::format("{} {} (respawn {}s)", tSpectator.m_sName, tSpectator.m_sMode, tSpectator.m_flRespawnIn).c_str());
			else
				H::Draw.StringOutlined(fFont, x + iconOffset, y, tColor, Vars::Menu::Theme::Background.Value, ALIGN_LEFT, std::format("{} {}", tSpectator.m_sName, tSpectator.m_sMode).c_str());
		}
		break;
	}
	}
}