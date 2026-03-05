#pragma once
#include "../SDK/SDK.h"
#include "../SDK/Definitions/Misc/UtlObjectReference.h"
#include "../Features/Visuals/Groups/Groups.h"
#include "../Features/Visuals/FakeAngle/FakeAngle.h"
#include "../Features/Ticks/Ticks.h"

#include <functional>
#include <regex>
#include <sstream>

inline int ColorToInt(Color_t col)
{
	return col.r << 16 | col.g << 8 | col.b;
}

class AxisSet
{
public:
	float m_flOldAxisValue = 0.f;
	float m_flOldSimulationTime = 0.f;
	float m_flNewAxisValue = 0.f;
	float m_flNewSimulationTime = 0.f;

public:
	float Get(bool bZ = false) const
	{
		int iDeltaTicks = TIME_TO_TICKS(m_flNewSimulationTime - m_flOldSimulationTime);
		float flGravityCorrection = 0.f;
		if (bZ)
		{
			static auto sv_gravity = U::ConVars.FindVar("sv_gravity");
			float flDeltaTicks = float(iDeltaTicks) + 0.5f; // ?
			flGravityCorrection = (powf(flDeltaTicks, 2.f) - flDeltaTicks) / 2.f * sv_gravity->GetFloat() * powf(TICK_INTERVAL, 2);
		}
		float flDeltaValue = m_flNewAxisValue - m_flOldAxisValue;
		float flTickVelocity = flDeltaValue + (flDeltaValue ? PLAYER_ORIGIN_COMPRESSION / 2 * sign(m_flNewAxisValue) : 0.f) - flGravityCorrection;
		return flTickVelocity / TICKS_TO_TIME(iDeltaTicks);
	}
};

class AxisInfo
{
public:
	AxisSet x = {}, y = {}, z = {};

public:
	AxisSet& operator[](int i)
	{
		return ((AxisSet*)this)[i];
	}

	AxisSet operator[](int i) const
	{
		return ((AxisSet*)this)[i];
	}

	Vec3 Get(bool bGrounded = false) const
	{
		return { x.Get(), y.Get(), z.Get(!bGrounded) };
	}

	float Get(int i) const
	{
		return ((AxisSet*)this)[i].Get(i == 2);
	}
};

class CBaseHudChatInputLine
{
public:
	VIRTUAL_ARGS(InvalidateLayout, void, 66, (bool layoutNow, bool reloadScheme), this, layoutNow, reloadScheme);
};

struct animevent_t
{
	int				event;
	const char* options;
	float			cycle;
	float			eventtime;
	int				type;
	CBaseAnimating* pSource;
};

typedef enum
{
	AE_INVALID = -1,
	AE_EMPTY,
	AE_NPC_LEFTFOOT,
	AE_NPC_RIGHTFOOT,
	AE_NPC_BODYDROP_LIGHT,
	AE_NPC_BODYDROP_HEAVY,
	AE_NPC_SWISHSOUND,
	AE_NPC_180TURN,
	AE_NPC_ITEM_PICKUP,
	AE_NPC_WEAPON_DROP,
	AE_NPC_WEAPON_SET_SEQUENCE_NAME,
	AE_NPC_WEAPON_SET_SEQUENCE_NUMBER,
	AE_NPC_WEAPON_SET_ACTIVITY,
	AE_NPC_HOLSTER,
	AE_NPC_DRAW,
	AE_NPC_WEAPON_FIRE,
	AE_CL_PLAYSOUND,
	AE_SV_PLAYSOUND,
	AE_CL_STOPSOUND,
	AE_START_SCRIPTED_EFFECT,
	AE_STOP_SCRIPTED_EFFECT,
	AE_CLIENT_EFFECT_ATTACH,
	AE_MUZZLEFLASH,
	AE_NPC_MUZZLEFLASH,
	AE_THUMPER_THUMP,
	AE_AMMOCRATE_PICKUP_AMMO,
	AE_NPC_RAGDOLL,
	AE_NPC_ADDGESTURE,
	AE_NPC_RESTARTGESTURE,
	AE_NPC_ATTACK_BROADCAST,
	AE_NPC_HURT_INTERACTION_PARTNER,
	AE_NPC_SET_INTERACTION_CANTDIE,
	AE_SV_DUSTTRAIL,
	AE_CL_CREATE_PARTICLE_EFFECT,
	AE_RAGDOLL,
	AE_CL_ENABLE_BODYGROUP,
	AE_CL_DISABLE_BODYGROUP,
	AE_CL_BODYGROUP_SET_VALUE,
	AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN,
	AE_WPN_PRIMARYATTACK,
	AE_WPN_INCREMENTAMMO,
	AE_WPN_HIDE,
	AE_WPN_UNHIDE,
	AE_WPN_PLAYWPNSOUND,
	AE_RD_ROBOT_POP_PANELS_OFF,
	AE_TAUNT_ENABLE_MOVE,
	AE_TAUNT_DISABLE_MOVE,
	AE_CL_REMOVE_PARTICLE_EFFECT,
	LAST_SHARED_ANIMEVENT,
} Animevent;

enum cmd_source_t
{
	src_client,
	src_command
};

static std::string s_sCmdString;

#define PRE_STR "\x7\x7\x7\x7\x7\x7\x7"
static std::vector<std::pair<std::string, std::string>> s_vStatic = {
	{ "\\x1", "\x1" },
	{ "\\x01", "\x1" },
	{ "\\x2", PRE_STR"\x2" },
	{ "\\x02", PRE_STR"\x2" },
	{ "\\x3", PRE_STR"\x3" },
	{ "\\x03", PRE_STR"\x3" },
	{ "\\x4", PRE_STR"\x4" },
	{ "\\x04", PRE_STR"\x4" },
	{ "\\x5", PRE_STR"\x5" },
	{ "\\x05", PRE_STR"\x5" },
	{ "\\x6", PRE_STR"\x6" },
	{ "\\x06", PRE_STR"\x6" },
	{ "\\x7", PRE_STR"\x7" },
	{ "\\x07", PRE_STR"\x7" },
	{ "\\x8", PRE_STR"\x8" },
	{ "\\x08", PRE_STR"\x8" },

	{ "\\{default}", "\x1" },
	{ "\\{clear}", PRE_STR"\x8""00000000" },
	{ "\\{red}", PRE_STR"\x7""ff0000" },
	{ "\\{green}", PRE_STR"\x7""00ff00" },
	{ "\\{blue}", PRE_STR"\x7""0000ff" },
	{ "\\{yellow}", PRE_STR"\x7""ffff00" },
	{ "\\{pink}", PRE_STR"\x7""ff00ff" },
	{ "\\{cyan}", PRE_STR"\x7""00ffff" },
	{ "\\{orange}", PRE_STR"\x7""ff7000" },
	{ "\\{purple}", PRE_STR"\x7""7f00ff" },
	{ "\\{brown}", PRE_STR"\x7""583927" },
	{ "\\{gold}", PRE_STR"\x7""c8a900" },
	{ "\\{gray}", PRE_STR"\x7""cccccc" },
	{ "\\{black}", PRE_STR"\x7""000000" },
	{ "\\{bluteam}", PRE_STR"\x7""99ccff" },
	{ "\\{blueteam}", PRE_STR"\x7""99ccff" },
	{ "\\{redteam}", PRE_STR"\x7""ff4040" },
	{ "\\{normal}", PRE_STR"\x7""b2b2b2" },
	{ "\\{unique}", PRE_STR"\x7""ffd700" },
	{ "\\{strange}", PRE_STR"\x7""cf6a32" },
	{ "\\{vintage}", PRE_STR"\x7""476291" },
	{ "\\{haunted}", PRE_STR"\x7""38f3ab" },
	{ "\\{genuine}", PRE_STR"\x7""4d7455" },
	{ "\\{unusual}", PRE_STR"\x7""8650ac" },
	{ "\\{collectors}", PRE_STR"\x7""aa0000" },
	{ "\\{community}", PRE_STR"\x7""70b04a" },
	{ "\\{selfmade}", PRE_STR"\x7""70b04a" },
	{ "\\{valve}", PRE_STR"\x7""a50f79" },
	{ "\\{elite}", PRE_STR"\x7""eb4b4b" },
	{ "\\{assassin}", PRE_STR"\x7""d32ce6" },
	{ "\\{commando}", PRE_STR"\x7""8847ff" },
	{ "\\{mercenary}", PRE_STR"\x7""4b69ff" },

	{ "\\t", "\t" },
};
static std::vector<std::function<void()>> s_vDynamic = {
	[&]()
	{
		auto pResource = H::Entities.GetResource();
		if (!pResource)
			return;

		std::string sFind = "\\{self}";
		std::string sReplace = pResource->GetName(I::EngineClient->GetLocalPlayer());

		size_t iPos = 0;
		while (true)
		{
			auto iFind = s_sCmdString.find(sFind, iPos);
			if (iFind == std::string::npos)
				break;

			iPos = iFind + sReplace.length();
			s_sCmdString = s_sCmdString.replace(iFind, sFind.length(), sReplace);
		}
	},
	[&]()
	{
		auto pResource = H::Entities.GetResource();
		if (!pResource)
			return;

		std::string sFind = "\\{team}";
		std::string sReplace = PRE_STR"\x7""cccccc";
		switch (pResource->m_iTeam(I::EngineClient->GetLocalPlayer()))
		{
		case TF_TEAM_BLUE: sReplace = PRE_STR"\x7""99ccff"; break;
		case TF_TEAM_RED: sReplace = PRE_STR"\x7""ff4040"; break;
		}

		size_t iPos = 0;
		while (true)
		{
			auto iFind = s_sCmdString.find(sFind, iPos);
			if (iFind == std::string::npos)
				break;

			iPos = iFind + sReplace.length();
			s_sCmdString = s_sCmdString.replace(iFind, sFind.length(), sReplace);
		}
	},
	[&]()
	{
		auto sRegex = R"(\\\{rgb:(\d+)?(?:,(\d+))?(?:,(\d+))?\})";

		while (true)
		{
			std::smatch tMatch; std::regex_search(s_sCmdString, tMatch, std::regex(sRegex));
			if (tMatch.size() != 4)
				break;

			int r = !tMatch[1].str().empty() ? std::stoi(tMatch[1]) : 255;
			int g = !tMatch[2].str().empty() ? std::stoi(tMatch[2]) : 255;
			int b = !tMatch[3].str().empty() ? std::stoi(tMatch[3]) : 255;

			Color_t tColor; tColor.SetRGB(r, g, b);
			s_sCmdString = s_sCmdString.replace(tMatch.position(), tMatch.length(), std::format(PRE_STR"{}", tColor.ToHex()));
		}
	},
	[&]()
	{
		auto sRegex = R"(\\\{rgba:(\d+)?(?:,(\d+))?(?:,(\d+))?(?:,(\d+))?\})";

		while (true)
		{
			std::smatch tMatch; std::regex_search(s_sCmdString, tMatch, std::regex(sRegex));
			if (tMatch.size() != 5)
				break;

			int r = !tMatch[1].str().empty() ? std::stoi(tMatch[1]) : 255;
			int g = !tMatch[2].str().empty() ? std::stoi(tMatch[2]) : 255;
			int b = !tMatch[3].str().empty() ? std::stoi(tMatch[3]) : 255;
			int a = !tMatch[4].str().empty() ? std::stoi(tMatch[4]) : 255;

			Color_t tColor; tColor.SetRGB(r, g, b, a);
			s_sCmdString = s_sCmdString.replace(tMatch.position(), tMatch.length(), std::format(PRE_STR"{}", tColor.ToHexA()));
		}
	},
	[&]()
	{
		auto sRegex = R"(\\\{hsv:(\d+)?(?:,(\d+))?(?:,(\d+))?\})";

		while (true)
		{
			std::smatch tMatch; std::regex_search(s_sCmdString, tMatch, std::regex(sRegex));
			if (tMatch.size() != 4)
				break;

			int h = !tMatch[1].str().empty() ? std::stoi(tMatch[1]) : 0;
			int s = !tMatch[2].str().empty() ? std::stoi(tMatch[2]) : 100;
			int v = !tMatch[3].str().empty() ? std::stoi(tMatch[3]) : 100;

			Color_t tColor; tColor.SetHSV(h, s, v);
			s_sCmdString = s_sCmdString.replace(tMatch.position(), tMatch.length(), std::format(PRE_STR"{}", tColor.ToHex()));
		}
	},
	[&]()
	{
		auto sRegex = R"(\\\{hsva:(\d+)?(?:,(\d+))?(?:,(\d+))?(?:,(\d+))?\})";

		while (true)
		{
			std::smatch tMatch; std::regex_search(s_sCmdString, tMatch, std::regex(sRegex));
			if (tMatch.size() != 5)
				break;

			int h = !tMatch[1].str().empty() ? std::stoi(tMatch[1]) : 0;
			int s = !tMatch[2].str().empty() ? std::stoi(tMatch[2]) : 100;
			int v = !tMatch[3].str().empty() ? std::stoi(tMatch[3]) : 100;
			int a = !tMatch[4].str().empty() ? std::stoi(tMatch[4]) : 255;

			Color_t tColor; tColor.SetHSV(h, s, v, a);
			s_sCmdString = s_sCmdString.replace(tMatch.position(), tMatch.length(), std::format(PRE_STR"{}", tColor.ToHexA()));
		}
	},
	[&]()
	{
		auto sRegex = R"(\\\{(\d+):(.+)\})";

		while (true)
		{
			std::smatch tMatch; std::regex_search(s_sCmdString, tMatch, std::regex(sRegex));
			if (tMatch.size() != 3)
				break;

			int n = std::stoi(tMatch[1]);
			auto str = tMatch[2].str();

			std::stringstream ssStream;
			for (int i = 0; i < n; i++)
				ssStream << str;

			s_sCmdString = s_sCmdString.replace(tMatch.position(), tMatch.length(), ssStream.str());
		}
	},
};

inline Color_t GetHealthColor(CBaseEntity* pEntity, Group_t* pGroup, bool bAllowOverheal)
{
	if (pEntity->IsBuilding())
	{
		const auto pBuilding = pEntity->As<CBaseObject>();
		const float flHealth = pBuilding->m_iHealth();
		const float flMaxHealth = pBuilding->m_iMaxHealth();
		const float health = std::clamp(flHealth / flMaxHealth, 0.f, 1.f);

		if (health < 0.5f)
			return pGroup->m_tHealthColorLow.Lerp(pGroup->m_tHealthColorMid, health * 2.0f);

		return pGroup->m_tHealthColorMid.Lerp(pGroup->m_tHealthColorHigh, (health - 0.5f) * 2.0f);
	}
	else if (pEntity->IsPlayer())
	{
		const auto pPlayer = pEntity->As<CTFPlayer>();
		const float flHealth = pPlayer->m_iHealth();
		const float flMaxHealth = pPlayer->GetMaxHealth();
		const float health = bAllowOverheal
			? std::max(flHealth / flMaxHealth, 0.f)
			: std::clamp(flHealth / flMaxHealth, 0.f, 1.f);

		if (health > 1.0f)
			return pGroup->m_tHealthColorHigh;
		else if (health < 0.5f)
			return  pGroup->m_tHealthColorLow.Lerp(pGroup->m_tHealthColorMid, health * 2.0f);
		else
			return pGroup->m_tHealthColorMid.Lerp(pGroup->m_tHealthColorHigh, (health - 0.5f) * 2.0f);
	}

	return Color_t(255, 255, 255, 255);
}

#define PIX_VALVE_ORANGE 0xFFF5940F
class PIXEvent
{
public:
	PIXEvent(IMatRenderContext* pRenderContext, const char* szName, unsigned long color = PIX_VALVE_ORANGE)
		: m_pRenderContext(pRenderContext)
	{
		m_pRenderContext->BeginPIXEvent(color, szName);
	}
	~PIXEvent()
	{
		m_pRenderContext->EndPIXEvent();
	}
private:
	IMatRenderContext* m_pRenderContext;
};

static inline void DestroyGlowObject(std::unordered_map<int, CGlowObject*>::iterator& it)
{
	delete it->second;
	it->second = nullptr;
}

#define MATH_EPSILON (1.f / 16)
#define PSILENT_EPSILON (1.f - MATH_EPSILON)
#define REAL_EPSILON (0.1f + MATH_EPSILON)
#define SNAP_SIZE_EPSILON (10.f - MATH_EPSILON)
#define SNAP_NOISE_EPSILON (0.5f + MATH_EPSILON)


struct CmdHistory_t
{
	Vec3 m_vAngle;
	bool m_bAttack1;
	bool m_bAttack2;
	bool m_bSendingPacket;
};

static inline void UpdateInfo(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, CUserCmd* pCmd)
{
	G::PSilentAngles = G::SilentAngles = G::Attacking = G::Throwing = false;
	G::LastUserCmd = G::CurrentUserCmd ? G::CurrentUserCmd : pCmd;
	G::CurrentUserCmd = pCmd;
	G::OriginalCmd = *pCmd;

	if (!pWeapon)
		return;

	G::CanPrimaryAttack = G::CanSecondaryAttack = G::Reloading = false;

	if (pWeapon->GetMaxClip1() != WEAPON_NOCLIP && !pWeapon->m_bReloadsSingly())
	{	// dumb fix
		float flOldCurtime = I::GlobalVars->curtime;
		I::GlobalVars->curtime = TICKS_TO_TIME(pLocal->m_nTickBase());
		pWeapon->CheckReload();
		I::GlobalVars->curtime = flOldCurtime;
	}

	bool bCanAttack = pLocal->CanAttack();
	{
		static int iStaticItemDefinitionIndex = 0;
		int iOldItemDefinitionIndex = iStaticItemDefinitionIndex;
		int iNewItemDefinitionIndex = iStaticItemDefinitionIndex = pWeapon->m_iItemDefinitionIndex();

		if (iNewItemDefinitionIndex != iOldItemDefinitionIndex || !bCanAttack || !pWeapon->m_iClip1())
			F::Ticks.m_iWait = -1;
	}
	if (bCanAttack)
	{
		G::CanPrimaryAttack = pWeapon->CanPrimaryAttack();
		G::CanSecondaryAttack = pWeapon->CanSecondaryAttack();

		switch (pWeapon->GetWeaponID())
		{
		case TF_WEAPON_FLAME_BALL:
			if (G::CanPrimaryAttack)
			{
				// do this, otherwise it will be a tick behind
				float flFrametime = TICK_INTERVAL * 100;
				float flMeterMult = S::IHasGenericMeter_GetMeterMultiplier.Call<float>(pWeapon->m_pMeter());
				float flRate = SDK::AttribHookValue(1.f, "item_meter_charge_rate", pWeapon) - 1;
				float flMult = SDK::AttribHookValue(1.f, "mult_item_meter_charge_rate", pWeapon);
				float flTankPressure = pLocal->m_flTankPressure() + flFrametime * flMeterMult / (flRate * flMult);

				if (G::CanPrimaryAttack && flTankPressure < 100.f)
					G::CanPrimaryAttack = G::CanSecondaryAttack = false;
			}
			break;
		case TF_WEAPON_MINIGUN:
		{
			int iState = pWeapon->As<CTFMinigun>()->m_iWeaponState();
			if (iState != AC_STATE_FIRING && iState != AC_STATE_SPINNING || !pWeapon->HasPrimaryAmmoForShot())
				G::CanPrimaryAttack = false;
			break;
		}
		case TF_WEAPON_FLAREGUN_REVENGE:
			if (pCmd->buttons & IN_ATTACK2)
				G::CanPrimaryAttack = false;
			break;
		case TF_WEAPON_BAT_WOOD:
		case TF_WEAPON_BAT_GIFTWRAP:
			if (!pWeapon->HasPrimaryAmmoForShot())
				G::CanSecondaryAttack = false;
			break;
		case TF_WEAPON_MEDIGUN:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_LASER_POINTER:
			break;
		case TF_WEAPON_PARTICLE_CANNON:
		{
			float flChargeBeginTime = pWeapon->As<CTFParticleCannon>()->m_flChargeBeginTime();
			if (flChargeBeginTime > 0)
			{
				float flTotalChargeTime = TICKS_TO_TIME(pLocal->m_nTickBase()) - flChargeBeginTime;
				if (flTotalChargeTime < TF_PARTICLE_MAX_CHARGE_TIME)
				{
					G::CanPrimaryAttack = G::CanSecondaryAttack = false;
					break;
				}
			}
			[[fallthrough]];
		}
		default:
			if (pWeapon->GetSlot() != SLOT_MELEE)
			{
				bool bAmmo = pWeapon->HasPrimaryAmmoForShot();
				bool bReload = pWeapon->IsInReload();
				if (!bAmmo && pWeapon->m_iItemDefinitionIndex() != Soldier_m_TheBeggarsBazooka)
					G::CanPrimaryAttack = G::CanSecondaryAttack = false;
				if (bReload && bAmmo && !G::CanPrimaryAttack)
					G::Reloading = true;
			}
		}
		if (G::CanPrimaryAttack)
		{
			switch (pWeapon->GetWeaponID())
			{
			case TF_WEAPON_FLAMETHROWER:
			case TF_WEAPON_FLAME_BALL:
			case TF_WEAPON_FLAREGUN:
			case TF_WEAPON_FLAREGUN_REVENGE:
				if (pLocal->IsUnderwater())
					G::CanPrimaryAttack = G::CanSecondaryAttack = false;
			}
		}
	}

	G::Attacking = SDK::IsAttacking(pLocal, pWeapon, pCmd);
	G::PrimaryWeaponType = SDK::GetWeaponType(pWeapon, &G::SecondaryWeaponType);
	G::CanHeadshot = pWeapon->CanHeadshot() || pWeapon->AmbassadorCanHeadshot(TICKS_TO_TIME(pLocal->m_nTickBase()));
}

static inline void LocalAnimations(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	static std::vector<Vec3> vAngles = {};
	vAngles.push_back(pCmd->viewangles);
	auto pAnimState = pLocal->m_PlayerAnimState();
	if (G::SendPacket && pAnimState)
	{
		float flOldFrametime = I::GlobalVars->frametime;
		float flOldCurtime = I::GlobalVars->curtime;
		I::GlobalVars->frametime = TICK_INTERVAL;
		I::GlobalVars->curtime = TICKS_TO_TIME(pLocal->m_nTickBase());
		for (auto& vAngle : vAngles)
		{
			if (pLocal->IsTaunting() && pLocal->m_bAllowMoveDuringTaunt())
				pLocal->m_flTauntYaw() = vAngle.y;
			pAnimState->Update(pAnimState->m_flEyeYaw = vAngle.y, vAngle.x);
			pLocal->FrameAdvance(TICK_INTERVAL);
		}
		I::GlobalVars->frametime = flOldFrametime;
		I::GlobalVars->curtime = flOldCurtime;
		vAngles.clear();

		F::FakeAngle.Run(pLocal);
	}
}

static inline void AntiCheatCompatibility(CUserCmd* pCmd)
{
	if (!Vars::Misc::Game::AntiCheatCompatibility.Value)
		return;

	Math::ClampAngles(pCmd->viewangles); // shouldn't happen, but failsafe

	static std::deque<CmdHistory_t> vHistory;
	vHistory.emplace_front(pCmd->viewangles, pCmd->buttons & IN_ATTACK, pCmd->buttons & IN_ATTACK2, G::SendPacket);
	if (vHistory.size() > 5)
		vHistory.pop_back();

	if (vHistory.size() < 3)
		return;

	// prevent trigger checks, though this shouldn't happen ordinarily
	if (!vHistory[0].m_bAttack1 && vHistory[1].m_bAttack1 && !vHistory[2].m_bAttack1)
		pCmd->buttons |= IN_ATTACK;
	if (!vHistory[0].m_bAttack2 && vHistory[1].m_bAttack2 && !vHistory[2].m_bAttack2)
		pCmd->buttons |= IN_ATTACK2;

	// don't care if we are actually attacking or not, a miss is less important than a detection
	if (vHistory[0].m_bAttack1 || vHistory[1].m_bAttack1 || vHistory[2].m_bAttack1)
	{
		// prevent silent aim checks
		if (Math::CalcFov(vHistory[0].m_vAngle, vHistory[1].m_vAngle) > PSILENT_EPSILON
			&& Math::CalcFov(vHistory[0].m_vAngle, vHistory[2].m_vAngle) < REAL_EPSILON)
		{
			pCmd->viewangles = vHistory[1].m_vAngle.LerpAngle(vHistory[0].m_vAngle, 0.5f);
			if (Math::CalcFov(pCmd->viewangles, vHistory[2].m_vAngle) < REAL_EPSILON)
				pCmd->viewangles = vHistory[0].m_vAngle + Vec3(0.f, REAL_EPSILON * 2);
			vHistory[0].m_vAngle = pCmd->viewangles;
			vHistory[0].m_bSendingPacket = G::SendPacket = vHistory[1].m_bSendingPacket;
		}

		// prevent aim snap checks
		if (vHistory.size() == 5)
		{
			float flDelta01 = Math::CalcFov(vHistory[0].m_vAngle, vHistory[1].m_vAngle);
			float flDelta12 = Math::CalcFov(vHistory[1].m_vAngle, vHistory[2].m_vAngle);
			float flDelta23 = Math::CalcFov(vHistory[2].m_vAngle, vHistory[3].m_vAngle);
			float flDelta34 = Math::CalcFov(vHistory[3].m_vAngle, vHistory[4].m_vAngle);

			if ((
				flDelta12 > SNAP_SIZE_EPSILON && flDelta23 < SNAP_NOISE_EPSILON && vHistory[2].m_vAngle != vHistory[3].m_vAngle
				|| flDelta23 > SNAP_SIZE_EPSILON && flDelta12 < SNAP_NOISE_EPSILON && vHistory[1].m_vAngle != vHistory[2].m_vAngle
				)
				&& flDelta01 < SNAP_NOISE_EPSILON && vHistory[0].m_vAngle != vHistory[1].m_vAngle
				&& flDelta34 < SNAP_NOISE_EPSILON && vHistory[3].m_vAngle != vHistory[4].m_vAngle)
			{
				pCmd->viewangles.y += SNAP_NOISE_EPSILON * 2;
				vHistory[0].m_vAngle = pCmd->viewangles;
				vHistory[0].m_bSendingPacket = G::SendPacket = vHistory[1].m_bSendingPacket;
			}
		}
	}
}

struct CriticalStorage_t
{
	float m_flCritTokenBucket;
	int m_nCritChecks;
	int m_nCritSeedRequests;
};

class CSheet;
typedef __m128 fltx4;

struct UniqueId_t
{
	unsigned char m_Value[16];
};
typedef UniqueId_t DmObjectId_t;

class CParticleSystemDefinition
{
public:
	byte pad0[12]; // why is this here
	int m_nInitialParticles;
	int m_nPerParticleUpdatedAttributeMask;
	int m_nPerParticleInitializedAttributeMask;
	int m_nInitialAttributeReadMask;
	int m_nAttributeReadMask;
	uint64 m_nControlPointReadMask;
	Vector m_BoundingBoxMin;
	Vector m_BoundingBoxMax;
	char m_pszMaterialName[MAX_PATH];
	// bunch of shit i wont ever use
	void* m_Material;
	void* m_pFirstCollection;
	char m_pszCullReplacementName[128];
	float m_flCullRadius;
	float m_flCUllFillCost;
	int m_nCullControlPoint;
	Color_t m_ConstantColor;
	float m_flConstantRadius;
	float m_flConstantRotation;
	float m_flConstantRotationSpeed;
	int m_nConstantSequenceNumber;
	int m_nConstantSequenceNumber1;
	int m_nGroupID;
	float m_flMaximumTimeStep;
	float m_flMaximumSimTime;
	float m_flMinimumSimTime;
	int m_nMinimumFrames;
	bool m_bViewModelEffect;
	size_t m_nContextDataSize;
	DmObjectId_t m_Id;
	float m_flMaxDrawDistance;
	float m_flNoDrawTimeToGoToSleep;
	int m_nMaxParticles;
	int m_nSkipRenderControlPoint;

	CUtlString m_Name;
};

class CParticleCollection
{
public:
	CUtlReference<CSheet> m_Sheet;
	fltx4 m_fl4CurTime;
	int m_nPaddedActiveParticles;
	float m_flCurTime;
	int m_nActiveParticles;
	float m_flDt;
	float m_flPreviousDt;
	float m_flNextSleepTime;
	CUtlReference<CParticleSystemDefinition> m_pDef;
};

struct SpriteRenderInfo_t
{
	size_t m_nXYZStride{};
	fltx4* m_pXYZ{};
	size_t m_nRotStride{};
	fltx4* m_pRot{};
	size_t m_nYawStride{};
	fltx4* m_pYaw{};
	size_t m_nRGBStride{};
	fltx4* m_pRGB{};
	size_t m_nCreationTimeStride{};
	fltx4* m_pCreationTimeStamp{};
	size_t m_nSequenceStride{};
	fltx4* m_pSequenceNumber{};
	size_t m_nSequence1Stride{};
	fltx4* m_pSequence1Number{};
	float m_flAgeScale{};
	float m_flAgeScale2{};
	void* m_pSheet{};
	int m_nVertexOffset{};
	CParticleCollection* m_pParticles{};
};

struct ParticleRenderData_t
{
	float m_flSortKey;
	int   m_nIndex;
	float m_flRadius;
	uint8 m_nAlpha;
	uint8 m_nAlphaPad[3];
};

class IParticleEffect
{
public:
	virtual ~IParticleEffect() = 0;
	virtual void Update(float fTimeDelta) = 0;
	virtual void StartRender(VMatrix& effectMatrix) = 0;
	virtual bool ShouldSimulate() const = 0;
	virtual void SetShouldSimulate(bool bSim) = 0;
	virtual void SimulateParticles(void* pIterator) = 0;
	virtual void RenderParticles(void* pIterator) = 0;
	virtual void NotifyRemove() = 0;
	virtual void NotifyDestroyParticle(void* pParticle) = 0;
	virtual const Vector& GetSortOrigin() = 0;
	virtual const Vector* GetParticlePosition(void* pParticle) = 0;
	virtual const char* GetEffectName() = 0;
};

class CDefaultClientRenderable : public IClientUnknown, public IClientRenderable
{
	virtual const Vector& GetRenderOrigin(void) = 0;
	virtual const QAngle& GetRenderAngles(void) = 0;
	virtual const matrix3x4& RenderableToWorldTransform() = 0;
	virtual bool					ShouldDraw(void) = 0;
	virtual bool					IsTransparent(void) = 0;
	virtual bool					IsTwoPass(void) = 0;
	virtual void					OnThreadedDrawSetup() {}
	virtual bool					UsesPowerOfTwoFrameBufferTexture(void) = 0;
	virtual bool					UsesFullFrameBufferTexture(void) = 0;
	virtual ClientShadowHandle_t	GetShadowHandle() const = 0;
	virtual ClientRenderHandle_t& RenderHandle() = 0;
	virtual int						GetBody() = 0;
	virtual int						GetSkin() = 0;
	virtual bool					UsesFlexDelayedWeights() = 0;
	virtual const model_t* GetModel() const = 0;
	virtual int						DrawModel(int flags) = 0;
	virtual void					ComputeFxBlend() = 0;
	virtual int						GetFxBlend() = 0;
	virtual bool					LODTest() = 0;
	virtual bool					SetupBones(matrix3x4* pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime) = 0;
	virtual void					SetupWeights(const matrix3x4* pBoneToWorld, int nFlexWeightCount, float* pFlexWeights, float* pFlexDelayedWeights) = 0;
	virtual void					DoAnimationEvents(void) = 0;
	virtual IPVSNotify* GetPVSNotifyInterface() = 0;
	virtual void					GetRenderBoundsWorldspace(Vector& absMins, Vector& absMaxs) = 0;

	// Determine the color modulation amount
	virtual void	GetColorModulation(float* color) = 0;

	// Should this object be able to have shadows cast onto it?
	virtual bool	ShouldReceiveProjectedTextures(int flags) = 0;

	// These methods return true if we want a per-renderable shadow cast direction + distance
	virtual bool	GetShadowCastDistance(float* pDist, ShadowType_t shadowType) const = 0;
	virtual bool	GetShadowCastDirection(Vector* pDirection, ShadowType_t shadowType) const = 0;

	virtual void	GetShadowRenderBounds(Vector& mins, Vector& maxs, ShadowType_t shadowType) = 0;

	virtual bool IsShadowDirty() = 0;
	virtual void MarkShadowDirty(bool bDirty) = 0;
	virtual IClientRenderable* GetShadowParent() = 0;
	virtual IClientRenderable* FirstShadowChild() = 0;
	virtual IClientRenderable* NextShadowPeer() = 0;
	virtual ShadowType_t ShadowCastType() = 0;
	virtual void CreateModelInstance() = 0;
	virtual ModelInstanceHandle_t GetModelInstance() = 0;

	// Attachments
	virtual int LookupAttachment(const char* pAttachmentName) = 0;
	virtual	bool GetAttachment(int number, Vector& origin, QAngle& angles) = 0;
	virtual bool GetAttachment(int number, matrix3x4& matrix) = 0;

	// Rendering clip plane, should be 4 floats, return value of NULL indicates a disabled render clip plane
	virtual float* GetRenderClipPlane() = 0;

	virtual void RecordToolMessage() = 0;
	virtual bool IgnoresZBuffer(void) const = 0;

	// IClientUnknown implementation.
public:
	virtual void SetRefEHandle(const CBaseHandle& handle) = 0;
	virtual const CBaseHandle& GetRefEHandle() const = 0;

	virtual IClientUnknown* GetIClientUnknown() = 0;
	virtual ICollideable* GetCollideable() = 0;
	virtual IClientRenderable* GetClientRenderable() = 0;
	virtual IClientNetworkable* GetClientNetworkable() = 0;
	virtual IClientEntity* GetIClientEntity() = 0;
	virtual CBaseEntity* GetBaseEntity() = 0;
	virtual IClientThinkable* GetClientThinkable() = 0;
public:
	ClientRenderHandle_t m_hRenderHandle;
};

class CNewParticleEffect : public IParticleEffect, public CParticleCollection, public CDefaultClientRenderable
{
public:
	friend class CRefCountAccessor;

	CNewParticleEffect* m_pNext;
	CNewParticleEffect* m_pPrev;

	virtual bool IsTransparent() = 0;
	virtual bool IsTwoPass() = 0;
	virtual bool UsesPowerOfTwoFrameBufferTexture() = 0;
	virtual bool UsesFullFrameBufferTexture(void) = 0;
	virtual int DrawModel(int flags) = 0;
	virtual void SimulateParticles(void* pIterator) = 0;
	virtual void RenderParticles(void* pIterator) = 0;
	virtual void SetParticleCullRadius(float radius) = 0;
	virtual void NotifyRemove(void) = 0;
	virtual const Vector& GetSortOrigin(void) = 0;
	virtual void Update(float flTimeDelta) = 0;
	virtual bool ShouldSimulate() = 0;
	virtual void SetShouldSimulate(bool bSim) = 0;
	virtual ~CNewParticleEffect() = 0;

	// Used to track down bugs.
	const char* m_pDebugName;

	bool		m_bDontRemove : 1;
	bool		m_bRemove : 1;
	bool		m_bDrawn : 1;
	bool		m_bNeedsBBoxUpdate : 1;
	bool		m_bIsFirstFrame : 1;
	bool		m_bAutoUpdateBBox : 1;
	bool		m_bAllocated : 1;
	bool		m_bSimulate : 1;
	bool		m_bShouldPerformCullCheck : 1;

	int			m_nToolParticleEffectId;
	Vector		m_vSortOrigin;
	EHANDLE		m_hOwner;
	EHANDLE     m_hControlPointOwners[64];

	// holds the min/max bounds used to manage this thing in the client leaf system
	Vector		m_LastMin;
	Vector		m_LastMax;

	bool		m_bViewModelEffect;

	int			m_RefCount;		// When this goes to zero and the effect has no more active
	// particles, (and it's dynamically allocated), it will delete itself.
};

// classes for getting particle owners
// used for flamethrower's flames
class CTFFlameManager
{
public:
	NETVAR(m_hWeapon, EHANDLE, "CTFFlameManager", "m_hWeapon");
};
// used for medibeam in first person and stuff like muzzle flashes
class CBaseViewModel
{
public:
	NETVAR(m_hOwner, EHANDLE, "CBaseViewModel", "m_hOwner");
};

struct TickbaseFix_t
{
	CUserCmd* m_pCmd;
	int m_iLastOutgoingCommand;
	int m_iTickbaseShift;
};

struct CProxyAnimatedWeaponSheen
{
	__int64 idk;
	IMaterialVar* m_AnimatedTextureVar;
	IMaterialVar* m_AnimatedTextureFrameNumVar;
	float m_FrameRate;
	__declspec(align(4)) bool m_WrapAnimation;
	IMaterialVar* m_pSheenIndexVar;
	IMaterialVar* m_pTintVar;
	IMaterialVar* m_pSheenVar;
	IMaterialVar* m_pSheenMaskVar;
	IMaterialVar* m_pScaleXVar;
	IMaterialVar* m_pScaleYVar;
	IMaterialVar* m_pOffsetXVar;
	IMaterialVar* m_pOffsetYVar;
	IMaterialVar* m_pDirectionVar;
	float m_flNextStartTime;
	float m_flScaleX;
	float m_flScaleY;
	float m_flSheenOffsetX;
	float m_flSheenOffsetY;
	int m_iSheenDir;
};

class IRecipientFilter
{
public:
	virtual			~IRecipientFilter() {}

	virtual bool	IsReliable(void) const = 0;
	virtual bool	IsInitMessage(void) const = 0;

	virtual int		GetRecipientCount(void) const = 0;
	virtual int		GetRecipientIndex(int slot) const = 0;
};

struct CSoundParameters
{
	CSoundParameters()
	{
		channel = 0; // 0
		volume = 1.0f;  // 1.0f
		pitch = 100; // 100

		pitchlow = 100;
		pitchhigh = 100;

		soundlevel = SNDLVL_NORM; // 75dB
		soundname[0] = 0;
		play_to_owner_only = false;
		count = 0;

		delay_msec = 0;
	}

	int				channel;
	float			volume;
	int				pitch;
	int				pitchlow, pitchhigh;
	soundlevel_t	soundlevel;
	// For weapon sounds...
	bool			play_to_owner_only;
	int				count;
	char 			soundname[128];
	int				delay_msec;
};

struct EmitSound_t
{
	EmitSound_t() :
		m_nChannel(0),
		m_pSoundName(0),
		m_flVolume(1.0f),
		m_SoundLevel(0),
		m_nFlags(0),
		m_nPitch(100),
		m_nSpecialDSP(0),
		m_pOrigin(0),
		m_flSoundTime(0.0f),
		m_pflSoundDuration(0),
		m_bEmitCloseCaption(true),
		m_bWarnOnMissingCloseCaption(false),
		m_bWarnOnDirectWaveReference(false),
		m_nSpeakerEntity(-1),
		m_UtlVecSoundOrigin(),
		m_hSoundScriptHandle(-1)
	{
	}

	EmitSound_t(const CSoundParameters& src);

	int							m_nChannel;
	char const* m_pSoundName;
	float						m_flVolume;
	int				m_SoundLevel;
	int							m_nFlags;
	int							m_nPitch;
	int							m_nSpecialDSP;
	const Vector* m_pOrigin;
	float						m_flSoundTime; ///< NOT DURATION, but rather, some absolute time in the future until which this sound should be delayed
	float* m_pflSoundDuration;
	bool						m_bEmitCloseCaption;
	bool						m_bWarnOnMissingCloseCaption;
	bool						m_bWarnOnDirectWaveReference;
	int							m_nSpeakerEntity;
	mutable CUtlVector< Vector >	m_UtlVecSoundOrigin;  ///< Actual sound origin(s) (can be multiple if sound routed through speaker entity(ies) )
	mutable short		m_hSoundScriptHandle;
};

const static std::vector<const char*> s_vFootsteps = { "footstep", "flesh_impact_hard", "body_medium_impact_soft", "glass_sheet_step", "rubber_tire_impact_soft", "plastic_box_impact_soft", "plastic_barrel_impact_soft", "cardboard_box_impact_soft", "ceiling_tile_step" };
const static std::vector<const char*> s_vNoisemaker = { "items\\halloween", "items\\football_manager", "items\\japan_fundraiser", "items\\samurai\\tf_samurai_noisemaker", "items\\summer", "misc\\happy_birthday_tf", "misc\\jingle_bells" };
const static std::vector<const char*> s_vFryingPan = { "pan_" };
const static std::vector<const char*> s_vWater = { "ambient_mp3\\water\\water_splash", "slosh", "wade" };

static inline bool ShouldBlockSound(const char* pSound)
{
	if (!Vars::Misc::Sound::Block.Value || !pSound)
		return false;

	std::string sSound = pSound;
	std::transform(sSound.begin(), sSound.end(), sSound.begin(), ::tolower);
	auto CheckSound = [&](const std::vector<const char*>& vSounds, int iFlag = -1)
		{
			if (/*iFlag == -1 ||*/ Vars::Misc::Sound::Block.Value & iFlag)
			{
				for (auto& sNoise : vSounds)
				{
					if (sSound.find(sNoise) != std::string::npos)
						return true;
				}
			}
			return false;
		};

	if (CheckSound(s_vFootsteps, Vars::Misc::Sound::BlockEnum::Footsteps))
		return true;

	if (CheckSound(s_vNoisemaker, Vars::Misc::Sound::BlockEnum::Noisemaker))
		return true;

	if (CheckSound(s_vFryingPan, Vars::Misc::Sound::BlockEnum::FryingPan))
		return true;

	if (CheckSound(s_vWater, Vars::Misc::Sound::BlockEnum::Water))
		return true;

	return false;
}

inline void SetColorRender(CBaseEntity* pEntity, Color_t color)
{
	// drawmodel doesnt care about clrrender alpha (it runs its own) but other stuff does so keep it as what it was originally
	auto& m_clrRender = pEntity->m_clrRender();
	m_clrRender.r = color.r;
	m_clrRender.g = color.g;
	m_clrRender.b = color.b;
}

class CStaticProp
{
public:
	byte pad[20];
	Vector						m_Origin;
	QAngle						m_Angles;
	model_t* m_pModel;
	SpatialPartitionHandle_t	m_Partition;
	ModelInstanceHandle_t		m_ModelInstance;
	unsigned char				m_Alpha;
	unsigned char				m_nSolidType;
	unsigned char				m_Skin;
	unsigned char				m_Flags;
	unsigned short				m_FirstLeaf;
	unsigned short				m_LeafCount;
	CBaseHandle					m_EntHandle;	// FIXME: Do I need client + server handles?
	ClientRenderHandle_t		m_RenderHandle;
	unsigned short				m_FadeIndex;	// Index into the m_StaticPropFade dictionary
	float						m_flForcedFadeScale;

	// bbox is the same for both GetBounds and GetRenderBounds since static props never move.
	// GetRenderBounds is interpolated data, and GetBounds is last networked.
	Vector					m_RenderBBoxMin;
	Vector					m_RenderBBoxMax;
	matrix3x4				m_ModelToWorld;
	float					m_flRadius;

	Vector					m_WorldRenderBBoxMin;
	Vector					m_WorldRenderBBoxMax;

	// FIXME: This sucks. Need to store the lighting origin off
	// because the time at which the static props are unserialized
	// doesn't necessarily match the time at which we can initialize the light cache
	Vector					m_LightingOrigin;
};

static inline Color_t GetScoreboardColor(int iIndex)
{
	if (iIndex == I::EngineClient->GetLocalPlayer())
		return Vars::Colors::Local.Value;

	auto pResource = H::Entities.GetResource();
	if (!F::Groups.GroupsActive() || !pResource)
		return { 255, 255, 255, 255 };

	if (auto pEntity = I::ClientEntityList->GetClientEntity(iIndex)->As<CBaseEntity>())
	{
		Group_t* pGroup{};
		if (F::Groups.GetGroup(pEntity, pGroup, false))
			return F::Groups.GetColor(pEntity, pGroup);
	}

	return { 255, 255, 255, 255 };
}

enum ECritBoostUpdateType 
{ 
	kCritBoost_Ignore, 
	kCritBoost_ForceRefresh 
};

struct Restore_t
{
	Vec3 m_vOrigin;
	Vec3 m_vMins;
	Vec3 m_vMaxs;
};

#define MAX_CHECK_COUNT_DEPTH 2
typedef uint32 TraceCounter_t;
typedef CUtlVector<TraceCounter_t> CTraceCounterVec;

struct TraceInfo_t
{
	Vector m_start;
	Vector m_end;
	Vector m_mins;
	Vector m_maxs;
	Vector m_extents;
	Vector m_delta;
	Vector m_invDelta;
	trace_t m_trace;
	trace_t m_stabTrace;
	int m_contents;
	bool m_ispoint;
	bool m_isswept;
	void* m_pBSPData;
	Vector m_DispStabDir;
	int m_bDispHit;
	bool m_bCheckPrimary;
	int m_nCheckDepth;
	TraceCounter_t m_Count[MAX_CHECK_COUNT_DEPTH];
	CTraceCounterVec m_BrushCounters[MAX_CHECK_COUNT_DEPTH];
	CTraceCounterVec m_DispCounters[MAX_CHECK_COUNT_DEPTH];
};

static void PopIdName(SteamNetworkingPOPID popID, char* out)
{
	out[0] = static_cast<char>(popID >> 16);
	out[1] = static_cast<char>(popID >> 8);
	out[2] = static_cast<char>(popID);
	out[3] = static_cast<char>(popID >> 24);
	out[4] = 0;
}

inline int GetDatacenter(uint32_t uHash)
{
	switch (uHash)
	{
	case FNV1A::Hash32Const("atl"): return Vars::Misc::Queueing::ForceRegionsEnum::ATL;
	case FNV1A::Hash32Const("ord"): return Vars::Misc::Queueing::ForceRegionsEnum::ORD;
	case FNV1A::Hash32Const("dfw"): return Vars::Misc::Queueing::ForceRegionsEnum::DFW;
	case FNV1A::Hash32Const("lax"): return Vars::Misc::Queueing::ForceRegionsEnum::LAX;
	case FNV1A::Hash32Const("sea"):
	case FNV1A::Hash32Const("eat"): return Vars::Misc::Queueing::ForceRegionsEnum::SEA;
	case FNV1A::Hash32Const("iad"): return Vars::Misc::Queueing::ForceRegionsEnum::IAD;
	case FNV1A::Hash32Const("ams"):
	case FNV1A::Hash32Const("ams4"): return Vars::Misc::Queueing::ForceRegionsEnum::AMS;
	case FNV1A::Hash32Const("fsn"): return Vars::Misc::Queueing::ForceRegionsEnum::FSN;
	case FNV1A::Hash32Const("fra"): return Vars::Misc::Queueing::ForceRegionsEnum::FRA;
	case FNV1A::Hash32Const("hel"): return Vars::Misc::Queueing::ForceRegionsEnum::HEL;
	case FNV1A::Hash32Const("lhr"): return Vars::Misc::Queueing::ForceRegionsEnum::LHR;
	case FNV1A::Hash32Const("mad"): return Vars::Misc::Queueing::ForceRegionsEnum::MAD;
	case FNV1A::Hash32Const("par"): return Vars::Misc::Queueing::ForceRegionsEnum::PAR;
	case FNV1A::Hash32Const("sto"):
	case FNV1A::Hash32Const("sto2"): return Vars::Misc::Queueing::ForceRegionsEnum::STO;
	case FNV1A::Hash32Const("vie"): return Vars::Misc::Queueing::ForceRegionsEnum::VIE;
	case FNV1A::Hash32Const("waw"): return Vars::Misc::Queueing::ForceRegionsEnum::WAW;
	case FNV1A::Hash32Const("eze"): return Vars::Misc::Queueing::ForceRegionsEnum::EZE;
	case FNV1A::Hash32Const("lim"): return Vars::Misc::Queueing::ForceRegionsEnum::LIM;
	case FNV1A::Hash32Const("scl"): return Vars::Misc::Queueing::ForceRegionsEnum::SCL;
	case FNV1A::Hash32Const("gru"): return Vars::Misc::Queueing::ForceRegionsEnum::GRU;
	case FNV1A::Hash32Const("maa2"): return Vars::Misc::Queueing::ForceRegionsEnum::MAA;
	case FNV1A::Hash32Const("dxb"): return Vars::Misc::Queueing::ForceRegionsEnum::DXB;
	case FNV1A::Hash32Const("hkg"): return Vars::Misc::Queueing::ForceRegionsEnum::HKG;
	case FNV1A::Hash32Const("bom2"): return Vars::Misc::Queueing::ForceRegionsEnum::BOM;
	case FNV1A::Hash32Const("seo"): return Vars::Misc::Queueing::ForceRegionsEnum::SEO;
	case FNV1A::Hash32Const("sgp"): return Vars::Misc::Queueing::ForceRegionsEnum::SGP;
	case FNV1A::Hash32Const("tyo"): return Vars::Misc::Queueing::ForceRegionsEnum::TYO;
	case FNV1A::Hash32Const("syd"): return Vars::Misc::Queueing::ForceRegionsEnum::SYD;
	case FNV1A::Hash32Const("jnb"): return Vars::Misc::Queueing::ForceRegionsEnum::JNB;
	}
	return 0;
}

inline int GetNetworkBase(int nTick, int nEntity)
{
	int nEntityMod = nEntity % I::GlobalVars->nTimestampRandomizeWindow;
	int nBaseTick = I::GlobalVars->nTimestampNetworkingBase * int((nTick - nEntityMod) / I::GlobalVars->nTimestampNetworkingBase);
	return nBaseTick;
}