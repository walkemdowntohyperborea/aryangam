#pragma once
#include "../SDK/SDK.h"
#include "../SDK/Definitions/Misc/UtlObjectReference.h"
#include <unordered_set>

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