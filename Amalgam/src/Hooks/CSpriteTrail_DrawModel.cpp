#include "../SDK/SDK.h"
#include "../Features/Visuals/Groups/Groups.h"

MAKE_SIGNATURE(CSpriteTrail_DrawModel, "client.dll", "48 8B C4 55 57 41 54 41 55 41 56 48 8D A8", 0x0);

inline void SetColorRender(CBaseEntity* pEntity, Color_t color)
{
	// drawmodel doesnt care about clrrender alpha (it runs its own) but other stuff does so keep it as what it was originally
	auto& m_clrRender = pEntity->m_clrRender();
	m_clrRender.r = color.r;
	m_clrRender.g = color.g;
	m_clrRender.b = color.b;
}

MAKE_HOOK(CSpriteTrail_DrawModel, S::CSpriteTrail_DrawModel(), int,
	void* rcx, int flags)
{
	DEBUG_RETURN(CSpriteTrail_DrawModel, rcx, flags);

	// clean screenshots wont do anything if we set m_clrRender directly
	if (!(Vars::Visuals::World::Modulations.Value & Vars::Visuals::World::ModulationsEnum::Particle))
		return CALL_ORIGINAL(rcx, flags);

	auto pSpriteTrail = reinterpret_cast<CBaseEntity*>(rcx);
	if (!pSpriteTrail)
		return CALL_ORIGINAL(rcx, flags);

	switch (Vars::Visuals::World::ParticleModulationStyle.Value)
	{
	case Vars::Visuals::World::ParticleModulationStyleEnum::Rainbow:
	{
		SetColorRender(pSpriteTrail, H::Draw.Rainbow())
		break;
	}
	case Vars::Visuals::World::ParticleModulationStyleEnum::SolidColor:
	{
		SetColorRender(pSpriteTrail, Vars::Colors::ParticleModulation.Value);
		break;
	}
	case Vars::Visuals::World::ParticleModulationStyleEnum::GroupColored:
	{
		auto pOwnerEntity = pSpriteTrail->m_hOwnerEntity().Get();
		if (!pOwnerEntity)
			return CALL_ORIGINAL(rcx, flags);

		if (Group_t* pGroup{}; F::Groups.GetGroup(pOwnerEntity, pGroup, false))
			SetColorRender(pSpriteTrail, F::Groups.GetColor(pOwnerEntity, pGroup));
	}
	}

	return CALL_ORIGINAL(rcx, flags);
}