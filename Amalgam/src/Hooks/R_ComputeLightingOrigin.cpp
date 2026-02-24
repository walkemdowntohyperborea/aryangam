#include "../SDK/SDK.h"

MAKE_SIGNATURE(R_ComputeLightingOrigin, "engine.dll", "48 89 5C 24 ? 57 48 83 EC ? 48 63 82", 0x0);

MAKE_HOOK(R_ComputeLightingOrigin, S::R_ComputeLightingOrigin(), void,
	IClientRenderable* pRenderable, studiohdr_t* pStudioHdr, const matrix3x4& matrix, Vector& center)
{
	DEBUG_RETURN(R_ComputeLightingOrigin, pRenderable, pStudioHdr, matrix, center);

	if (!I::EngineClient->IsInGame() || !I::EngineClient->IsConnected() || !H::Entities.GetLocal())
		return CALL_ORIGINAL(pRenderable, pStudioHdr, matrix, center);

	if (auto pEntity = reinterpret_cast<CBaseEntity*>(pRenderable))
	{
		if (auto pOwner = pEntity->m_hOwnerEntity().Get(); pOwner->IsPlayer())
		{
			center = pOwner->GetRenderCenter();
			return;
		}
		else if(pEntity->IsPlayer())
		{
			center = pEntity->GetRenderCenter();
			return;
		}
	}

	CALL_ORIGINAL(pRenderable, pStudioHdr, matrix, center);
}