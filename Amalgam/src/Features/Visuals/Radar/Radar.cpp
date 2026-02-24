#include "Radar.h"

#include "../Groups/Groups.h"
#include "../../Players/PlayerUtils.h"
#include "../../ImGui/Menu/Menu.h"

bool CRadar::GetDrawPosition(CTFPlayer* pLocal, CBaseEntity* pEntity, int& x, int& y, int& z)
{
	const float flRange = Vars::Radar::Main::Range.Value;
	const float flYaw = -DEG2RAD(I::EngineClient->GetViewAngles().y);
	const float flSin = sinf(flYaw), flCos = cosf(flYaw);

	Vec3 vDelta = pLocal->GetAbsOrigin() - pEntity->GetAbsOrigin();
	Vec2 vPos = { vDelta.x * flSin + vDelta.y * flCos, vDelta.x * flCos - vDelta.y * flSin };

	switch (Vars::Radar::Main::Style.Value)
	{
	case Vars::Radar::Main::StyleEnum::Circle:
	{
		const float flDist = vDelta.Length2D();
		if (flDist > flRange)
		{
			if (!Vars::Radar::Main::DrawOutOfRange.Value)
				return false;

			vPos *= flRange / flDist;
		}
		break;
	}
	case Vars::Radar::Main::StyleEnum::Rectangle:
		if (fabs(vPos.x) > flRange || fabs(vPos.y) > flRange)
		{
			if (!Vars::Radar::Main::DrawOutOfRange.Value)
				return false;

			Vec2 a = { -flRange / vPos.x, -flRange / vPos.y };
			Vec2 b = { flRange / vPos.x, flRange / vPos.y };
			Vec2 c = { std::min(a.x, b.x), std::min(a.y, b.y) };
			vPos *= fabsf(std::max(c.x, c.y));
		}
	}

	auto& tWindowBox = Vars::Radar::Main::Window.Value;
	x = tWindowBox.x + vPos.x / flRange * tWindowBox.w / 2;
	y = tWindowBox.y + vPos.y / flRange * tWindowBox.w / 2 + tWindowBox.h / 2.f;
	z = vDelta.z;

	return true;
}

void CRadar::DrawBackground()
{
	auto& tWindowBox = Vars::Radar::Main::Window.Value;
	Color_t& tThemeBack = Vars::Menu::Theme::Background.Value;
	Color_t& tThemeAccent = Vars::Menu::Theme::Accent.Value;
	Color_t tColorBackground = { tThemeBack.r, tThemeBack.g, tThemeBack.b, byte(Vars::Radar::Main::BackgroundAlpha.Value) };
	Color_t tColorAccent = { tThemeAccent.r, tThemeAccent.g, tThemeAccent.b, byte(Vars::Radar::Main::LineAlpha.Value) };

	switch (Vars::Radar::Main::Style.Value)
	{
	case Vars::Radar::Main::StyleEnum::Circle:
	{
		const float flRadius = tWindowBox.w / 2.f;
		H::Draw.FillCircle(tWindowBox.x, tWindowBox.y + flRadius, flRadius, 100, tColorBackground);
		H::Draw.LineCircle(tWindowBox.x, tWindowBox.y + flRadius, flRadius, 100, tColorAccent);
		break;
	}
	case Vars::Radar::Main::StyleEnum::Rectangle:
		H::Draw.FillRoundRect(tWindowBox.x - tWindowBox.w / 2, tWindowBox.y, tWindowBox.w, tWindowBox.h, H::Draw.Scale(3), tColorBackground);
		H::Draw.LineRoundRect(tWindowBox.x - tWindowBox.w / 2, tWindowBox.y, tWindowBox.w, tWindowBox.h, H::Draw.Scale(3), tColorAccent);
	}

	H::Draw.Line(tWindowBox.x - tWindowBox.w / 2, tWindowBox.y + tWindowBox.h / 2, tWindowBox.x + tWindowBox.w / 2 - 1, tWindowBox.y + tWindowBox.h / 2, tColorAccent);
	H::Draw.Line(tWindowBox.x, tWindowBox.y, tWindowBox.x, tWindowBox.y + tWindowBox.h - 1, tColorAccent);
}

static void DrawTexture(CTFPlayer* pLocal, Group_t* pGroup, CBaseEntity* pEntity, int x, int y, int z)
{
	if (pEntity->IsBuilding())
	{
		
	}
	else if (pEntity->IsPlayer())
	{
		
	}
}

void CRadar::DrawPoints(CTFPlayer* pLocal)
{
	for (auto& [pEntity, pGroup] : F::Groups.GetGroup())
	{
		int x, y, z;
		switch (pEntity->GetClassID())
		{
		case ETFClassID::CHalloweenGiftPickup:
			if (GetDrawPosition(pLocal, pEntity, x, y, z))
			{
				const int iSize = Vars::Radar::World::Size.Value;
				if (Vars::Radar::World::Background.Value)
				{
					const float flRadius = sqrtf(pow(iSize, 2) * 2) / 2;
					H::Draw.FillCircle(x, y, flRadius, 20, F::Groups.GetColor(pEntity, pGroup));
				}
				H::Draw.Texture(x, y, iSize, iSize, 39);
			}
			break;
		case ETFClassID::CTFPumpkinBomb:
		case ETFClassID::CTFGenericBomb:
			if (GetDrawPosition(pLocal, pEntity, x, y, z))
			{
				const int iSize = Vars::Radar::World::Size.Value;
				if (Vars::Radar::World::Background.Value)
				{
					const float flRadius = sqrtf(pow(iSize, 2) * 2) / 2;
					H::Draw.FillCircle(x, y, flRadius, 20, F::Groups.GetColor(pEntity, pGroup));
				}

				H::Draw.Texture(x, y, iSize, iSize, 36);
			}
			break;
		case ETFClassID::CCurrencyPack:
			if (GetDrawPosition(pLocal, pEntity, x, y, z))
			{
				const int iSize = Vars::Radar::World::Size.Value;
				if (Vars::Radar::World::Background.Value)
				{
					const float flRadius = sqrtf(pow(iSize, 2) * 2) / 2;
					H::Draw.FillCircle(x, y, flRadius, 20, F::Groups.GetColor(pEntity, pGroup));
				}

				H::Draw.Texture(x, y, iSize, iSize, 35);
			}
			break;
		case ETFClassID::CObjectSentrygun:
		case ETFClassID::CObjectDispenser:
		case ETFClassID::CObjectTeleporter:
			if (GetDrawPosition(pLocal, pEntity, x, y, z))
			{
				const int iSize = Vars::Radar::Building::Size.Value;
				if (Vars::Radar::Building::Background.Value)
				{
					const float flRadius = sqrtf(pow(iSize, 2) * 2) / 2;
					H::Draw.FillCircle(x, y, flRadius, 20, F::Groups.GetColor(pEntity, pGroup));
				}

				auto pBuilding = pEntity->As<CBaseObject>();

				if (!pBuilding->m_bWasMapPlaced())
				{
					auto pOwner = pBuilding->m_hBuilder().Get();
					if (pOwner)
					{
						const int nIndex = pOwner->entindex();

						const int iSize = Vars::Radar::Building::Size.Value;
						switch (pBuilding->GetClassID())
						{
						case ETFClassID::CObjectSentrygun:
							H::Draw.Texture(x, y, iSize, iSize, 26 + pBuilding->m_iUpgradeLevel());
							break;
						case ETFClassID::CObjectDispenser:
							H::Draw.Texture(x, y, iSize, iSize, 30);
							break;
						case ETFClassID::CObjectTeleporter:
							H::Draw.Texture(x, y, iSize, iSize, pBuilding->m_iObjectMode() ? 32 : 31);
							break;
						}

						int iBounds = iSize;
						if (Vars::Radar::Building::Background.Value)
						{
							const float flRadius = sqrtf(pow(iSize, 2) * 2) / 2;
							iBounds = flRadius * 2;
						}

						if (pGroup->m_iESP & ESPEnum::HealthBar)
						{
							const int iMaxHealth = pBuilding->m_iMaxHealth(), iHealth = pBuilding->m_iHealth();

							const auto pBuilding = pEntity->As<CBaseObject>();

							const float flHealth = pBuilding->m_iHealth(), flMaxHealth = pBuilding->m_iMaxHealth();
							const float health = std::clamp(flHealth / flMaxHealth, 0.f, 1.f);

							Color_t tColor;
							if (health < 0.5f)
								tColor = pGroup->m_tHealthColorLow.Lerp(pGroup->m_tHealthColorMid, health * 2.0f);
							else
								tColor = pGroup->m_tHealthColorMid.Lerp(pGroup->m_tHealthColorHigh, (health - 0.5f) * 2.0f);

							float flRatio = std::clamp(float(iHealth) / iMaxHealth, 0.f, 1.f);
							H::Draw.FillRectPercent(x - iBounds / 2, y - iBounds / 2, 2, iBounds, flRatio, tColor, { 0, 0, 0, 255 }, ALIGN_BOTTOM, true);
						}
					}
				}
			}
			break;
		case ETFClassID::CTFPlayer:
			auto pPlayer = pEntity->As<CTFPlayer>();
			if (pPlayer->IsDormant() && !H::Entities.GetDormancy(pPlayer->entindex()) || !pPlayer->IsAlive() || pPlayer->IsAGhost())
				break;

			if (GetDrawPosition(pLocal, pEntity, x, y, z))
			{
				const int iSize = Vars::Radar::Player::Size.Value;
				if (Vars::Radar::Player::Background.Value)
				{
					const float flRadius = sqrtf(pow(iSize, 2) * 2) / 2;
					H::Draw.FillCircle(x, y, flRadius, 20, F::Groups.GetColor(pEntity, pGroup));
				}

				const Color_t tColor = F::Groups.GetColor(pEntity, pGroup);

				int iBounds = iSize;
				if (Vars::Radar::Player::Background.Value)
				{
					const float flRadius = sqrtf(pow(iSize, 2) * 2) / 2;
					H::Draw.FillCircle(x, y, flRadius, 20, tColor);
					iBounds = flRadius * 2;
				}

				auto pPlayer = pEntity->As<CTFPlayer>();
				switch (Vars::Radar::Player::Icon.Value)
				{
				case Vars::Radar::Player::IconEnum::Avatars:
				{
					player_info_t pi{};
					if (I::EngineClient->GetPlayerInfo(pPlayer->entindex(), &pi) && !pi.fakeplayer)
					{
						int iType = 0; F::PlayerUtils.GetPlayerName(pPlayer->entindex(), nullptr, &iType);
						if (iType != 1)
						{
							H::Draw.Avatar(x, y, iSize, iSize, pi.friendsID);
							break;
						}
					}
					[[fallthrough]];
				}
				case Vars::Radar::Player::IconEnum::Portraits:
					if (int iTeam = pPlayer->IsInValidTeam())
					{
						H::Draw.Texture(x, y, iSize, iSize, pPlayer->m_iClass() + (iTeam == TF_TEAM_RED ? 9 : 18) - 1);
						break;
					}
					[[fallthrough]];
				case Vars::Radar::Player::IconEnum::Icons:
					H::Draw.Texture(x, y, iSize, iSize, pPlayer->m_iClass() - 1);
					break;
				}

				if (pGroup->m_iESP & ESPEnum::HealthBar)
				{

					const float flHealth = pPlayer->m_iHealth(), flMaxHealth = pPlayer->GetMaxHealth();
					const float health = std::clamp(flHealth / flMaxHealth, 0.f, 1.f);

					Color_t tColor;
					if (health > 1.0f)
						tColor = pGroup->m_tHealthColorHigh;
					else if (health < 0.5f)
						tColor = pGroup->m_tHealthColorLow.Lerp(pGroup->m_tHealthColorMid, health * 2.0f);
					else
						tColor = pGroup->m_tHealthColorMid.Lerp(pGroup->m_tHealthColorHigh, (health - 0.5f) * 2.0f);

					float flRatio = std::clamp(flHealth / flMaxHealth, 0.f, 1.f);
					H::Draw.FillRectPercent(x - iBounds / 2, y - iBounds / 2, 2, iBounds, flRatio, tColor, { 0, 0, 0, 255 }, ALIGN_BOTTOM, true);

					if (flHealth > flMaxHealth)
					{
						const float flMaxOverheal = floorf(flMaxHealth / 10.f) * 5;
						flRatio = std::clamp((flHealth - flMaxHealth) / flMaxOverheal, 0.f, 1.f);
						tColor = pGroup->m_tHealthColorOverheal;
						H::Draw.FillRectPercent(x - iBounds / 2, y - iBounds / 2, 2, iBounds, flRatio, tColor, { 0, 0, 0, 0 }, ALIGN_BOTTOM, true);
					}
				}

				if (Vars::Radar::Player::Height.Value && std::abs(z) > 80.f)
				{
					const int m = x - iSize / 2;
					const int iOffset = z < 0 ? -5 : 5;
					const int yPos = z < 0 ? y - iBounds / 2 - 2 : y + iBounds / 2 + 2;

					H::Draw.FillPolygon({ Vec2(m, yPos), Vec2(m + iSize * 0.5f, yPos + iOffset), Vec2(m + iSize, yPos) }, tColor);
				}
			}
			break;
		}

		uint32_t uHash = H::Entities.GetModel(pEntity->entindex());
		if (H::Entities.IsSpellbook(uHash))
		{
			if (GetDrawPosition(pLocal, pEntity, x, y, z))
			{
				const int iSize = Vars::Radar::World::Size.Value;
				if (Vars::Radar::World::Background.Value)
				{
					const float flRadius = sqrtf(pow(iSize, 2) * 2) / 2;
					H::Draw.FillCircle(x, y, flRadius, 20, F::Groups.GetColor(pEntity, pGroup));
				}

				H::Draw.Texture(x, y, iSize, iSize, 38);
			}
			break;
		}
		else if (H::Entities.IsPowerup(uHash))
		{
			if (GetDrawPosition(pLocal, pEntity, x, y, z))
			{
				const int iSize = Vars::Radar::World::Size.Value;
				if (Vars::Radar::World::Background.Value)
				{
					const float flRadius = sqrtf(pow(iSize, 2) * 2) / 2;
					H::Draw.FillCircle(x, y, flRadius, 20, F::Groups.GetColor(pEntity, pGroup));
				}

				H::Draw.Texture(x, y, iSize, iSize, 37);
			}
			break;
		}
		else if (H::Entities.IsAmmo(uHash) || pEntity->GetClassID() == ETFClassID::CTFAmmoPack)
		{
			if (GetDrawPosition(pLocal, pEntity, x, y, z))
			{
				const int iSize = Vars::Radar::World::Size.Value;
				if (Vars::Radar::World::Background.Value)
				{
					const float flRadius = sqrtf(pow(iSize, 2) * 2) / 2;
					H::Draw.FillCircle(x, y, flRadius, 20, F::Groups.GetColor(pEntity, pGroup));
				}

				H::Draw.Texture(x, y, iSize, iSize, 34);
			}
		}
		else if (H::Entities.IsHealth(uHash))
		{
			if (GetDrawPosition(pLocal, pEntity, x, y, z))
			{
				const int iSize = Vars::Radar::World::Size.Value;
				if (Vars::Radar::World::Background.Value)
				{
					const float flRadius = sqrtf(pow(iSize, 2) * 2) / 2;
					H::Draw.FillCircle(x, y, flRadius, 20, F::Groups.GetColor(pEntity, pGroup));
				}

				H::Draw.Texture(x, y, iSize, iSize, 33);
			}
		}
	}
}

void CRadar::Run(CTFPlayer* pLocal)
{
	if (!Vars::Radar::Main::Enabled.Value || I::MatSystemSurface->IsCursorVisible() && !I::EngineClient->IsPlayingDemo() && !F::Menu.m_bIsOpen)
		return;

	DrawBackground();
	DrawPoints(pLocal);
}