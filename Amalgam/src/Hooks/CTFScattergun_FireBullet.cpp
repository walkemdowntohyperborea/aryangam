#include "../SDK/SDK.h"

MAKE_SIGNATURE(CBaseEntity_ApplyAbsVelocityImpulse, "client.dll", "48 89 5C 24 ? 57 48 83 EC ? F3 0F 10 05 ? ? ? ? 48 8B FA", 0x0);
MAKE_SIGNATURE(CTFScattergun_FireBullet, "client.dll", "48 89 74 24 ? 48 89 7C 24 ? 4C 89 74 24 ? 55 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 01", 0x0);
MAKE_SIGNATURE(CTFPlayerShared_StunPlayer, "client.dll", "40 53 57 41 54 48 81 EC ? ? ? ? 0F 29 B4 24", 0x0);

MAKE_HOOK(CBaseEntity_ApplyAbsVelocityImpulse, S::CBaseEntity_ApplyAbsVelocityImpulse(), void,
	CBaseEntity* rcx, const Vector& inVecImpulse)
{
	DEBUG_RETURN(CBaseEntity_ApplyAbsVelocityImpulse, rcx, inVecImpulse);

	if (!rcx || !rcx->IsPlayer())
		return CALL_ORIGINAL(rcx, inVecImpulse);

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

	CALL_ORIGINAL(rcx, vecForce * flImpulseScale);
}

static bool m_bScattergunJump = false;
MAKE_HOOK(CTFScattergun_FireBullet, S::CTFScattergun_FireBullet(), void,
	void* rcx, CTFPlayer* pPlayer)
{
	DEBUG_RETURN(CTFScattergun_FireBullet, rcx, pPlayer);

	const auto pLocal = H::Entities.GetLocal();
	const auto pWeapon = reinterpret_cast<CBaseCombatWeapon*>(rcx);
	if (!pPlayer || !pWeapon)
		return; // this will crash anyways

	if(!pLocal || pPlayer->entindex() != pLocal->entindex())
		return CALL_ORIGINAL(rcx, pPlayer);

	const auto pNetChannel = I::EngineClient->GetNetChannelInfo();
	if (!pNetChannel)
		return CALL_ORIGINAL(rcx, pPlayer);

	bool bIsForceANature = pWeapon->m_iItemDefinitionIndex() == Scout_m_ForceANature || pWeapon->m_iItemDefinitionIndex() == Scout_m_FestiveForceANature;
	if (SDK::GetRoundState() == GR_STATE_PREROUND || m_bScattergunJump ||
		!bIsForceANature || pLocal->IsOnGround())
		return CALL_ORIGINAL(rcx, pPlayer);

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

	static auto CBaseEntity_ApplyAbsVelocityImpulse = U::Hooks.m_mHooks["CBaseEntity_ApplyAbsVelocityImpulse"];
	CBaseEntity_ApplyAbsVelocityImpulse->Call<void>(pLocal, Vec3(0, 0, 50.f));
	pLocal->m_fFlags() &= ~FL_ONGROUND;

	CALL_ORIGINAL(rcx, pPlayer);
}

MAKE_HOOK(CTFGameMovement_SetGroundEntity, U::Memory.GetVirtual(I::GameMovement, 21), void,
	void* rcx, trace_t* pm)
{
	DEBUG_RETURN(CTFGameMovement_SetGroundEntity, rcx, pm);

	CALL_ORIGINAL(rcx, pm);

	if(pm && pm->m_pEnt == H::Entities.GetLocal())
		m_bScattergunJump = false;
}