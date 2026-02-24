#include "Announcer.h"

bool CAnnouncer::IsEventProperForKillCounter(const char* sWeaponName)
{
	if (strstr(sWeaponName, "deflect_"))
		return (!strcmp(sWeaponName, "deflect_promode"));
	if (!strcmp(sWeaponName, "world") || !strcmp(sWeaponName, "player"))
		return false;

	return true;
}

void CAnnouncer::Event(IGameEvent* pEvent, uint32_t uHash)
{
	switch (uHash)
	{
	case FNV1A::Hash32Const("player_spawn"):
	{
		int iIndex = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid"));
		int nLocal = I::EngineClient->GetLocalPlayer();
		if (iIndex == nLocal)
		{
			flLastKillTime = 0.f;
			iKillCounter = 0;
			iKillstreakCounter = 0;
		}
		return;
	}
	case FNV1A::Hash32Const("player_death"):
	{
		int iIndex = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("userid"));
		int nAttacker = I::EngineClient->GetPlayerForUserID(pEvent->GetInt("attacker"));
		int nLocal = I::EngineClient->GetLocalPlayer();

		if (iIndex == nLocal)
		{
			flLastKillTime = 0.f;
			iKillCounter = 0;
			iKillstreakCounter = 0;
			return;
		}

		if (nAttacker != nLocal)
			return;

		bool bIsEventProper = IsEventProperForKillCounter(pEvent->GetString("weapon"));
		if (bIsEventProper)
			iKillCounter++;
		iKillstreakCounter++;

		if (!Vars::Misc::Sound::QuakeAnnouncer.Value)
			return;

		if (I::GlobalVars->curtime - flLastKillTime < 2.5f)
		{
			switch (iKillstreakCounter)
			{
			case 2:
				I::MatSystemSurface->PlaySound("quake/doublekill.mp3");
				break;
			case 3:
				I::MatSystemSurface->PlaySound("quake/triplekill.mp3");
				break;
			case 4:
				I::MatSystemSurface->PlaySound("quake/multikill.mp3");
				break;
			case 5:
				I::MatSystemSurface->PlaySound("quake/megakill.mp3");
				break;
			case 6:
				I::MatSystemSurface->PlaySound("quake/ultrakill.mp3");
				break;
			case 7:
				I::MatSystemSurface->PlaySound("quake/monsterkill.mp3");
				break;
			case 8:
				I::MatSystemSurface->PlaySound("quake/ludicrouskill.mp3");
				break;
			default:
				I::MatSystemSurface->PlaySound("quake/holyshit.mp3");
			}
		}
		else
			iKillstreakCounter = 1;

		switch (pEvent->GetInt("customkill"))
		{
		case TF_DMG_CUSTOM_HEADSHOT:
		case TF_DMG_CUSTOM_HEADSHOT_DECAPITATION:
			I::MatSystemSurface->PlaySound("quake/headshot.mp3");
		}

		int iDamageBits = pEvent->GetInt("damagebits");
		if (iDamageBits & DMG_SLASH || iDamageBits & DMG_CLUB)
			I::MatSystemSurface->PlaySound("quake/humiliation.mp3");

		if (iKillCounter > 0)
		{
			switch (iKillCounter)
			{
			case 5:
				I::MatSystemSurface->PlaySound("quake/killingspree.mp3");
				break;
			case 10:
				I::MatSystemSurface->PlaySound("quake/rampage.mp3");
				break;
			case 15:
				I::MatSystemSurface->PlaySound("quake/dominating.mp3");
				break;
			case 20:
				I::MatSystemSurface->PlaySound("quake/unstoppable.mp3");
				break;
			default:
				if (iKillCounter % 5 == 0)
					I::MatSystemSurface->PlaySound("quake/godlike.mp3");
			}
			if (bIsEventProper)
				flLastKillTime = I::GlobalVars->curtime;
			return;
		}
	}
	}
}