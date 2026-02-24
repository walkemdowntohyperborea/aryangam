#pragma once
#include "CBaseHandle.h"
#include "CBaseEntity.h"
#include "../Misc/refcount.h"

static const int GLOW_FOR_ALL_SPLIT_SCREEN_SLOTS = -1;
class CGlowObjectManager
{
public:
	CGlowObjectManager() :
		m_nFirstFreeSlot(GlowObjectDefinition_t::END_OF_FREE_LIST)
	{
	}

	int RegisterGlowObject(CBaseEntity* pEntity, const Vec3& vGlowColor, float flGlowAlpha, bool bRenderWhenOccluded, bool bRenderWhenUnoccluded, int nSplitScreenSlot)
	{
		int nIndex;
		if (m_nFirstFreeSlot == GlowObjectDefinition_t::END_OF_FREE_LIST)
			nIndex = m_GlowObjectDefinitions.AddToTail();
		else
		{
			nIndex = m_nFirstFreeSlot;
			m_nFirstFreeSlot = m_GlowObjectDefinitions[nIndex].m_nNextFreeSlot;
		}

		m_GlowObjectDefinitions[nIndex].m_hEntity = pEntity;
		m_GlowObjectDefinitions[nIndex].m_vGlowColor = vGlowColor;
		m_GlowObjectDefinitions[nIndex].m_flGlowAlpha = flGlowAlpha;
		m_GlowObjectDefinitions[nIndex].m_bRenderWhenOccluded = bRenderWhenOccluded;
		m_GlowObjectDefinitions[nIndex].m_bRenderWhenUnoccluded = bRenderWhenUnoccluded;
		m_GlowObjectDefinitions[nIndex].m_nSplitScreenSlot = nSplitScreenSlot;
		m_GlowObjectDefinitions[nIndex].m_nNextFreeSlot = GlowObjectDefinition_t::ENTRY_IN_USE;

		return nIndex;
	}

	void UnregisterGlowObject(int nGlowObjectHandle)
	{
		m_GlowObjectDefinitions[nGlowObjectHandle].m_nNextFreeSlot = m_nFirstFreeSlot;
		m_GlowObjectDefinitions[nGlowObjectHandle].m_hEntity = NULL;
		m_nFirstFreeSlot = nGlowObjectHandle;
	}

	void SetEntity(int nGlowObjectHandle, CBaseEntity* pEntity)
	{
		m_GlowObjectDefinitions[nGlowObjectHandle].m_hEntity = pEntity;
	}

	void SetColor(int nGlowObjectHandle, const Vector& vGlowColor)
	{
		m_GlowObjectDefinitions[nGlowObjectHandle].m_vGlowColor = vGlowColor;
	}

	void SetAlpha(int nGlowObjectHandle, float flAlpha)
	{
		m_GlowObjectDefinitions[nGlowObjectHandle].m_flGlowAlpha = flAlpha;
	}

	void SetRenderFlags(int nGlowObjectHandle, bool bRenderWhenOccluded, bool bRenderWhenUnoccluded)
	{
		m_GlowObjectDefinitions[nGlowObjectHandle].m_bRenderWhenOccluded = bRenderWhenOccluded;
		m_GlowObjectDefinitions[nGlowObjectHandle].m_bRenderWhenUnoccluded = bRenderWhenUnoccluded;
	}

	bool IsRenderingWhenOccluded(int nGlowObjectHandle) const
	{
		return m_GlowObjectDefinitions[nGlowObjectHandle].m_bRenderWhenOccluded;
	}

	bool IsRenderingWhenUnoccluded(int nGlowObjectHandle) const
	{
		return m_GlowObjectDefinitions[nGlowObjectHandle].m_bRenderWhenUnoccluded;
	}

	bool HasGlowEffect(CBaseEntity* pEntity) const
	{
		for (int i = 0; i < m_GlowObjectDefinitions.Count(); ++i)
		{
			if (!m_GlowObjectDefinitions[i].IsUnused() && m_GlowObjectDefinitions[i].m_hEntity.Get() == pEntity)
				return true;
		}

		return false;
	}

	struct GlowObjectDefinition_t
	{
		bool ShouldDraw(int nSlot) const
		{
			return m_hEntity.Get() &&
				(m_nSplitScreenSlot == GLOW_FOR_ALL_SPLIT_SCREEN_SLOTS || m_nSplitScreenSlot == nSlot) &&
				(m_bRenderWhenOccluded || m_bRenderWhenUnoccluded) &&
				m_hEntity->ShouldDraw() &&
				!m_hEntity->IsDormant();
		}

		bool IsUnused() const { return m_nNextFreeSlot != GlowObjectDefinition_t::ENTRY_IN_USE; }

		EHANDLE m_hEntity;
		Vec3 m_vGlowColor;
		float m_flGlowAlpha;

		bool m_bRenderWhenOccluded;
		bool m_bRenderWhenUnoccluded;
		int m_nSplitScreenSlot;

		// Linked list of free slots
		int m_nNextFreeSlot;

		// Special values for GlowObjectDefinition_t::m_nNextFreeSlot
		static const int END_OF_FREE_LIST = -1;
		static const int ENTRY_IN_USE = -2;
	};

	CUtlVector<GlowObjectDefinition_t> m_GlowObjectDefinitions;
	int m_nFirstFreeSlot;
};

class CGlowObject
{
public:
	CGlowObject(CGlowObjectManager* pGlowManager, CBaseEntity* pEntity, const Vec3& vGlowColor = Vec3(1.0f, 1.0f, 1.0f), float flGlowAlpha = 1.0f, bool bRenderWhenOccluded = false, bool bRenderWhenUnoccluded = false, int nSplitScreenSlot = GLOW_FOR_ALL_SPLIT_SCREEN_SLOTS)
	{
		m_pGlowManager = pGlowManager;
		m_nGlowObjectHandle = m_pGlowManager->RegisterGlowObject(pEntity, vGlowColor, flGlowAlpha, bRenderWhenOccluded, bRenderWhenUnoccluded, nSplitScreenSlot);
	}

	~CGlowObject()
	{
		m_pGlowManager->UnregisterGlowObject(m_nGlowObjectHandle);
		m_pGlowManager = nullptr;
	}

	void SetEntity(CBaseEntity* pEntity)
	{
		m_pGlowManager->SetEntity(m_nGlowObjectHandle, pEntity);
	}

	void SetColor(const Vector& vGlowColor)
	{
		m_pGlowManager->SetColor(m_nGlowObjectHandle, vGlowColor);
	}

	void SetAlpha(float flAlpha)
	{
		m_pGlowManager->SetAlpha(m_nGlowObjectHandle, flAlpha);
	}

	void SetRenderFlags(bool bRenderWhenOccluded, bool bRenderWhenUnoccluded)
	{
		m_pGlowManager->SetRenderFlags(m_nGlowObjectHandle, bRenderWhenOccluded, bRenderWhenUnoccluded);
	}

	bool IsRenderingWhenOccluded() const
	{
		return m_pGlowManager->IsRenderingWhenOccluded(m_nGlowObjectHandle);
	}

	bool IsRenderingWhenUnoccluded() const
	{
		return m_pGlowManager->IsRenderingWhenUnoccluded(m_nGlowObjectHandle);
	}

	bool IsRendering() const
	{
		return IsRenderingWhenOccluded() || IsRenderingWhenUnoccluded();
	}

	// Add more accessors/mutators here as needed

private:
	int m_nGlowObjectHandle;
	CGlowObjectManager* m_pGlowManager; // this memory is static

	// Assignment & copy-construction disallowed
	CGlowObject(const CGlowObject& other);
	CGlowObject& operator=(const CGlowObject& other);
};

class CMatRenderContextPtr : public CRefPtr<IMatRenderContext>
{
	typedef CRefPtr<IMatRenderContext> BaseClass;
public:
	CMatRenderContextPtr() {}
	CMatRenderContextPtr(IMatRenderContext* pInit) : BaseClass(pInit) { if (BaseClass::m_pObject) BaseClass::m_pObject->BeginRender(); }
	CMatRenderContextPtr(IMaterialSystem* pFrom) : BaseClass(pFrom->GetRenderContext()) { if (BaseClass::m_pObject) BaseClass::m_pObject->BeginRender(); }
	~CMatRenderContextPtr() { if (BaseClass::m_pObject) BaseClass::m_pObject->EndRender(); }

	IMatRenderContext* operator=(IMatRenderContext* p) { if (p) p->BeginRender(); return BaseClass::operator=(p); }

	void SafeRelease() { if (BaseClass::m_pObject) BaseClass::m_pObject->EndRender(); BaseClass::SafeRelease(); }
	void AssignAddRef(IMatRenderContext* pFrom) { if (BaseClass::m_pObject) BaseClass::m_pObject->EndRender(); BaseClass::AssignAddRef(pFrom); BaseClass::m_pObject->BeginRender(); }

	void GetFrom(IMaterialSystem* pFrom) { AssignAddRef(pFrom->GetRenderContext()); }


private:
	CMatRenderContextPtr(const CMatRenderContextPtr& from);
	void operator=(const CMatRenderContextPtr& from);

};