#include "ESP.h"

#include "../Groups/Groups.h"
#include "../../Players/PlayerUtils.h"
#include "../../Spectate/Spectate.h"
#include "../../Simulation/MovementSimulation/MovementSimulation.h"
#include "../../Simulation/ProjectileSimulation/ProjectileSimulation.h"
#include "../../../Hooks/Hooks.h"
#include "WeaponNames/WeaponNames.h"

MAKE_SIGNATURE(CTFPlayerSharedUtils_GetEconItemViewByLoadoutSlot, "client.dll", "48 89 6C 24 ? 56 41 54 41 55 41 56 41 57 48 83 EC", 0x0);

static inline Color_t GetHealthColor(CBaseEntity* pEntity, Group_t* pGroup, bool bAllowOverheal)
{
	if (pEntity->IsBuilding())
	{
		const auto pBuilding = pEntity->As<CBaseObject>();

		const float flHealth = pBuilding->m_iHealth(), flMaxHealth = pBuilding->m_iMaxHealth();
		const float health = std::clamp(flHealth / flMaxHealth, 0.f, 1.f);

		if (health < 0.5f)
			return pGroup->m_tHealthColorLow.Lerp(pGroup->m_tHealthColorMid, health * 2.0f);

		return pGroup->m_tHealthColorMid.Lerp(pGroup->m_tHealthColorHigh, (health - 0.5f) * 2.0f);
	}
	else if (pEntity->IsPlayer())
	{
		const auto pPlayer = pEntity->As<CTFPlayer>();

		const float flHealth = pPlayer->m_iHealth(), flMaxHealth = pPlayer->GetMaxHealth();
		const float health = bAllowOverheal ? std::max(flHealth / flMaxHealth, 0.f) : std::clamp(flHealth / flMaxHealth, 0.f, 1.f);

		if (health > 1.0f)
			return pGroup->m_tHealthColorOverheal;
		else if (health < 0.5f)
			return pGroup->m_tHealthColorLow.Lerp(pGroup->m_tHealthColorMid, health * 2.0f);
		
		return pGroup->m_tHealthColorMid.Lerp(pGroup->m_tHealthColorHigh, (health - 0.5f) * 2.0f);
	}

	return Color_t(255, 255, 255, 255);
}

static inline void StorePlayer(CTFPlayer* pPlayer, CTFPlayer* pLocal, Group_t* pGroup, std::unordered_map<CBaseEntity*, PlayerCache_t>& mCache)
{
	int iIndex = pPlayer->entindex();
	

	if (int iObserverMode = pLocal->m_iObserverMode();
		iObserverMode == OBS_MODE_FIRSTPERSON || iObserverMode == OBS_MODE_THIRDPERSON
		? !I::Input->CAM_IsThirdPerson() && iIndex == I::EngineClient->GetLocalPlayer()
		: iObserverMode == OBS_MODE_FIRSTPERSON && pLocal->m_hObserverTarget().GetEntryIndex() == iIndex)
		return;

	auto pWeapon = pPlayer->m_hActiveWeapon()->As<CTFWeaponBase>();
	auto pResource = H::Entities.GetResource();
	bool bLocal = pPlayer->entindex() == I::EngineClient->GetLocalPlayer();
	int iClassNum = pPlayer->m_iClass();

	PlayerCache_t& tCache = mCache[pPlayer];
	tCache.m_flAlpha = pGroup->m_tColor.a / 255.f;
	tCache.m_tColor = pGroup->m_bESPUseGroupColor ? F::Groups.GetColor(pPlayer, pGroup).Alpha(255) : Color_t(255, 255, 255, 255);
	tCache.m_bBox = pGroup->m_iESP & ESPEnum::Box;
	tCache.m_bBones = pGroup->m_iESP & ESPEnum::Bones;

	if (pGroup->m_iESP & ESPEnum::Distance && !bLocal)
	{
		Vec3 vDelta = pPlayer->m_vecOrigin() - pLocal->m_vecOrigin();
		tCache.m_vConditionText.emplace_back(ALIGN_BOTTOM, std::format("[{:.0f}M]", vDelta.Length2D() / 41), tCache.m_tColor, pGroup->m_tOutlineColor);
	}

	if (pResource)
	{
		if (pGroup->m_iESP & ESPEnum::Name)
			tCache.m_vText.emplace_back(ALIGN_TOP, F::PlayerUtils.GetPlayerName(iIndex, pResource->GetName(iIndex)), tCache.m_tColor, pGroup->m_tOutlineColor);

		if (pGroup->m_iESP & (ESPEnum::Labels | ESPEnum::Priority) && !pResource->IsFakePlayer(iIndex))
		{
			uint32_t uAccountID = pResource->m_iAccountID(iIndex);

			if (pGroup->m_iESP & ESPEnum::Priority)
			{
				if (auto pTag = F::PlayerUtils.GetSignificantTag(uAccountID, 1))
				{
					std::string sName = pTag->m_sName;
					std::transform(sName.begin(), sName.end(), sName.begin(), ::toupper);
					tCache.m_vText.emplace_back(ALIGN_TOP, sName, pTag->m_tColor, pGroup->m_tOutlineColor);
				}

			}

			if (pGroup->m_iESP & ESPEnum::Labels)
			{
				std::vector<PriorityLabel_t*> vTags = {};
				for (auto& iID : F::PlayerUtils.m_mPlayerTags[uAccountID])
				{
					auto pTag = F::PlayerUtils.GetTag(iID);
					if (pTag && pTag->m_bLabel)
						vTags.push_back(pTag);
				}

				if (H::Entities.IsFriend(uAccountID))
				{
					auto pTag = &F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(FRIEND_TAG)];
					if (pTag->m_bLabel)
						vTags.push_back(pTag);
				}
				else if (H::Entities.InParty(uAccountID))
				{
					auto pTag = &F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(PARTY_TAG)];
					if (pTag->m_bLabel)
						vTags.push_back(pTag);
				}

				if (H::Entities.IsF2P(uAccountID))
				{
					auto pTag = &F::PlayerUtils.m_vTags[F::PlayerUtils.TagToIndex(F2P_TAG)];
					if (pTag->m_bLabel)
						vTags.push_back(pTag);
				}

				if (vTags.size())
				{
					std::sort(vTags.begin(), vTags.end(), [&](const auto a, const auto b) -> bool
						{
							// sort by priority if unequal
							if (a->m_iPriority != b->m_iPriority)
								return a->m_iPriority > b->m_iPriority;

							return a->m_sName < b->m_sName;
						});

					for (auto& pTag : vTags)
					{
						std::string sName = pTag->m_sName;
						std::transform(sName.begin(), sName.end(), sName.begin(), ::toupper);
						tCache.m_vText.emplace_back(ALIGN_TOP, sName, pTag->m_tColor, pGroup->m_tOutlineColor);
					}
				}
			}
		}
	}

	float flHealth = pPlayer->m_iHealth(), flMaxHealth = pPlayer->GetMaxHealth();
	if (pGroup->m_iESP & ESPEnum::HealthBar)
	{
		tCache.m_flHealth = flHealth > flMaxHealth
			? 1.f + std::clamp((flHealth - flMaxHealth) / (floorf(flMaxHealth / 10.f) * 5), 0.f, 1.f)
			: std::clamp(flHealth / flMaxHealth, 0.f, 1.f);
		Color_t tColor = GetHealthColor(pPlayer, pGroup, false);
		tCache.m_vBars.emplace_back(ALIGN_LEFT, tCache.m_flHealth, tColor, pGroup->m_tHealthColorOverheal);
	}
	if (pGroup->m_iESP & ESPEnum::HealthText)
		tCache.m_vConditionText.emplace_back(ALIGN_BOTTOMLEFT, std::format("{}", flHealth), tCache.m_tColor, pGroup->m_tOutlineColor);

	if (pGroup->m_iESP & (ESPEnum::UberBar | ESPEnum::UberText) && iClassNum == TF_CLASS_MEDIC)
	{
		auto pMediGun = pPlayer->GetWeaponFromSlot(SLOT_SECONDARY);
		if (pMediGun && pMediGun->GetClassID() == ETFClassID::CWeaponMedigun)
		{
			float flUber = std::clamp(pMediGun->As<CWeaponMedigun>()->m_flChargeLevel(), 0.f, 1.f);
			if (pGroup->m_iESP & ESPEnum::UberBar)
				tCache.m_vBars.emplace_back(ALIGN_BOTTOM, flUber, Vars::Colors::Uber.Value, Color_t(), false);
			if (pGroup->m_iESP & ESPEnum::UberText)
				tCache.m_vConditionText.emplace_back(pGroup->m_iESP & ESPEnum::UberBar ? ALIGN_BOTTOMRIGHT : ALIGN_TOPRIGHT, std::format("{:.0f}%", flUber * 100), tCache.m_tColor, pGroup->m_tOutlineColor);
		}
	}

	if (pGroup->m_iESP & ESPEnum::ClassIcon)
		tCache.m_iClassIcon = iClassNum;
	if (pGroup->m_iESP & ESPEnum::ClassText)
	{
		std::string sClass = SDK::GetClassByIndex(iClassNum);
		std::transform(sClass.begin(), sClass.end(), sClass.begin(), ::toupper);
		tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, sClass, tCache.m_tColor, pGroup->m_tOutlineColor);
	}

	if (pGroup->m_iESP & ESPEnum::WeaponIcon && pWeapon)
		tCache.m_pWeaponIcon = pWeapon->GetWeaponIcon();
	if (pGroup->m_iESP & ESPEnum::WeaponText && pWeapon)
		tCache.m_vConditionText.emplace_back(ALIGN_BOTTOM, F::WeaponNames.GetWeaponNameUpper(pWeapon), tCache.m_tColor, pGroup->m_tOutlineColor);

	{
		if (pGroup->m_iESP & ESPEnum::LagCompensation && !pPlayer->IsDormant() && !bLocal)
		{
			if (H::Entities.GetLagCompensation(iIndex))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "LAGCOMP", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
		}

		if (pGroup->m_iESP & ESPEnum::Ping && pResource && !bLocal)
		{
			int iPing = pResource->m_iPing(iIndex);
			if (iPing && (iPing >= 200 || iPing <= 5))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, std::format("{} MS", iPing), Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
		}

		if (pGroup->m_iESP & ESPEnum::KDR && pResource && !bLocal)
		{
			int iKills = pResource->m_iScore(iIndex), iDeaths = pResource->m_iDeaths(iIndex);
			if (iKills >= 20)
			{
				int iKDR = iKills / std::max(iDeaths, 1);
				if (iKDR >= 10)
					tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "HIGH KD", Vars::Colors::IndicatorTextMid.Value, pGroup->m_tOutlineColor);
			}
		}

		// Buffs
		if (pGroup->m_iESP & ESPEnum::Buffs)
		{
			if (pPlayer->IsUbered())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "UBERED", Vars::Colors::Uber.Value, pGroup->m_tOutlineColor);
			else if (pPlayer->InCond(TF_COND_PHASE))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, std::format("BONK {:.1f}S", Math::RemapVal(pPlayer->m_flEnergyDrinkMeter(), 100.f, 0.f, 8.f, 0.f)).c_str(), Vars::Colors::Uber.Value, pGroup->m_tOutlineColor);

			if (pPlayer->InCond(TF_COND_REGENONDAMAGEBUFF))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "CONCHEROR", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);

			if (pPlayer->InCond(TF_COND_DEFENSEBUFF))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "BATTALIONS", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			else if (pPlayer->InCond(TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "DEFENSE BUFF", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);

			bool bCrits = false, bMiniCrits = false;
			if (pPlayer->IsCritBoosted())
				pWeapon&& SDK::AttribHookValue(0, "crits_become_minicrits", pWeapon) == 1 ? bMiniCrits = true : bCrits = true;
			if (pPlayer->IsMiniCritBoosted())
				pWeapon&& SDK::AttribHookValue(0, "minicrits_become_crits", pWeapon) == 1 ? bCrits = true : bMiniCrits = true;
			if (pWeapon && SDK::AttribHookValue(0, "crit_while_airborne", pWeapon) && pPlayer->InCond(TF_COND_BLASTJUMPING))
				bCrits = true, bMiniCrits = false;

			if (bCrits)
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "CRITS", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			else if (bMiniCrits)
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "MINI-CRITS", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);

			if (pPlayer->InCond(TF_COND_BULLET_IMMUNE))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "BULLET+++", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			else if (pPlayer->InCond(TF_COND_MEDIGUN_UBER_BULLET_RESIST))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "BULLET++", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			else if (pPlayer->InCond(TF_COND_MEDIGUN_SMALL_BULLET_RESIST))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "BULLET+", tCache.m_tColor, pGroup->m_tOutlineColor);

			if (pPlayer->InCond(TF_COND_BLAST_IMMUNE))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "BLAST+++", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			else if (pPlayer->InCond(TF_COND_MEDIGUN_UBER_BLAST_RESIST))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "BLAST++", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			else if (pPlayer->InCond(TF_COND_MEDIGUN_SMALL_BLAST_RESIST))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "BLAST+", tCache.m_tColor, pGroup->m_tOutlineColor);

			if (pPlayer->InCond(TF_COND_FIRE_IMMUNE))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "FIRE+++", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			else if (pPlayer->InCond(TF_COND_MEDIGUN_UBER_FIRE_RESIST))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "FIRE++", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			else if (pPlayer->InCond(TF_COND_MEDIGUN_SMALL_FIRE_RESIST))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "FIRE+", tCache.m_tColor, pGroup->m_tOutlineColor);

			if (pPlayer->InCond(TF_COND_RUNE_STRENGTH))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "STRENGTH", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pPlayer->InCond(TF_COND_RUNE_HASTE))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "HASTE", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pPlayer->InCond(TF_COND_RUNE_REGEN))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "REGEN", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pPlayer->InCond(TF_COND_RUNE_RESIST))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "RESISTANCE", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pPlayer->InCond(TF_COND_RUNE_VAMPIRE))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "VAMPIRE", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pPlayer->InCond(TF_COND_RUNE_REFLECT))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "REFLECT", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pPlayer->InCond(TF_COND_RUNE_PRECISION))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "PRECISION", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pPlayer->InCond(TF_COND_RUNE_AGILITY))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "AGILITY", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pPlayer->InCond(TF_COND_RUNE_KNOCKOUT))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "KNOCKOUT", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pPlayer->InCond(TF_COND_RUNE_KING))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "KING", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pPlayer->InCond(TF_COND_RUNE_PLAGUE))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "PLAGUE", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pPlayer->InCond(TF_COND_RUNE_SUPERNOVA))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "SUPERNOVA", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pPlayer->InCond(TF_COND_POWERUPMODE_DOMINANT))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "DOMINANT", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);

			if (U::Hooks.m_TF_IsHolidayActive.fastcall<bool>(kHoliday_Halloween))
			{
				static const char* kSpellNames[] = {
				"FIREBALL", "BATS", "HEAL", "PUMPKINS", "JUMP", "STEALTH", "TELEPORT", "LIGHTNING",
				"MINIFY", "METEORS", "MONOCULUS", "SKELETONS", "GLOVE", "PARACHUTE", "HEAL", "BOMB"
				};
				for (int i = 0; i < MAX_WEAPONS; i++)
				{
					auto pWeapon = pPlayer->GetWeaponFromSlot(i)->As<CTFSpellBook>();
					if (!pWeapon || pWeapon->GetWeaponID() != TF_WEAPON_SPELLBOOK || !pWeapon->m_iSpellCharges())
						continue;

					tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, kSpellNames[pWeapon->m_iSelectedSpellIndex()], tCache.m_tColor, pGroup->m_tOutlineColor);
				}
			}

			if (pPlayer->InCond(TF_COND_RADIUSHEAL) ||
				pPlayer->InCond(TF_COND_HEALTH_BUFF) ||
				pPlayer->InCond(TF_COND_RADIUSHEAL_ON_DAMAGE) ||
				pPlayer->InCond(TF_COND_HALLOWEEN_QUICK_HEAL) ||
				pPlayer->InCond(TF_COND_HALLOWEEN_HELL_HEAL) ||
				pPlayer->InCond(TF_COND_KING_BUFFED) ||
				pPlayer->InCond(TF_COND_MEGAHEAL))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "HP++", Vars::Colors::IndicatorTextGood.Value, pGroup->m_tOutlineColor);
			else if (pPlayer->InCond(TF_COND_HEALTH_OVERHEALED))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "HP+", Vars::Colors::IndicatorTextGood.Value, pGroup->m_tOutlineColor);

			if (int iHeads = pPlayer->m_iDecapitations(); iHeads > 0)
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, std::format("{} HEAD{}", iHeads, iHeads != 1 ? "S" : "").c_str(),
					iHeads >= 3 ? Vars::Colors::IndicatorTextBad.Value : Vars::Colors::IndicatorTextMid.Value, pGroup->m_tOutlineColor);
		}

		// Debuffs
		if (pGroup->m_iESP & ESPEnum::Debuffs)
		{
			if (pPlayer->InCond(TF_COND_URINE))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "JARATE", Vars::Colors::IndicatorTextGood.Value, pGroup->m_tOutlineColor);
			else if (pPlayer->InCond(TF_COND_MARKEDFORDEATH) || pPlayer->InCond(TF_COND_MARKEDFORDEATH_SILENT))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "MARKED", Vars::Colors::IndicatorTextGood.Value, pGroup->m_tOutlineColor);

			if (pPlayer->InCond(TF_COND_MAD_MILK))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "MILK", Vars::Colors::IndicatorTextGood.Value, pGroup->m_tOutlineColor);

			if (pPlayer->InCond(TF_COND_GAS))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "GAS", Vars::Colors::IndicatorTextGood.Value, pGroup->m_tOutlineColor);

			if (pPlayer->InCond(TF_COND_STUNNED))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "SLOW", Vars::Colors::IndicatorTextGood.Value, pGroup->m_tOutlineColor);

			if (pPlayer->InCond(TF_COND_MELEE_ONLY))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "FORCED MELEE", Vars::Colors::IndicatorTextGood.Value, pGroup->m_tOutlineColor);
		}

		// Misc
		if (pGroup->m_iESP & ESPEnum::Flags)
		{
			if (pPlayer->m_bFeignDeathReady())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "DR", tCache.m_tColor, pGroup->m_tOutlineColor);

			if (float flInvis = pPlayer->GetEffectiveInvisibilityLevel())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, std::format("INVISIBLE {:.0f}%", flInvis * 100).c_str(), tCache.m_tColor, pGroup->m_tOutlineColor);

			if (pPlayer->IsTaunting())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "TAUNTING", Vars::Colors::IndicatorTextMisc.Value, pGroup->m_tOutlineColor);

			if (pPlayer->InCond(TF_COND_DISGUISING) || pPlayer->InCond(TF_COND_DISGUISED))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "DISGUISED", tCache.m_tColor, pGroup->m_tOutlineColor);

			if (pPlayer->InCond(TF_COND_AIMING) || pPlayer->InCond(TF_COND_ZOOMED))
			{
				switch (pWeapon ? pWeapon->GetWeaponID() : -1)
				{
				case TF_WEAPON_MINIGUN:
					tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "REVVED", tCache.m_tColor, pGroup->m_tOutlineColor);
					break;
				case TF_WEAPON_SNIPERRIFLE:
				case TF_WEAPON_SNIPERRIFLE_CLASSIC:
				case TF_WEAPON_SNIPERRIFLE_DECAP:
				{
					if (bLocal)
					{
						float flCharge = Math::RemapVal(pWeapon->As<CTFSniperRifle>()->m_flChargedDamage(), 0.f, 150.f, 0.f, 100.f);
						tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, std::format("CHARGING {:.0f}%", flCharge), tCache.m_tColor, pGroup->m_tOutlineColor);
						break;
					}
					else
					{
						auto GetSniperDot = [](CBaseEntity* pEntity) -> CSniperDot*
							{
								for (auto pDot : H::Entities.GetGroup(EntityEnum::SniperDots))
								{
									if (pDot->m_hOwnerEntity().Get() == pEntity)
										return pDot->As<CSniperDot>();
								}
								return nullptr;
							};
						if (CSniperDot* pPlayerDot = GetSniperDot(pPlayer))
						{
							float flChargeTime = std::max(SDK::AttribHookValue(3.f, "mult_sniper_charge_per_sec", pWeapon), 1.5f);
							float flCharge = Math::RemapVal(TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick) - pPlayerDot->m_flChargeStartTime() - 0.3f, 0.f, flChargeTime, 0.f, 100.f);
							tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, std::format("CHARGING {:.0f}%", flCharge), tCache.m_tColor, pGroup->m_tOutlineColor);
							break;
						}
					}
					tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "CHARGING", tCache.m_tColor, pGroup->m_tOutlineColor);
					break;
				}
				case TF_WEAPON_COMPOUND_BOW:
					if (bLocal)
					{
						float flCharge = Math::RemapVal(TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick) - pWeapon->As<CTFPipebombLauncher>()->m_flChargeBeginTime(), 0.f, 1.f, 0.f, 100.f);
						tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, std::format("CHARGING {:.0f}%", flCharge), tCache.m_tColor, pGroup->m_tOutlineColor);
						break;
					}
					tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "CHARGING", tCache.m_tColor, pGroup->m_tOutlineColor);
					break;
				default:
					tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "CHARGING", tCache.m_tColor, pGroup->m_tOutlineColor);
				}
			}

			if (pPlayer->InCond(TF_COND_SHIELD_CHARGE))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "CHARGING", tCache.m_tColor, pGroup->m_tOutlineColor);
		}
	}
}

static inline void StoreBuilding(CBaseObject* pBuilding, CTFPlayer* pLocal, Group_t* pGroup, std::unordered_map<CBaseEntity*, BuildingCache_t>& mCache)
{
	if (pBuilding->m_bPlacing())
		return;

	auto pOwner = pBuilding->m_hBuilder().Get();
	int iIndex = pOwner ? pOwner->entindex() : -1;

	bool bIsMini = pBuilding->m_bMiniBuilding();

	BuildingCache_t& tCache = mCache[pBuilding];
	tCache.m_flAlpha = pGroup->m_tColor.a / 255.f;
	tCache.m_tColor = pGroup->m_bESPUseGroupColor ? F::Groups.GetColor(pOwner ? pOwner : pBuilding, pGroup).Alpha(255) : Color_t(255, 255, 255, 255);
	tCache.m_bBox = pGroup->m_iESP & ESPEnum::Box;

	if (pGroup->m_iESP & ESPEnum::Distance)
	{
		Vec3 vDelta = pBuilding->m_vecOrigin() - pLocal->m_vecOrigin();
		tCache.m_vConditionText.emplace_back(ALIGN_BOTTOM, std::format("[{:.0f}M]", vDelta.Length2D() / 41), tCache.m_tColor, pGroup->m_tOutlineColor);
	}

	if (pGroup->m_iESP & ESPEnum::Name)
	{
		const char* szName = "Building";
		switch (pBuilding->GetClassID())
		{
		case ETFClassID::CObjectSentrygun: szName = bIsMini ? "Mini Sentry" : "Sentry"; break;
		case ETFClassID::CObjectDispenser: szName = "Dispenser"; break;
		case ETFClassID::CObjectTeleporter: szName = pBuilding->m_iObjectMode() ? "Teleporter Exit" : "Teleporter Entrance";
		}
		tCache.m_vText.emplace_back(ALIGN_TOP, szName, tCache.m_tColor, pGroup->m_tOutlineColor);
	}

	if(pGroup->m_iESP & ESPEnum::Level && !bIsMini)
		tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, std::format("LEVEL {}", pBuilding->m_iUpgradeLevel()), tCache.m_tColor, pGroup->m_tOutlineColor);

	float flHealth = pBuilding->m_iHealth(), flMaxHealth = pBuilding->m_iMaxHealth();
	if (pGroup->m_iESP & ESPEnum::HealthBar)
	{
		tCache.m_flHealth = std::clamp(flHealth / flMaxHealth, 0.f, 1.f);
		Color_t tColor = GetHealthColor(pBuilding, pGroup, false);
		tCache.m_vBars.emplace_back(ALIGN_LEFT, tCache.m_flHealth, tColor, pGroup->m_tHealthColorOverheal);
	}
	if (pGroup->m_iESP & ESPEnum::HealthText)
		tCache.m_vConditionText.emplace_back(ALIGN_BOTTOMLEFT, std::format("{}", flHealth), tCache.m_tColor, pGroup->m_tOutlineColor);

	if (pGroup->m_iESP & ESPEnum::Owner && !pBuilding->m_bWasMapPlaced() && pOwner)
	{
		if (auto pResource = H::Entities.GetResource(); pResource)
			tCache.m_vText.emplace_back(ALIGN_TOP, F::PlayerUtils.GetPlayerName(iIndex, pResource->GetName(iIndex)), tCache.m_tColor, pGroup->m_tOutlineColor);
	}

	if (pGroup->m_iESP & ESPEnum::Flags)
	{
		float flConstructed = pBuilding->m_flPercentageConstructed();
		if (flConstructed < 1.f)
			tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, std::format("{:.0f}%", flConstructed * 100.f), tCache.m_tColor, pGroup->m_tOutlineColor);

		if (pBuilding->IsSentrygun() && pBuilding->As<CObjectSentrygun>()->m_bPlayerControlled())
			tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "WRANGLED", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);

		if (pBuilding->m_bHasSapper())
			tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "SAPPED", Vars::Colors::IndicatorTextGood.Value, pGroup->m_tOutlineColor);
		else if (pBuilding->m_bDisabled())
			tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "DISABLED", Vars::Colors::IndicatorTextGood.Value, pGroup->m_tOutlineColor);

		if (pBuilding->IsSentrygun() && !pBuilding->m_bBuilding())
		{
			int iShells, iMaxShells, iRockets, iMaxRockets; pBuilding->As<CObjectSentrygun>()->GetAmmoCount(iShells, iMaxShells, iRockets, iMaxRockets);
			if (!iShells)
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "NO AMMO", tCache.m_tColor, pGroup->m_tOutlineColor);
			if (!bIsMini && !iRockets)
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "NO ROCKETS", tCache.m_tColor, pGroup->m_tOutlineColor);
		}
	}
}

static inline const char* GetProjectileName(CBaseEntity* pProjectile)
{
	const char* sReturn = "Projectile";
	switch (pProjectile->GetClassID())
	{
	case ETFClassID::CTFWeaponBaseMerasmusGrenade: sReturn = "Bomb"; break;
	case ETFClassID::CTFGrenadePipebombProjectile: sReturn = pProjectile->As<CTFGrenadePipebombProjectile>()->HasStickyEffects() ? "Sticky" : "Pipe"; break;
	case ETFClassID::CTFStunBall: sReturn = "Baseball"; break;
	case ETFClassID::CTFBall_Ornament: sReturn = "Bauble"; break;
	case ETFClassID::CTFProjectile_Jar: sReturn = "Jarate"; break;
	case ETFClassID::CTFProjectile_Cleaver: sReturn = "Cleaver"; break;
	case ETFClassID::CTFProjectile_JarGas: sReturn = "Gas"; break;
	case ETFClassID::CTFProjectile_JarMilk:
	case ETFClassID::CTFProjectile_ThrowableBreadMonster: sReturn = "Milk"; break;
	case ETFClassID::CTFProjectile_SpellBats:
	case ETFClassID::CTFProjectile_SpellKartBats: sReturn = "Bats"; break;
	case ETFClassID::CTFProjectile_SpellMeteorShower: sReturn = "Meteor shower"; break;
	case ETFClassID::CTFProjectile_SpellMirv:
	case ETFClassID::CTFProjectile_SpellPumpkin: sReturn = "Pumpkin"; break;
	case ETFClassID::CTFProjectile_SpellSpawnBoss: sReturn = "Monoculus"; break;
	case ETFClassID::CTFProjectile_SpellSpawnHorde:
	case ETFClassID::CTFProjectile_SpellSpawnZombie: sReturn = "Skeleton"; break;
	case ETFClassID::CTFProjectile_SpellTransposeTeleport: sReturn = "Teleport"; break;
	case ETFClassID::CTFProjectile_Arrow: sReturn = pProjectile->As<CTFProjectile_Arrow>()->m_iProjectileType() == TF_PROJECTILE_BUILDING_REPAIR_BOLT ? "Repair" : "Arrow"; break;
	case ETFClassID::CTFProjectile_GrapplingHook: sReturn = "Grapple"; break;
	case ETFClassID::CTFProjectile_HealingBolt: sReturn = "Heal"; break;
	case ETFClassID::CTFProjectile_Rocket:
	case ETFClassID::CTFProjectile_EnergyBall:
	case ETFClassID::CTFProjectile_SentryRocket: sReturn = "Rocket"; break;
	case ETFClassID::CTFProjectile_BallOfFire: sReturn = "Fire"; break;
	case ETFClassID::CTFProjectile_MechanicalArmOrb: sReturn = "Short circuit"; break;
	case ETFClassID::CTFProjectile_SpellFireball: sReturn = "Fireball"; break;
	case ETFClassID::CTFProjectile_SpellLightningOrb: sReturn = "Lightning"; break;
	case ETFClassID::CTFProjectile_SpellKartOrb: sReturn = "Fist"; break;
	case ETFClassID::CTFProjectile_Flare: sReturn = "Flare"; break;
	case ETFClassID::CTFProjectile_EnergyRing: sReturn = "Energy"; break;
	}
	return sReturn;
}

static inline void StoreProjectile(CBaseEntity* pProjectile, CTFPlayer* pLocal, Group_t* pGroup, std::unordered_map<CBaseEntity*, EntityCache_t>& mCache)
{
	auto pOwner = F::ProjSim.GetEntities(pProjectile).second;
	int iIndex = pOwner ? pOwner->entindex() : -1;

	EntityCache_t& tCache = mCache[pProjectile];
	tCache.m_flAlpha = pGroup->m_tColor.a / 255.f;
	tCache.m_tColor = tCache.m_tColor = pGroup->m_bESPUseGroupColor ? F::Groups.GetColor(pOwner ? pOwner : pProjectile, pGroup).Alpha(255) : Color_t(255, 255, 255, 255);
	tCache.m_bBox = pGroup->m_iESP & ESPEnum::Box;

	if (pGroup->m_iESP & ESPEnum::Distance)
	{
		Vec3 vDelta = pProjectile->m_vecOrigin() - pLocal->m_vecOrigin();
		tCache.m_vConditionText.emplace_back(ALIGN_BOTTOM, std::format("[{:.0f}M]", vDelta.Length2D() / 41), tCache.m_tColor, pGroup->m_tOutlineColor);
	}

	if (pGroup->m_iESP & ESPEnum::Name)
		tCache.m_vText.emplace_back(ALIGN_TOP, GetProjectileName(pProjectile), tCache.m_tColor, pGroup->m_tOutlineColor);

	if (pGroup->m_iESP & ESPEnum::Owner && pOwner)
	{
		if (auto pResource = H::Entities.GetResource(); pResource)
			tCache.m_vText.emplace_back(ALIGN_TOP, F::PlayerUtils.GetPlayerName(iIndex, pResource->GetName(iIndex)), tCache.m_tColor, pGroup->m_tOutlineColor);
	}

	if (pGroup->m_iESP & ESPEnum::Flags)
	{
		switch (pProjectile->GetClassID())
		{
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
			if (pProjectile->As<CTFWeaponBaseGrenadeProj>()->m_bCritical())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "CRIT", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pProjectile->As<CTFWeaponBaseGrenadeProj>()->m_iDeflected() && (pProjectile->GetClassID() != ETFClassID::CTFGrenadePipebombProjectile || !pProjectile->GetAbsVelocity().IsZero()))
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "REFLECTED", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			break;
		case ETFClassID::CTFProjectile_Arrow:
		case ETFClassID::CTFProjectile_GrapplingHook:
		case ETFClassID::CTFProjectile_HealingBolt:
			if (pProjectile->As<CTFProjectile_Arrow>()->m_bCritical())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "CRIT", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pProjectile->As<CTFBaseRocket>()->m_iDeflected())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "REFLECTED", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pProjectile->As<CTFProjectile_Arrow>()->m_bArrowAlight())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "FIRE", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			break;
		case ETFClassID::CTFProjectile_Rocket:
		case ETFClassID::CTFProjectile_BallOfFire:
		case ETFClassID::CTFProjectile_MechanicalArmOrb:
		case ETFClassID::CTFProjectile_SentryRocket:
		case ETFClassID::CTFProjectile_SpellFireball:
		case ETFClassID::CTFProjectile_SpellLightningOrb:
		case ETFClassID::CTFProjectile_SpellKartOrb:
			if (pProjectile->As<CTFProjectile_Rocket>()->m_bCritical())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "CRIT", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pProjectile->As<CTFBaseRocket>()->m_iDeflected())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "REFLECTED", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			break;
		case ETFClassID::CTFProjectile_EnergyBall:
			if (pProjectile->As<CTFProjectile_EnergyBall>()->m_bChargedShot())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "CRIT", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pProjectile->As<CTFBaseRocket>()->m_iDeflected())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "REFLECTED", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			break;
		case ETFClassID::CTFProjectile_Flare:
			if (pProjectile->As<CTFProjectile_Flare>()->m_bCritical())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "CRIT", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			if (pProjectile->As<CTFBaseRocket>()->m_iDeflected())
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "REFLECTED", Vars::Colors::IndicatorTextBad.Value, pGroup->m_tOutlineColor);
			break;
		}
	}
}

static inline void StoreObjective(CBaseEntity* pObjective, CTFPlayer* pLocal, Group_t* pGroup, std::unordered_map<CBaseEntity*, EntityCache_t>& mCache)
{
	auto pOwner = pObjective->m_hOwnerEntity()->As<CTFPlayer>();
	if (pOwner == pLocal)
		return;

	EntityCache_t& tCache = mCache[pObjective];
	tCache.m_flAlpha = pGroup->m_tColor.a / 255.f;
	tCache.m_tColor = pGroup->m_bESPUseGroupColor ? F::Groups.GetColor(pObjective, pGroup).Alpha(255) : Color_t(255, 255, 255, 255);
	tCache.m_bBox = pGroup->m_iESP & ESPEnum::Box;

	if (pGroup->m_iESP & ESPEnum::Distance)
	{
		Vec3 vDelta = pObjective->m_vecOrigin() - pLocal->m_vecOrigin();
		tCache.m_vConditionText.emplace_back(ALIGN_BOTTOM, std::format("[{:.0f}M]", vDelta.Length2D() / 41), tCache.m_tColor, pGroup->m_tOutlineColor);
	}

	switch (pObjective->GetClassID())
	{
	case ETFClassID::CCaptureFlag:
	{
		auto pIntel = pObjective->As<CCaptureFlag>();

		if (pGroup->m_iESP & ESPEnum::Name)
			tCache.m_vText.emplace_back(ALIGN_TOP, "Intel", tCache.m_tColor, pGroup->m_tOutlineColor);

		if (pGroup->m_iESP & ESPEnum::Flags)
		{
			switch (pIntel->m_nFlagStatus())
			{
			case TF_FLAGINFO_HOME:
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "HOME", tCache.m_tColor, pGroup->m_tOutlineColor);
				break;
			case TF_FLAGINFO_DROPPED:
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "DROPPED", tCache.m_tColor, pGroup->m_tOutlineColor);
				break;
			default:
				tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, "STOLEN", tCache.m_tColor, pGroup->m_tOutlineColor);
			}
		}

		if (pGroup->m_iESP & ESPEnum::IntelReturnTime && pIntel->m_nFlagStatus() == TF_FLAGINFO_DROPPED)
		{
			float flReturnTime = std::max(pIntel->m_flResetTime() - TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick), 0.f);
			tCache.m_vConditionText.emplace_back(ALIGN_TOPRIGHT, std::format("RETURN {:.1f}S", pIntel->m_flResetTime() - TICKS_TO_TIME(I::ClientState->m_ClockDriftMgr.m_nServerTick)).c_str(), tCache.m_tColor, pGroup->m_tOutlineColor);
		}

		break;
	}
	}
}

static inline void StoreMisc(CBaseEntity* pEntity, CTFPlayer* pLocal, Group_t* pGroup, std::unordered_map<CBaseEntity*, EntityCache_t>& mCache)
{
	EntityCache_t& tCache = mCache[pEntity];
	tCache.m_flAlpha = pGroup->m_tColor.a / 255.f;
	tCache.m_tColor = pGroup->m_bESPUseGroupColor ? F::Groups.GetColor(pEntity, pGroup).Alpha(255) : Color_t(255, 255, 255, 255);
	tCache.m_bBox = pGroup->m_iESP & ESPEnum::Box;

	if (pGroup->m_iESP & ESPEnum::Distance)
	{
		Vec3 vDelta = pEntity->m_vecOrigin() - pLocal->m_vecOrigin();
		tCache.m_vConditionText.emplace_back(ALIGN_BOTTOM, std::format("[{:.0f}M]", vDelta.Length2D() / 41), tCache.m_tColor, pGroup->m_tOutlineColor);
	}

	if (pGroup->m_iESP & ESPEnum::Name)
	{
		const char* sName = "Unknown";
		switch (pEntity->GetClassID())
		{
		case ETFClassID::CTFBaseBoss: sName = "Boss"; break;
		case ETFClassID::CTFTankBoss: sName = "Tank"; break;
		case ETFClassID::CMerasmus: sName = "Merasmus"; break;
		case ETFClassID::CEyeballBoss: sName = "Monoculus"; break;
		case ETFClassID::CHeadlessHatman: sName = "Horseless Headless Horsemann"; break;
		case ETFClassID::CZombie: sName = "Skeleton"; break;
		case ETFClassID::CBaseAnimating:
		{
			auto uHash = H::Entities.GetModel(pEntity->entindex());
			if (H::Entities.IsHealth(uHash))
				sName = "Health";
			else if (H::Entities.IsAmmo(uHash))
				sName = "Ammo";
			else if (H::Entities.IsSpellbook(uHash))
				sName = "Spellbook";
			else if (H::Entities.IsPowerup(uHash))
			{
				sName = "Powerup";
				switch (uHash)
				{
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_agility.mdl"): sName = "Agility"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_crit.mdl"): sName = "Revenge"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_defense.mdl"): sName = "Resistance"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_haste.mdl"): sName = "Haste"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_king.mdl"): sName = "King"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_knockout.mdl"): sName = "Knockout"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_plague.mdl"): sName = "Plague"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_precision.mdl"): sName = "Precision"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_reflect.mdl"): sName = "Reflect"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_regen.mdl"): sName = "Regeneration"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_strength.mdl"): sName = "Strength"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_supernova.mdl"): sName = "Supernova"; break;
				case FNV1A::Hash32Const("models/pickups/pickup_powerup_vampire.mdl"): sName = "Vampire";
				}
			}
			break;
		}
		case ETFClassID::CTFAmmoPack: sName = "Ammo"; break;
		case ETFClassID::CCurrencyPack: sName = "Money"; break;
		case ETFClassID::CTFPumpkinBomb:
		case ETFClassID::CTFGenericBomb: sName = "Bomb"; break;
		case ETFClassID::CHalloweenGiftPickup: sName = "Gargoyle"; break;
		}

		tCache.m_vText.emplace_back(ALIGN_TOP, sName, pGroup->m_tColor, pGroup->m_tOutlineColor);
	}
}

void CESP::Store(CTFPlayer* pLocal)
{
	m_mPlayerCache.clear();
	m_mBuildingCache.clear();
	m_mEntityCache.clear();
	if (!pLocal || !F::Groups.GroupsActive())
		return;

	for (auto& [pEntity, pGroup] : F::Groups.GetGroup(false))
	{
		if (!pGroup->m_iESP)
			continue;

		if (pEntity->IsPlayer())
			StorePlayer(pEntity->As<CTFPlayer>(), pLocal, pGroup, m_mPlayerCache);
		else if (pEntity->IsBuilding())
			StoreBuilding(pEntity->As<CBaseObject>(), pLocal, pGroup, m_mBuildingCache);
		else if (pEntity->IsProjectile())
			StoreProjectile(pEntity, pLocal, pGroup, m_mEntityCache);
		else if (pEntity->GetClassID() == ETFClassID::CCaptureFlag)
			StoreObjective(pEntity, pLocal, pGroup, m_mEntityCache);
		else
			StoreMisc(pEntity, pLocal, pGroup, m_mEntityCache);
	}
}

void CESP::Draw()
{
	DrawWorld();
	DrawBuildings();
	DrawPlayers();
}

void CESP::DrawPlayers()
{
	if (m_mPlayerCache.empty())
		return;

	const auto& fFont = H::Fonts.GetFont(FONT_ESP);
	const auto& fSmallFont = H::Fonts.GetFont(FONT_ESP_SMALL);

	const int nTall = fFont.m_nTall + H::Draw.Scale(2);
	const int nSmallTall = fSmallFont.m_nTall + H::Draw.Scale(2);
	for (auto& [pEntity, tCache] : m_mPlayerCache)
	{
		float x, y, w, h;
		if (!GetDrawBounds(pEntity, x, y, w, h))
			continue;

		int l = x - H::Draw.Scale(6), r = x + w + H::Draw.Scale(6), m = x + w / 2;
		int t = y - H::Draw.Scale(5), b = y + h + H::Draw.Scale(5);
		int lOffset = 0, rOffset = 0, bOffset = 0, tOffset = 0;
		I::MatSystemSurface->DrawSetAlphaMultiplier(tCache.m_flAlpha);

		if (tCache.m_bBox)
			H::Draw.LineRectOutline(x, y, w, h, tCache.m_tColor, { 0, 0, 0, 255 });

		if (tCache.m_bBones)
		{
			auto pPlayer = pEntity->As<CTFPlayer>();
			matrix3x4 aBones[MAXSTUDIOBONES];
			if (pPlayer->SetupBones(aBones, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, I::GlobalVars->curtime))
			{
				int iHead = pPlayer->GetBaseToHitbox(HITBOX_HEAD);
				int iSpine2 = pPlayer->GetBaseToHitbox(HITBOX_SPINE2);
				int iPelvis = pPlayer->GetBaseToHitbox(HITBOX_PELVIS);
				int iLeftUpperarm = pPlayer->GetBaseToHitbox(HITBOX_LEFT_UPPERARM);
				int iLeftForearm = pPlayer->GetBaseToHitbox(HITBOX_LEFT_FOREARM);
				int iLeftHand = pPlayer->GetBaseToHitbox(HITBOX_LEFT_HAND);
				int iRightUpperarm = pPlayer->GetBaseToHitbox(HITBOX_RIGHT_UPPERARM);
				int iRightForearm = pPlayer->GetBaseToHitbox(HITBOX_RIGHT_FOREARM);
				int iRightHand = pPlayer->GetBaseToHitbox(HITBOX_RIGHT_HAND);
				int iLeftThigh = pPlayer->GetBaseToHitbox(HITBOX_LEFT_THIGH);
				int iLeftCalf = pPlayer->GetBaseToHitbox(HITBOX_LEFT_CALF);
				int iLeftFoot = pPlayer->GetBaseToHitbox(HITBOX_LEFT_FOOT);
				int iRightThigh = pPlayer->GetBaseToHitbox(HITBOX_RIGHT_THIGH);
				int iRightCalf = pPlayer->GetBaseToHitbox(HITBOX_RIGHT_CALF);
				int iRightFoot = pPlayer->GetBaseToHitbox(HITBOX_RIGHT_FOOT);

				DrawBones(pPlayer, aBones, { iHead, iSpine2, iPelvis }, tCache.m_tColor);
				DrawBones(pPlayer, aBones, { iSpine2, iLeftUpperarm, iLeftForearm, iLeftHand }, tCache.m_tColor);
				DrawBones(pPlayer, aBones, { iSpine2, iRightUpperarm, iRightForearm, iRightHand }, tCache.m_tColor);
				DrawBones(pPlayer, aBones, { iPelvis, iLeftThigh, iLeftCalf, iLeftFoot }, tCache.m_tColor);
				DrawBones(pPlayer, aBones, { iPelvis, iRightThigh, iRightCalf, iRightFoot }, tCache.m_tColor);
			}
		}

		for (auto& [iMode, flPercent, tColor, tOverfill, bAdjust] : tCache.m_vBars)
		{
			auto drawBar = [&](int x, int y, int w, int h, EAlign eAlign = ALIGN_LEFT)
				{
					if (flPercent > 1.f)
					{
						H::Draw.FillRectPercent(x, y, w, h, 1.f, tColor, { 0, 0, 0, 255 }, eAlign, bAdjust);
						H::Draw.FillRectPercent(x, y, w, h, flPercent - 1.f, tOverfill, { 0, 0, 0, 0 }, eAlign, bAdjust);
					}
					else
						H::Draw.FillRectPercent(x, y, w, h, flPercent, tColor, { 0, 0, 0, 255 }, eAlign, bAdjust);
				};

			int iSpace = H::Draw.Scale(4);
			int iThickness = H::Draw.Scale(2, Scale_Round);
			switch (iMode)
			{
			case ALIGN_LEFT:
				drawBar(x - iSpace - iThickness - lOffset, y, iThickness, h, ALIGN_BOTTOM);
				lOffset += iSpace + iThickness;
				break;
			case ALIGN_BOTTOM:
				drawBar(x, y + h + iSpace + bOffset, w, iThickness);
				bOffset += iSpace + iThickness;
				break;
			}
		}

		for (auto& [iMode, sText, tColor, tOutline] : tCache.m_vText)
		{
			switch (iMode)
			{
			case ALIGN_TOP:
				H::Draw.StringOutlined(fFont, m, t - tOffset, tColor, tOutline, ALIGN_BOTTOM, sText.c_str());
				tOffset += nTall;
				break;
			case ALIGN_BOTTOM:
				H::Draw.StringOutlined(fFont, m, b + bOffset, tColor, tOutline, ALIGN_TOP, sText.c_str());
				bOffset += nTall;
				break;
			case ALIGN_LEFT:
				H::Draw.StringOutlined(fFont, l - lOffset, y - H::Draw.Scale(2) + h - h * std::min(tCache.m_flHealth, 1.f), tColor, tOutline, ALIGN_TOPRIGHT, sText.c_str());
				break;
			case ALIGN_TOPRIGHT:
				H::Draw.StringOutlined(fFont, r, y - H::Draw.Scale(2) + rOffset, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				rOffset += nTall;
				break;
			case ALIGN_BOTTOMRIGHT:
				H::Draw.StringOutlined(fFont, r, y + h, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				break;
			}
		}

		for (auto& [iMode, sText, tColor, tOutline] : tCache.m_vConditionText)
		{
			switch (iMode)
			{
			case ALIGN_TOP:
				H::Draw.StringOutlined(fSmallFont, m, t - tOffset, tColor, tOutline, ALIGN_BOTTOM, sText.c_str());
				tOffset += nSmallTall;
				break;
			case ALIGN_BOTTOM:
				H::Draw.StringOutlined(fSmallFont, m, b + bOffset, tColor, tOutline, ALIGN_TOP, sText.c_str());
				bOffset += nSmallTall;
				break;
			case ALIGN_LEFT:
				H::Draw.StringOutlined(fSmallFont, l - lOffset, y - H::Draw.Scale(2) + h - h * std::min(tCache.m_flHealth, 1.f), tColor, tOutline, ALIGN_TOPRIGHT, sText.c_str());
				break;
			case ALIGN_TOPRIGHT:
				H::Draw.StringOutlined(fSmallFont, r, y - H::Draw.Scale(2) + rOffset, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				rOffset += nSmallTall;
				break;
			case ALIGN_BOTTOMRIGHT:
				H::Draw.StringOutlined(fSmallFont, r, y + h, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				break;
			case ALIGN_BOTTOMLEFT: // special case for health text since its used nowhere else
				int wide, tall = 0;
				I::MatSystemSurface->GetTextSize(fSmallFont.m_dwFont, SDK::ConvertUtf8ToWide(sText).c_str(), wide, tall);
				H::Draw.StringOutlined(fSmallFont, x - H::Draw.Scale(4) - H::Draw.Scale(2, Scale_Round) - lOffset - (wide / 2.f), y - H::Draw.Scale(4) + h - h * std::min(tCache.m_flHealth, 1.f), tColor, tOutline, ALIGN_TOPRIGHT, sText.c_str());
				break;
			}
		}

		if (tCache.m_iClassIcon)
		{
			int size = H::Draw.Scale(18, Scale_Round);
			H::Draw.Texture(m, t - tOffset, size, size, tCache.m_iClassIcon - 1, ALIGN_BOTTOM);
		}

		if (tCache.m_pWeaponIcon)
		{
			float flW = tCache.m_pWeaponIcon->Width(), flH = tCache.m_pWeaponIcon->Height();
			float flScale = H::Draw.Scale(std::min((w + 40) / 2.f, 80.f) / std::max(flW, flH * 2));
			H::Draw.DrawHudTexture(m - flW / 2.f * flScale, b + bOffset, flScale, tCache.m_pWeaponIcon, { 255, 255, 255, tCache.m_tColor.a });
		}
	}

	I::MatSystemSurface->DrawSetAlphaMultiplier(1.f);
}

void CESP::DrawBuildings()
{
	if (m_mBuildingCache.empty())
		return;

	const auto& fFont = H::Fonts.GetFont(FONT_ESP);
	const auto& fSmallFont = H::Fonts.GetFont(FONT_ESP_SMALL);

	const int nTall = fFont.m_nTall + H::Draw.Scale(2);
	const int nSmallTall = fSmallFont.m_nTall + H::Draw.Scale(2);
	for (auto& [pEntity, tCache] : m_mBuildingCache)
	{
		float x, y, w, h;
		if (!GetDrawBounds(pEntity, x, y, w, h))
			continue;

		int l = x - H::Draw.Scale(6), r = x + w + H::Draw.Scale(6), m = x + w / 2;
		int t = y - H::Draw.Scale(5), b = y + h + H::Draw.Scale(5);
		int lOffset = 0, rOffset = 0, bOffset = 0, tOffset = 0;
		I::MatSystemSurface->DrawSetAlphaMultiplier(tCache.m_flAlpha);

		if (tCache.m_bBox)
			H::Draw.LineRectOutline(x, y, w, h, tCache.m_tColor, { 0, 0, 0, 255 });
		for (auto& [iMode, flPercent, tColor, tOverfill, bAdjust] : tCache.m_vBars)
		{
			auto drawBar = [&](int x, int y, int w, int h, EAlign eAlign = ALIGN_LEFT)
				{
					if (flPercent > 1.f)
					{
						H::Draw.FillRectPercent(x, y, w, h, 1.f, tColor, { 0, 0, 0, 255 }, eAlign, bAdjust);
						H::Draw.FillRectPercent(x, y, w, h, flPercent - 1.f, tOverfill, { 0, 0, 0, 0 }, eAlign, bAdjust);
					}
					else
						H::Draw.FillRectPercent(x, y, w, h, flPercent, tColor, { 0, 0, 0, 255 }, eAlign, bAdjust);
				};

			int iSpace = H::Draw.Scale(4);
			int iThickness = H::Draw.Scale(2, Scale_Round);
			switch (iMode)
			{
			case ALIGN_LEFT:
				drawBar(x - iSpace - iThickness - lOffset, y, iThickness, h, ALIGN_BOTTOM);
				lOffset += iSpace + iThickness;
				break;
			case ALIGN_BOTTOM:
				drawBar(x, y + h + iSpace + bOffset, w, iThickness);
				bOffset += iSpace + iThickness;
				break;
			}
		}

		for (auto& [iMode, sText, tColor, tOutline] : tCache.m_vText)
		{
			switch (iMode)
			{
			case ALIGN_TOP:
				H::Draw.StringOutlined(fFont, m, t - tOffset, tColor, tOutline, ALIGN_BOTTOM, sText.c_str());
				tOffset += nTall;
				break;
			case ALIGN_BOTTOM:
				H::Draw.StringOutlined(fFont, m, b + bOffset, tColor, tOutline, ALIGN_TOP, sText.c_str());
				bOffset += nTall;
				break;
			case ALIGN_LEFT:
				H::Draw.StringOutlined(fFont, l - lOffset, y - H::Draw.Scale(2) + h - h * std::min(tCache.m_flHealth, 1.f), tColor, tOutline, ALIGN_TOPRIGHT, sText.c_str());
				break;
			case ALIGN_TOPRIGHT:
				H::Draw.StringOutlined(fFont, r, y - H::Draw.Scale(2) + rOffset, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				rOffset += nTall;
				break;
			case ALIGN_BOTTOMRIGHT:
				H::Draw.StringOutlined(fFont, r, y + h, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				break;
			}
		}

		for (auto& [iMode, sText, tColor, tOutline] : tCache.m_vConditionText)
		{
			switch (iMode)
			{
			case ALIGN_TOP:
				H::Draw.StringOutlined(fSmallFont, m, t - tOffset, tColor, tOutline, ALIGN_BOTTOM, sText.c_str());
				tOffset += nSmallTall;
				break;
			case ALIGN_BOTTOM:
				H::Draw.StringOutlined(fSmallFont, m, b + bOffset, tColor, tOutline, ALIGN_TOP, sText.c_str());
				bOffset += nSmallTall;
				break;
			case ALIGN_LEFT:
				H::Draw.StringOutlined(fSmallFont, l - lOffset, y - H::Draw.Scale(2) + h - h * std::min(tCache.m_flHealth, 1.f), tColor, tOutline, ALIGN_TOPRIGHT, sText.c_str());
				break;
			case ALIGN_TOPRIGHT:
				H::Draw.StringOutlined(fSmallFont, r, y - H::Draw.Scale(2) + rOffset, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				rOffset += nSmallTall;
				break;
			case ALIGN_BOTTOMRIGHT:
				H::Draw.StringOutlined(fSmallFont, r, y + h, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				break;
			case ALIGN_BOTTOMLEFT: // special case for health text since its used nowhere else
				H::Draw.StringOutlined(fSmallFont, l + H::Draw.Scale(5, Scale_Round), y - H::Draw.Scale(4) + h - h * std::min(tCache.m_flHealth, 1.f), tColor, tOutline, ALIGN_TOPRIGHT, sText.c_str());
				break;
			}
		}
	}

	I::MatSystemSurface->DrawSetAlphaMultiplier(1.f);
}

void CESP::DrawWorld()
{
	if (m_mEntityCache.empty())
		return;

	const auto& fFont = H::Fonts.GetFont(FONT_ESP);
	const auto& fSmallFont = H::Fonts.GetFont(FONT_ESP_SMALL);

	const int nTall = fFont.m_nTall + H::Draw.Scale(2);
	const int nSmallTall = fSmallFont.m_nTall + H::Draw.Scale(2);
	for (auto& [pEntity, tCache] : m_mEntityCache)
	{
		float x, y, w, h;
		if (!GetDrawBounds(pEntity, x, y, w, h))
			continue;

		int l = x - H::Draw.Scale(6), r = x + w + H::Draw.Scale(6), m = x + w / 2;
		int t = y - H::Draw.Scale(5), b = y + h + H::Draw.Scale(5);
		int lOffset = 0, rOffset = 0, bOffset = 0, tOffset = 0;
		I::MatSystemSurface->DrawSetAlphaMultiplier(tCache.m_flAlpha);

		if (tCache.m_bBox)
			H::Draw.LineRectOutline(x, y, w, h, tCache.m_tColor, { 0, 0, 0, 255 });


		for (auto& [iMode, sText, tColor, tOutline] : tCache.m_vText)
		{
			switch (iMode)
			{
			case ALIGN_TOP:
				H::Draw.StringOutlined(fFont, m, t - tOffset, tColor, tOutline, ALIGN_BOTTOM, sText.c_str());
				tOffset += nTall;
				break;
			case ALIGN_BOTTOM:
				H::Draw.StringOutlined(fFont, m, b + bOffset, tColor, tOutline, ALIGN_TOP, sText.c_str());
				bOffset += nTall;
				break;
			case ALIGN_TOPRIGHT:
				H::Draw.StringOutlined(fFont, r, y - H::Draw.Scale(2) + rOffset, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				rOffset += nTall;
				break;
			}
		}
		
		for (auto& [iMode, sText, tColor, tOutline] : tCache.m_vConditionText)
		{
			switch (iMode)
			{
			case ALIGN_TOP:
				H::Draw.StringOutlined(fSmallFont, m, t - tOffset, tColor, tOutline, ALIGN_BOTTOM, sText.c_str());
				tOffset += nSmallTall;
				break;
			case ALIGN_BOTTOM:
				H::Draw.StringOutlined(fSmallFont, m, b + bOffset, tColor, tOutline, ALIGN_TOP, sText.c_str());
				bOffset += nSmallTall;
				break;
			case ALIGN_TOPRIGHT:
				H::Draw.StringOutlined(fSmallFont, r, y - H::Draw.Scale(2) + rOffset, tColor, tOutline, ALIGN_TOPLEFT, sText.c_str());
				rOffset += nSmallTall;
				break;
			}
		}
	}

	I::MatSystemSurface->DrawSetAlphaMultiplier(1.f);
}

bool CESP::GetDrawBounds(CBaseEntity* pEntity, float& x, float& y, float& w, float& h)
{
	Vec3 vOrigin = pEntity->GetAbsOrigin();
	matrix3x4 mTransform = { { 1, 0, 0, vOrigin.x }, { 0, 1, 0, vOrigin.y }, { 0, 0, 1, vOrigin.z } };
	//if (pEntity->entindex() == I::EngineClient->GetLocalPlayer())
	Math::AngleMatrix({ 0.f, I::EngineClient->GetViewAngles().y, 0.f }, mTransform, false);

	float flLeft, flRight, flTop, flBottom;
	if (!SDK::IsOnScreen(pEntity, mTransform, &flLeft, &flRight, &flTop, &flBottom, true))
		return false;

	x = flLeft;
	y = flBottom;
	w = flRight - flLeft;
	h = flTop - flBottom;

	switch (pEntity->GetClassID())
	{
	case ETFClassID::CTFPlayer:
	case ETFClassID::CObjectSentrygun:
	case ETFClassID::CObjectDispenser:
	case ETFClassID::CObjectTeleporter:
		x += w * 0.125f;
		w *= 0.75f;
	}

	return !(x > H::Draw.m_nScreenW || x + w < 0 || y > H::Draw.m_nScreenH || y + h < 0);
}

void CESP::DrawBones(CTFPlayer* pPlayer, matrix3x4* aBones, std::vector<int> vecBones, Color_t clr)
{
	for (size_t n = 1; n < vecBones.size(); n++)
	{
		auto vBone1 = pPlayer->GetHitboxCenter(aBones, vecBones[n]);
		auto vBone2 = pPlayer->GetHitboxCenter(aBones, vecBones[n - 1]);

		Vec3 vScreen1, vScreen2;
		if (SDK::W2S(vBone1, vScreen1) && SDK::W2S(vBone2, vScreen2))
			H::Draw.Line(vScreen1.x, vScreen1.y, vScreen2.x, vScreen2.y, clr);
	}
}

static std::vector<Vec3> CreateCircle(Vec3 vOrigin, float flRadius)
{
	if (!flRadius)
		return {};

	Vec3 vAngles = Math::VectorAngles({ 0, 0, 1 });
	Vec3 vRight, vUp; Math::AngleVectors(vAngles, nullptr, &vRight, &vUp);

	std::vector<Vec3> vPoints = {};
	for (float i = 0.f; i < 100; i++)
	{
		Vec3 vPoint = vOrigin + (vRight * cos(2 * PI * i / 100) + vUp * sin(2 * PI * i / 100)) * flRadius;
		vPoints.push_back(vPoint);
	}
	vPoints.push_back(vPoints.front());

	return vPoints;
}

void CESP::DrawSoundESP()
{
	if (m_mSoundCircles.empty() || !F::Groups.GroupsActive())
		return;

	for (int i = 1; i < I::EngineClient->GetMaxClients(); i++)
	{	
		if (!m_mSoundCircles.contains(i))
			continue;

		float& flRadius = m_mSoundCircles[i];
		if (flRadius >= 100.f)
		{
			m_mSoundCircles.erase(i); 
			continue;
		}
			
		const auto pEntity = I::ClientEntityList->GetClientEntity(i)->As<CTFPlayer>();
		if (!pEntity || !pEntity->IsAlive() || pEntity->IsAGhost())
			continue;

		Vec3 vOrigin = pEntity->GetAbsOrigin();
		if (vOrigin.IsZero()) // check if the origin is at 0, 0, 0 which can sometimes happen with source engine sound system
		{
			m_mSoundCircles.erase(i); 
			continue;
		}

		Group_t* tGroup{};
		if (!(F::Groups.GetGroup(pEntity, tGroup, false) && tGroup->m_iESP & ESPEnum::Sound))
			continue;

		Color_t tColor = tGroup->m_bESPUseGroupColor ? F::Groups.GetColor(pEntity, tGroup) : Color_t(255, 255, 255, 255);
		tColor.a = Math::RemapVal(flRadius, 100.f, 75.f, 0.f, 1.f) * 255.f;

		static std::unordered_map<int, Vec3> mOrigin = {};
		if (!flRadius)
			mOrigin[i] = vOrigin; vOrigin = mOrigin[i];
		
		flRadius += tGroup->m_flCircleSpeed;

		auto vPoints = CreateCircle(vOrigin, flRadius);
		H::Draw.RenderPath(vPoints, tColor, false, Vars::Visuals::Simulation::StyleEnum::Line);
		H::Draw.RenderPath(vPoints, tColor, true, Vars::Visuals::Simulation::StyleEnum::Line);
		I::DebugOverlay->AddTextOverlayRGB(vOrigin, 0, 0.f, tColor.r, tColor.g, tColor.b, tColor.a, SDK::GetClassByIndex(pEntity->As<CTFPlayer>()->m_iClass()));
	}
}