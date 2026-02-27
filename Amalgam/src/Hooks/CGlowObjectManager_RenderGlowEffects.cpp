#include "../SDK/SDK.h"
#include "../Features/Visuals/Groups/Groups.h"
#include <unordered_set>

static inline Color_t GetHealthColor(CBaseEntity* pEntity, Group_t* pGroup, bool bAllowOverheal)
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

MAKE_SIGNATURE(CGlowObjectManager_RenderGlowEffects, "client.dll", "48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B E9 41 8B F8 48 8B 0D", 0x0);
MAKE_SIGNATURE(CGlowObjectManager_ApplyEntityGlowEffects, "client.dll", "48 8B C4 48 89 58 ? 48 89 50 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 0F 29 70", 0x0);

std::unordered_map<int, CGlowObject*> mGlowObjects;

static inline void DestroyGlowObject(std::unordered_map<int, CGlowObject*>::iterator& it)
{
	delete it->second;
	it->second = nullptr;
}

MAKE_HOOK(CGlowObjectManager_RenderGlowEffects, S::CGlowObjectManager_RenderGlowEffects(), void,
    CGlowObjectManager* rcx, const CViewSetup* pSetup, int nSplitScreenSlot)
{
    DEBUG_RETURN(CGlowObjectManager_RenderGlowEffects, rcx, pSetup, nSplitScreenSlot);

    if (!rcx)
        return CALL_ORIGINAL(rcx, pSetup, nSplitScreenSlot);
    
    if (!Vars::Colors::ClassicGlow.Value || G::Unload || !F::Groups.GroupsActive() || 
        Vars::Visuals::UI::CleanScreenshots.Value && I::EngineClient->IsTakingScreenshot())
    {
        for (auto& [pEntity, pGroup] : F::Groups.GetGroup())
        {
            if (!pEntity)
                continue;
            if (pEntity->IsBaseCombatCharacter())
                pEntity->As<CBaseCombatCharacter>()->m_bGlowEnabled() = false;
            else if (pEntity->GetClassID() == ETFClassID::CCaptureFlag)
                pEntity->As<CCaptureFlag>()->m_bGlowEnabled() = false;
        }

        for (auto& [pEntity, pGlow] : mGlowObjects)
        {
			delete pGlow;
			pGlow = nullptr;
        }

        mGlowObjects.clear();

        return CALL_ORIGINAL(rcx, pSetup, nSplitScreenSlot);
    }

    std::unordered_set<int> activeIndices;
     
    for (auto& [pEntity, pGroup] : F::Groups.GetGroup())
    {
        if (!pEntity || !pGroup)
            continue;

        const int iIndex = pEntity->entindex();
        if (iIndex <= 0 || !I::ClientEntityList->GetClientEntity(iIndex))
            continue;

        activeIndices.insert(iIndex);

        if (pGroup->m_tGlow())
        {
            Color_t tColor;

            EHANDLE pOwner = pEntity->IsBuilding()
				? pEntity->As<CBaseObject>()->m_hBuilder()
				: pEntity->m_hOwnerEntity();
            if (pGroup->m_bUseHealthGlow && 
                (pOwner ? (pOwner->IsBuilding() || pOwner->IsPlayer()) : (pEntity->IsBuilding() || pEntity->IsPlayer())))
                tColor = GetHealthColor(pOwner ? pOwner.Get() : pEntity, pGroup, true);
            else
                tColor = F::Groups.GetColor(pEntity, pGroup);

            const Vec3 vColor = { tColor.r / 255.f, tColor.g / 255.f, tColor.b / 255.f };
            const float flAlpha = tColor.a / 255.f;

            auto it = mGlowObjects.find(iIndex);
            if (it == mGlowObjects.end())
                mGlowObjects[iIndex] = new CGlowObject(rcx, pEntity, vColor, flAlpha, true, true, GLOW_FOR_ALL_SPLIT_SCREEN_SLOTS);
            else
            {
                CGlowObject* pGlow = it->second;
                pGlow->SetEntity(pEntity);
                pGlow->SetColor(vColor);
                pGlow->SetAlpha(flAlpha);
                pGlow->SetRenderFlags(true, true);
            }
        }
        else
        {
            auto it = mGlowObjects.find(iIndex);
            if (it != mGlowObjects.end())
            {
				DestroyGlowObject(it);
                mGlowObjects.erase(it);
            }
        }
    }

    for (auto it = mGlowObjects.begin(); it != mGlowObjects.end();)
    {
        if (!I::ClientEntityList->GetClientEntity(it->first) || activeIndices.find(it->first) == activeIndices.end())
        {
			DestroyGlowObject(it);
            it = mGlowObjects.erase(it);
        }
        else
            ++it;
    }

    // rebuild so we can ignore the convar checks
    auto pRenderContext = CMatRenderContextPtr(I::MaterialSystem);
    
    int nX, nY, nWidth, nHeight;
    pRenderContext->GetViewport(nX, nY, nWidth, nHeight);

    PIXEvent _pixEvent(pRenderContext, "EntityGlowEffects");
    // flBloomScale doesn't matter because it's never used
    S::CGlowObjectManager_ApplyEntityGlowEffects.Call<void>(rcx, pSetup, nSplitScreenSlot, &pRenderContext, 999999.f, nX, nY, nWidth, nHeight);
}