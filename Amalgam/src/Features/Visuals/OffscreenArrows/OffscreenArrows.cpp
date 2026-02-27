#include "OffscreenArrows.h"
#include "../../Visuals/Groups/Groups.h"
#include "../../ImGui/Menu/Menu.h"

// cr: seowned de for arc code and nitro for actual indicators look

static inline void Arc(int x, int y, int radius, float thickness, float start, float end, Color_t clr)
{
	static const float flPrecision = 1.f;
	static const float flStep = static_cast<float>(PI / 180.0f);

	float flInner = radius - thickness;

	for (float flAngle = start; flAngle < start + end; flAngle += flPrecision)
	{
		float flRad = flAngle * flStep;
		float flRad2 = (flAngle + flPrecision) * flStep;

		float flRadCos = std::cosf(flRad);
		float flRadSin = std::sinf(flRad);
		float flRad2Cos = std::cosf(flRad2);
		float flRad2Sin = std::sinf(flRad2);

		Vec2 vecInner1 = { x + flRadCos * flInner, y + flRadSin * flInner };
		Vec2 vecInner2 = { x + flRad2Cos * flInner, y + flRad2Sin * flInner };

		Vec2 vecOuter1 = { x + flRadCos * radius, y + flRadSin * radius };
		Vec2 vecOuter2 = { x + flRad2Cos * radius, y + flRad2Sin * radius };

		std::vector<Vertex_t> polys1 = { Vertex_t{ vecOuter1 }, Vertex_t{ vecOuter2 }, Vertex_t{ vecInner1 } };
		H::Draw.FillPolygon(polys1, clr);

		std::vector<Vertex_t> polys2 = { Vertex_t{ vecInner1 }, Vertex_t{ vecOuter2 }, Vertex_t{ vecInner2 } };
		H::Draw.FillPolygon(polys2, clr);
	}
}

// Helper function to calculate distance from screen center to edge in a given direction
static float GetDistanceToScreenEdge(float flScreenAngle, int screenW, int screenH)
{
	float flCenterX = screenW / 2.f;
	float flCenterY = screenH / 2.f;

	// Convert angle to radians (0 = top, 90 = right, etc.)
	float flRad = (flScreenAngle - 90.f) * (PI / 180.f);

	// Direction vector
	float flDirX = std::cosf(flRad);
	float flDirY = std::sinf(flRad);

	// Calculate distances to edges
	float flDistToEdge = FLT_MAX;

	if (flDirX > 0.001f)
		flDistToEdge = std::min(flDistToEdge, (screenW - flCenterX) / flDirX);
	else if (flDirX < -0.001f)
		flDistToEdge = std::min(flDistToEdge, flCenterX / -flDirX);

	if (flDirY > 0.001f)
		flDistToEdge = std::min(flDistToEdge, (screenH - flCenterY) / flDirY);
	else if (flDirY < -0.001f)
		flDistToEdge = std::min(flDistToEdge, flCenterY / -flDirY);

	return flDistToEdge;
}

void COffscreenArrows::Store(CTFPlayer* pLocal)
{
	m_mCache.clear();
	if (!F::Groups.GroupsActive())
		return;

	Vec3 vLocalPos = pLocal->GetEyePosition();
	for (auto& [pEntity, pGroup] : F::Groups.GetGroup(false))
	{
		if (!pGroup->m_bOffscreenArrows || pEntity->entindex() == I::EngineClient->GetLocalPlayer())
			continue;

		ArrowCache_t& tCache = m_mCache[pEntity];
		tCache.m_tColor = F::Groups.GetColor(pEntity, pGroup);
		tCache.m_iOffset = pGroup->m_iOffscreenArrowsOffset;
		tCache.m_flMaxDistance = pGroup->m_flOffscreenArrowsMaxDistance;

		float flWorldDist = pEntity->m_vecOrigin().DistTo(pLocal->m_vecOrigin());

		bool bIsDormant = pEntity->IsDormant();

		if (bIsDormant)
		{
			if (!tCache.m_bWasDormant)
				tCache.m_flLastValidDistance = flWorldDist;
			tCache.m_flDistance = tCache.m_flLastValidDistance;
		}
		else
		{
			tCache.m_flDistance = flWorldDist;
			tCache.m_flLastValidDistance = flWorldDist;
		}

		tCache.m_bWasDormant = bIsDormant;

		if (bIsDormant)
			tCache.m_tColor.a = static_cast<byte>(tCache.m_tColor.a * (0.6f + 0.4f * sinf(I::GlobalVars->curtime * 5.f)));
		else if (tCache.m_flDistance < 400 && pGroup->m_bOffscreenArrowsAlert)
			tCache.m_tColor.a = static_cast<byte>(tCache.m_tColor.a * (0.6f + 0.4f * sinf(I::GlobalVars->curtime * 55.f)));
		else
			tCache.m_tColor.a = static_cast<byte>(Math::RemapVal(pEntity->GetCenter().DistTo(vLocalPos), tCache.m_flMaxDistance, tCache.m_flMaxDistance * 0.9f, 0.f, 1.f) * 255.f);
	}
}

void COffscreenArrows::DrawArcTo(const Vec3& vFromPos, const Vec3& vToPos, Color_t tColor, int iOffsetMultiplier, int iOffset, float flMaxDistance, float flMaxArcSize, float flMinArcSize)
{
	auto pLocal = H::Entities.GetLocal();
	if (!pLocal)
		return;

	Vec2 vCenter = { H::Draw.m_nScreenW / 2.f, H::Draw.m_nScreenH / 2.f };

	float flOffset = -iOffset - iOffsetMultiplier * H::Draw.Scale(10.f);
	float flRadius = H::Draw.Scale(50.f) + flOffset;

	float flDistanceScale = Math::RemapVal(vFromPos.DistTo(vToPos), flMaxDistance, 0.0f, flMinArcSize, flMaxArcSize);
	float flOffsetScale = 1.0f - std::clamp(flOffset / 10.f, 0.0f, 1.0f) * 0.5f;
	float flArcSize = std::clamp(flDistanceScale * flOffsetScale, flMinArcSize, flMaxArcSize);

	Vec3 vViewAngles = I::EngineClient->GetViewAngles();

	Vec3 vDelta = vToPos - vFromPos;
	Vec3 vDeltaAngles = Math::VectorAngles(vDelta);

	float flRelativeYaw = vViewAngles.y - vDeltaAngles.y;

	while (flRelativeYaw > 180.f) 
		flRelativeYaw -= 360.f;
	while (flRelativeYaw < -180.f) 
		flRelativeYaw += 360.f;

	float flScreenAngle = flRelativeYaw + 90.f;

	while (flScreenAngle < 0.f) 
		flScreenAngle += 360.f;
	while (flScreenAngle >= 360.f) 
		flScreenAngle -= 360.f;

	Arc(vCenter.x, vCenter.y, flRadius, H::Draw.Scale(6.f), flScreenAngle - flArcSize / 2.f, flArcSize, tColor);
}

static bool ArcsOverlap(float angle1, float arcSize1, float angle2, float arcSize2, float threshold = 0.f)
{
	auto normalizeAngle = [](float angle) {
		angle = fmodf(angle, 360.f);
		if (angle < 0) angle += 360.f;
		return angle;
		};

	angle1 = normalizeAngle(angle1);
	angle2 = normalizeAngle(angle2);

	float halfSize1 = (arcSize1 / 2.f) + threshold;
	float halfSize2 = (arcSize2 / 2.f) + threshold;

	auto angularDistance = [](float a1, float a2) {
		float diff = fabs(a1 - a2);
		if (diff > 180.f) diff = 360.f - diff;
		return diff;
		};


	float centerDistance = angularDistance(angle1, angle2);
	float combinedHalfSizes = halfSize1 + halfSize2;

	return centerDistance < combinedHalfSizes;
}

void COffscreenArrows::Draw(CTFPlayer* pLocal)
{
	if (m_mCache.empty())
		return;

	std::vector<std::pair<CBaseEntity*, ArrowCache_t>> sortedCache(m_mCache.begin(), m_mCache.end());
	std::sort(sortedCache.begin(), sortedCache.end(),
		[](const auto& a, const auto& b) {
			return a.second.m_flDistance < b.second.m_flDistance;
		});

	Vec3 vLocalPos = pLocal->GetEyePosition();
	Vec3 vViewAngles = I::EngineClient->GetViewAngles();

	struct ArcInfo {
		CBaseEntity* pEntity;
		ArrowCache_t* pCache;
		float flScreenAngle;
		float flArcSize;
		float flDistanceToEdge;
		int iOffsetMultiplier = 0;
	};

	std::vector<ArcInfo> arcInfos;
	arcInfos.reserve(sortedCache.size());

	const float flFadeDistance = H::Draw.Scale(150.f);

	for (auto& [pEntity, tCache] : sortedCache)
	{
		Vec3 vEntityCenter = pEntity->GetCenter();

		Vec3 vScreenPos;
		bool bOnScreen = SDK::W2S(vEntityCenter, vScreenPos, false);

		if (bOnScreen && vScreenPos.x >= 0 && vScreenPos.x <= H::Draw.m_nScreenW && vScreenPos.y >= 0 && vScreenPos.y <= H::Draw.m_nScreenH)
			continue;

		float flDistanceScale = Math::RemapVal(vLocalPos.DistTo(vEntityCenter), tCache.m_flMaxDistance, 0.f, 5.f, 70.f);
		float flArcSize = std::clamp(flDistanceScale, 5.f, 70.f);

		Vec3 vDelta = vEntityCenter - vLocalPos;
		Vec3 vDeltaAngles = Math::VectorAngles(vDelta);

		float flRelativeYaw = vDeltaAngles.y - vViewAngles.y;

		while (flRelativeYaw > 180.f) flRelativeYaw -= 360.f;
		while (flRelativeYaw < -180.f) flRelativeYaw += 360.f;

		float flScreenAngle = flRelativeYaw + 90.f;

		while (flScreenAngle < 0.f) flScreenAngle += 360.f;
		while (flScreenAngle >= 360.f) flScreenAngle -= 360.f;

		float flDistToEdge = GetDistanceToScreenEdge(flScreenAngle, H::Draw.m_nScreenW, H::Draw.m_nScreenH);

		arcInfos.push_back({ pEntity, &tCache, flScreenAngle, flArcSize, flDistToEdge, 0 });
	}

	for (size_t i = 0; i < arcInfos.size(); ++i)
	{
		for (size_t j = i + 1; j < arcInfos.size(); ++j)
		{
			if (ArcsOverlap(arcInfos[i].flScreenAngle, arcInfos[i].flArcSize, arcInfos[j].flScreenAngle, arcInfos[j].flArcSize))
				arcInfos[j].iOffsetMultiplier++;
		}
	}

	for (auto& arcInfo : arcInfos)
	{
		float flEdgeFade = std::clamp(arcInfo.flDistanceToEdge / flFadeDistance, 0.f, 1.f);

		Color_t tFadedColor = arcInfo.pCache->m_tColor;
		tFadedColor.a = static_cast<byte>(arcInfo.pCache->m_tColor.a * flEdgeFade);

		if (!tFadedColor.a)
			continue;

		DrawArcTo(vLocalPos, arcInfo.pEntity->GetCenter(), tFadedColor,
			arcInfo.iOffsetMultiplier, arcInfo.pCache->m_iOffset, arcInfo.pCache->m_flMaxDistance,
			70.f, 5.f);
	}
}