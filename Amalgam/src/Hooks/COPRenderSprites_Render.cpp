#pragma warning(disable: 4267)

#include "COPRenderSprites_Render.h"

#include "../Features/Simulation/ProjectileSimulation/ProjectileSimulation.h"
#include "../Features/Visuals/Groups/Groups.h"

MAKE_SIGNATURE(COPRenderSprites_Render, "client.dll", "48 89 54 24 ? 55 53 57 41 55 41 56", 0x0);
MAKE_SIGNATURE(COPRenderSprites_RenderSpriteCard, "client.dll", "48 8B C4 48 89 58 ? 57 41 54", 0x0);
MAKE_SIGNATURE(COPRenderSprites_RenderTwoSequenceSpriteCard, "client.dll", "48 8B C4 48 89 58 ? 48 89 68 ? 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? F3 0F 10 1D", 0x0);
MAKE_SIGNATURE(CNewParticleEffect_DrawModel, "client.dll", "4C 8B DC 53 57 41 55", 0x0);
MAKE_SIGNATURE(CNewParticleEffect_Deconstructor, "client.dll", "48 89 5C 24 ? 56 48 83 EC ? 48 8D 05 ? ? ? ? 33 F6", 0x0);
MAKE_SIGNATURE(CParticleCollection_Render, "client.dll", "48 89 6C 24 ? 57 41 56 41 57 48 83 EC ? 48 8B F9", 0x0);

MAKE_HOOK(COPRenderSprites_Render, S::COPRenderSprites_Render(), void,
	void* rcx, IMatRenderContext* pRenderContext, CParticleCollection* pParticles, void* pContext)
{
	DEBUG_RETURN(COPRenderSprites_Render, rcx, pRenderContext, pParticles, pContext);

	if (Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return CALL_ORIGINAL(rcx, pRenderContext, pParticles, pContext);

	bool bValid = false;

	if (Vars::Visuals::Effects::DrawCritsThroughWalls.Value)
	{
		std::string sParticleName = pParticles->m_pDef->m_Name.m_pString;
		std::transform(sParticleName.begin(), sParticleName.end(), sParticleName.begin(), ::tolower);
		if (sParticleName.find("crit") != std::string::npos)
			bValid = true;
	}

	if(Vars::Visuals::Effects::DrawIconsThroughWalls.Value && !bValid)
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
		return CALL_ORIGINAL(rcx, pRenderContext, pParticles, pContext);

	pRenderContext->DepthRange(0.f, 0.2f);
	CALL_ORIGINAL(rcx, pRenderContext, pParticles, pContext);
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

MAKE_HOOK(CNewParticleEffect_DrawModel, S::CNewParticleEffect_DrawModel(), int,
	void* rcx, int flags)
{
	DEBUG_RETURN(CNewParticleEffect_DrawModel, rcx, flags);

	if (Vars::Visuals::World::ParticleModulationStyle.Value != Vars::Visuals::World::ParticleModulationStyleEnum::GroupColored)
		return CALL_ORIGINAL(rcx, flags);

	// reserved here and not outside func so we dont dump memory into these when we will never use it
	static bool bInitialized = false;
	if (!bInitialized)
	{
		ParticlesPerOwner.reserve(I::EngineClient->GetMaxClients());
		OwnerPerParticle.reserve(size_t(I::EngineClient->GetMaxClients() * 8));
		for (size_t i = 1; i <= 32; i++)
			ParticlesPerOwner[i].reserve(8);
		bInitialized = true;
	}

	pOwnerEnt = (*(EHANDLE*)(((uintptr_t)rcx - 16) + 10212)).Get();
	if (!pOwnerEnt)
	{
		if (auto pOwner = FindOwner(rcx))
			pOwnerEnt = pOwner;
		else
			return CALL_ORIGINAL(rcx, flags);
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

	auto original = CALL_ORIGINAL(rcx, flags);
	pOwnerEnt = nullptr;
	return original;
}

MAKE_HOOK(CNewParticleEffect_Deconstructor, S::CNewParticleEffect_Deconstructor(), void,
	void* rcx)
{
	DEBUG_RETURN(CNewParticleEffect_Deconstructor, rcx);

	CALL_ORIGINAL(rcx);

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

MAKE_HOOK(COPRenderSprites_RenderSpriteCard, S::COPRenderSprites_RenderSpriteCard(), void,
	void* rcx, void* meshBuilder, void* pCtx, SpriteRenderInfo_t& info, int hParticle, ParticleRenderData_t* pSortList, void* pCamera)
{
	DEBUG_RETURN(COPRenderSprites_RenderSpriteCard, rcx, meshBuilder, pCtx, info, hParticle, pSortList, pCamera);

	if (!(Vars::Visuals::World::Modulations.Value & Vars::Visuals::World::ModulationsEnum::Particle) 
		|| Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return CALL_ORIGINAL(rcx, meshBuilder, pCtx, info, hParticle, pSortList, pCamera);

	Color_t color = Vars::Colors::ParticleModulation.Value;
	switch (Vars::Visuals::World::ParticleModulationStyle.Value)
	{
	case Vars::Visuals::World::ParticleModulationStyleEnum::GroupColored:
		if (pOwnerEnt)
		{
			if (Group_t* pGroup{}; F::Groups.GetGroup(pOwnerEnt, pGroup, false))
				color = F::Groups.GetColor(pOwnerEnt, pGroup);
			else
				return CALL_ORIGINAL(rcx, meshBuilder, pCtx, info, hParticle, pSortList, pCamera);
		}
		break;
	case Vars::Visuals::World::ParticleModulationStyleEnum::Rainbow:
		color = H::Draw.Rainbow().Alpha(color.a);
	}

	if(!color.a)
		return CALL_ORIGINAL(rcx, meshBuilder, pCtx, info, hParticle, pSortList, pCamera);
	
	info.m_pRGB[((hParticle / 4) * info.m_nRGBStride) + 0].m128_f32[hParticle & 0x3] = color.r / 255.f;
	info.m_pRGB[((hParticle / 4) * info.m_nRGBStride) + 1].m128_f32[hParticle & 0x3] = color.g / 255.f;
	info.m_pRGB[((hParticle / 4) * info.m_nRGBStride) + 2].m128_f32[hParticle & 0x3] = color.b / 255.f;
	if (color.a != 255)
		pSortList->m_nAlpha = color.a;
	CALL_ORIGINAL(rcx, meshBuilder, pCtx, info, hParticle, pSortList, pCamera);
}

MAKE_HOOK(COPRenderSprites_RenderTwoSequenceSpriteCard, S::COPRenderSprites_RenderTwoSequenceSpriteCard(), void,
	void* rcx, void* meshBuilder, void* pCtx, SpriteRenderInfo_t& info, int hParticle, ParticleRenderData_t* pSortList, void* pCamera)
{
#ifdef DEBUG_HOOKS
	if (!Vars::Hooks::COPRenderSprites_Render[DEFAULT_BIND])
		return CALL_ORIGINAL(rcx, meshBuilder, pCtx, info, hParticle, pSortList, pCamera);
#endif

	if (!(Vars::Visuals::World::Modulations.Value &Vars::Visuals::World::ModulationsEnum::Particle) 
		|| Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
		return CALL_ORIGINAL(rcx, meshBuilder, pCtx, info, hParticle, pSortList, pCamera);

	Color_t color = Vars::Colors::ParticleModulation.Value;
	switch (Vars::Visuals::World::ParticleModulationStyle.Value)
	{
	case Vars::Visuals::World::ParticleModulationStyleEnum::GroupColored:
		if (pOwnerEnt)
		{
			if (Group_t* pGroup{}; F::Groups.GetGroup(pOwnerEnt, pGroup, false))
				color = F::Groups.GetColor(pOwnerEnt, pGroup);
			else
				return CALL_ORIGINAL(rcx, meshBuilder, pCtx, info, hParticle, pSortList, pCamera);
		}
		break;
	case Vars::Visuals::World::ParticleModulationStyleEnum::Rainbow:
		color = {
				static_cast<byte>(floor(sin(I::GlobalVars->curtime) * 127.0f + 128.0f)),
				static_cast<byte>(floor(sin(I::GlobalVars->curtime + 2.0f) * 127.0f + 128.0f)),
				static_cast<byte>(floor(sin(I::GlobalVars->curtime + 4.0f) * 127.0f + 128.0f)),
				color.a
		};
	}

	if (!color.a)
		return CALL_ORIGINAL(rcx, meshBuilder, pCtx, info, hParticle, pSortList, pCamera);

	info.m_pRGB[((hParticle / 4) * info.m_nRGBStride) + 0].m128_f32[hParticle & 0x3] = color.r / 255.f;
	info.m_pRGB[((hParticle / 4) * info.m_nRGBStride) + 1].m128_f32[hParticle & 0x3] = color.g / 255.f;
	info.m_pRGB[((hParticle / 4) * info.m_nRGBStride) + 2].m128_f32[hParticle & 0x3] = color.b / 255.f;
	if (color.a != 255)
		pSortList->m_nAlpha = Vars::Colors::ParticleModulation.Value.a;
	CALL_ORIGINAL(rcx, meshBuilder, pCtx, info, hParticle, pSortList, pCamera);
}

MAKE_HOOK(CParticleCollection_Render, S::CParticleCollection_Render(), void,
	void* rcx, IMatRenderContext* pRenderContext, bool bTranslucentOnly, void* pCameraObject)
{
	DEBUG_RETURN(CParticleCollection_Render, rcx, pRenderContext, bTranslucentOnly, pCameraObject);

	auto pParticleCollection = reinterpret_cast<CParticleCollection*>(rcx);
	if (!pParticleCollection || !pOwnerEnt)
		return CALL_ORIGINAL(rcx, pRenderContext, bTranslucentOnly, pCameraObject);

	auto& pDef = pParticleCollection->m_pDef; // some hud shit can cause crashes for no reason
	if (pDef && FNV1A::Hash32(pDef->m_Name.m_pString) != FNV1A::Hash32Const("laser_sight_beam"))
		return CALL_ORIGINAL(rcx, pRenderContext, bTranslucentOnly, pCameraObject);
	
	// wrangler
	if (pOwnerEnt->IsPlayer() && pOwnerEnt->As<CTFPlayer>()->m_iClass() != TF_CLASS_SNIPER)
		return CALL_ORIGINAL(rcx, pRenderContext, bTranslucentOnly, pCameraObject);

	Group_t* pGroup{};
	bool bFoundGroup = F::Groups.GetGroup(pOwnerEnt, pGroup, false);
	if (!bFoundGroup || bFoundGroup && !pGroup->m_bSightlinesIgnoreZ)
		return CALL_ORIGINAL(rcx, pRenderContext, bTranslucentOnly, pCameraObject);

	pRenderContext->DepthRange(0.f, 0.2f);
	CALL_ORIGINAL(rcx, pRenderContext, bTranslucentOnly, pCameraObject);
	pRenderContext->DepthRange(0.f, 1.f);
}