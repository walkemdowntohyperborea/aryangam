#pragma once
#include "../SDK/SDK.h"

#include "../Features/Misc/AntiAutobalance/AntiAutobalance.h"

#include <functional>
#include <Windows.h>
#include <SafetyHook/safetyhook.hpp>

#define CREATE_HOOK(name) safetyhook::InlineHook m_##name
#define INIT_HOOK(name, addr) m_##name## = safetyhook::create_inline(addr, name)
#define UNLOAD_HOOK(name) m_##name## = {}

//#define SEEDPRED_DEBUG

namespace WndProc
{
	inline HWND hwWindow;
	inline WNDPROC Original;
	LONG __stdcall Func(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize();
	void Unload();
}

class CHooks
{
public:
	bool Initialize();
	bool Unload();

	CREATE_HOOK(bf_read_ReadString);
	CREATE_HOOK(CAchievementMgr_CheckAchievementsEnabled);
	CREATE_HOOK(CAttributeManager_AttribHookInt);
	CREATE_HOOK(CBaseAnimating_Interpolate);
	CREATE_HOOK(CBaseAnimating_MaintainSequenceTransitions);
	CREATE_HOOK(CBaseAnimating_SetSequence);
	CREATE_HOOK(CBaseAnimating_SetupBones);
	CREATE_HOOK(CBaseAnimating_UpdateClientSideAnimation);
	CREATE_HOOK(CBaseEntity_AddVar);
	CREATE_HOOK(CBaseEntity_BaseInterpolatePart1);
	CREATE_HOOK(CBaseEntity_EstimateAbsVelocity);
	CREATE_HOOK(CBaseEntity_InterpolateServerEntities);
	CREATE_HOOK(CBaseEntity_ResetLatched);
	CREATE_HOOK(CBaseEntity_SetAbsVelocity);
	CREATE_HOOK(CBaseEntity_WorldSpaceCenter);
	//CREATE_HOOK(CBaseHudChat_StartMessageMode);
	CREATE_HOOK(CBaseHudChatLine_InsertAndColorizeText);
	CREATE_HOOK(CBasePlayer_CalcObserverView);
	CREATE_HOOK(CBasePlayer_CalcView);
	CREATE_HOOK(CTFPlayer_HandleTaunting);
	CREATE_HOOK(CThirdPersonManager_GetFinalCameraOffset);
	CREATE_HOOK(CBaseViewModel_CalcViewModelView);
	CREATE_HOOK(CBasePlayer_CalcViewModelView);
	CREATE_HOOK(CBasePlayer_ItemPostFrame);
	CREATE_HOOK(CBaseViewModel_ShouldFlipViewModel);
	CREATE_HOOK(Cbuf_ExecuteCommand);
	CREATE_HOOK(CClientModeShared_DoPostScreenSpaceEffects);
	CREATE_HOOK(CClientModeShared_OverrideView);
	CREATE_HOOK(CClientModeShared_ShouldDrawViewModel);
	CREATE_HOOK(CClientState_GetClientInterpAmount);
	CREATE_HOOK(CClientState_ProcessFixAngle);
	CREATE_HOOK(CGlowObjectManager_RenderGlowEffects);
	CREATE_HOOK(CHLClient_CreateMove);
	CREATE_HOOK(CHLClient_DispatchUserMessage);
	CREATE_HOOK(CHLClient_FrameStageNotify);
	CREATE_HOOK(CHLClient_LevelShutdown);
	CREATE_HOOK(CHLTVCamera_CalcView);
	CREATE_HOOK(CHLTVCamera_GetPrimaryTarget);
	CREATE_HOOK(CHLTVCamera_GetMode);
	CREATE_HOOK(CHudChat_GetClientColor);
	CREATE_HOOK(CHudCrosshair_GetDrawPosition);
	CREATE_HOOK(CInput_GetUserCmd);
	CREATE_HOOK(CInput_ValidateUserCmd);
	CREATE_HOOK(CInventoryManager_ShowItemsPickedUp);
	CREATE_HOOK(CL_CheckForPureServerWhitelist);
	CREATE_HOOK(CL_Move);
	CREATE_HOOK(CL_ProcessPacketEntities);
	CREATE_HOOK(CL_ReadPackets);
	CREATE_HOOK(ClientModeTFNormal_BIsFriendOrPartyMember);
	CREATE_HOOK(CMatchInviteNotification_OnTick);
	CREATE_HOOK(CMaterial_Uncache);
	CREATE_HOOK(CNetChannel_SendDatagram);
	CREATE_HOOK(CNetChannel_SendNetMsg);
	CREATE_HOOK(COPRenderSprites_Render);
	CREATE_HOOK(CNewParticleEffect_DrawModel);
	CREATE_HOOK(CNewParticleEffect_Deconstructor);
	CREATE_HOOK(COPRenderSprites_RenderSpriteCard);
	CREATE_HOOK(COPRenderSprites_RenderTwoSequenceSpriteCard);
	CREATE_HOOK(CParticleCollection_Render);
	CREATE_HOOK(CParticleProperty_Create_Name);
	CREATE_HOOK(CParticleProperty_Create_Point);
	CREATE_HOOK(CParticleProperty_AddControlPoint_Pointer);
	//CREATE_HOOK(CPhysicsObject_OutputDebugInfo);
	CREATE_HOOK(CPlayerResource_GetPlayerName);
#ifdef ANTIAUTOBALANCETESTING
	CREATE_HOOK(CPrediction_PostEntityPacketReceived);
#endif
	CREATE_HOOK(CPrediction_RunSimulation);
	CREATE_HOOK(CProxyAnimatedWeaponSheen_OnBind);
	CREATE_HOOK(CRendering3dView_EnableWorldFog);
	CREATE_HOOK(CSequenceTransitioner_CheckForSequenceChange);
	CREATE_HOOK(CSkyboxView_Enable3dSkyboxFog);
	CREATE_HOOK(CSniperDot_ClientThink);
	CREATE_HOOK(CSniperDot_GetRenderingPositions);
	CREATE_HOOK(CBasePlayer_EyePosition);
	CREATE_HOOK(CTFPlayer_EyeAngles);
	CREATE_HOOK(CSoundEmitterSystem_EmitSound);
	CREATE_HOOK(CBaseEntity_EmitSound);
	//CREATE_HOOK(S_StartDynamicSound);
	CREATE_HOOK(S_StartSound);
	//CREATE_HOOK(CSpriteTrail_DrawModel);
	CREATE_HOOK(CStaticPropMgr_ComputePropOpacity);
	CREATE_HOOK(CStaticPropMgr_DrawStaticProps);
	CREATE_HOOK(CStudioRender_SetColorModulation);
	CREATE_HOOK(CStudioRender_SetAlphaModulation);
	//CREATE_HOOK(CStudioRender_DrawModelStaticProp);
	CREATE_HOOK(CTFBadgePanel_SetupBadge);
	CREATE_HOOK(CTFClientScoreBoardDialog_UpdatePlayerAvatar);
	CREATE_HOOK(CTFMatchSummary_UpdatePlayerAvatar);
	CREATE_HOOK(CTFHudMannVsMachineScoreboard_UpdatePlayerAvatar);
	CREATE_HOOK(CTFHudMatchStatus_UpdatePlayerAvatar);
	CREATE_HOOK(SectionedListPanel_SetItemFgColor);
	CREATE_HOOK(CTFGCClientSystem_UpdateAssignedLobby);
	CREATE_HOOK(CTFInput_ApplyMouse);
	CREATE_HOOK(CTFInput_CAM_CapYaw);
	CREATE_HOOK(CTFPlayer_AvoidPlayers);
	CREATE_HOOK(CTFPlayer_BRenderAsZombie);
	CREATE_HOOK(CTFPlayer_BuildTransformations);
	CREATE_HOOK(CTFPlayer_ClientAdjustVOPitch);
	CREATE_HOOK(CTFPlayer_DoAnimationEvent);
	CREATE_HOOK(CTFPlayer_FireBullet);
	CREATE_HOOK(CTFPlayer_GetMinFOV);
	CREATE_HOOK(CTFPlayer_InSameDisguisedTeam);
	CREATE_HOOK(CTFFreezePanel_ShouldDraw);
	CREATE_HOOK(CTFFreezePanel_FireGameEvent);
	CREATE_HOOK(CTFPlayer_IsPlayerClass);
	CREATE_HOOK(CTFPlayer_ShouldDraw);
	CREATE_HOOK(CBasePlayer_ShouldDrawThisPlayer);
	CREATE_HOOK(CBasePlayer_ShouldDrawLocalPlayer);
	CREATE_HOOK(CBaseCombatWeapon_ShouldDraw);
	CREATE_HOOK(CViewRender_DrawViewModels);
	CREATE_HOOK(CTFPlayer_UpdateStepSound);
	CREATE_HOOK(CTFPlayerInventory_GetMaxItemCount);
	CREATE_HOOK(CTFPlayerInventory_VerifyChangedLoadoutsAreValid);
	CREATE_HOOK(GenerateEquipRegionConflictMask);
	CREATE_HOOK(CTFInventoryManager_GetItemInLoadoutForClass);
	CREATE_HOOK(CTFPlayerPanel_GetTeam);
	CREATE_HOOK(CTFTeamStatusPlayerPanel_Update);
	CREATE_HOOK(VGui_Panel_SetFgColor);
	CREATE_HOOK(VGui_Panel_SetBgColor);
	CREATE_HOOK(CTFPlayerShared_InCond);
	CREATE_HOOK(CTFPlayerShared_IsCritBoosted);
	CREATE_HOOK(CTFConditionList_InCond);
	CREATE_HOOK(CTFPlayerShared_IsPlayerDominated);
	CREATE_HOOK(CTFPlayerShared_ShouldSuppressPrediction);
	CREATE_HOOK(CTFRagdoll_CreateTFRagdoll);
	CREATE_HOOK(CTFRocketLauncher_CheckReloadMisfire);
	CREATE_HOOK(CTFRocketLauncher_FireProjectile);
	//CREATE_HOOK(CTFBat_Wood_LaunchBall);
	CREATE_HOOK(CBaseEntity_ApplyAbsVelocityImpulse);
	CREATE_HOOK(CTFScattergun_FireBullet);
	CREATE_HOOK(CTFGameMovement_SetGroundEntity);
	CREATE_HOOK(CTFWeaponBase_CalcIsAttackCritical);
	CREATE_HOOK(CTFWeaponBase_CanFireRandomCriticalShot);
	CREATE_HOOK(CTFWeaponBase_GetShootSound);
	CREATE_HOOK(CThirdPersonManager_Update);
	CREATE_HOOK(CViewRender_DrawUnderwaterOverlay);
	CREATE_HOOK(CViewRender_LevelInit);
	CREATE_HOOK(CViewRender_PerformScreenOverlay);
	CREATE_HOOK(CViewRender_RenderView);
	CREATE_HOOK(CWeaponMedigun_PrimaryAttack);
	CREATE_HOOK(DataTable_Warning);
	CREATE_HOOK(Direct3DDevice9_Present);
	CREATE_HOOK(Direct3DDevice9_Reset);
	CREATE_HOOK(VGuiSurface_LockCursor);
	CREATE_HOOK(VGuiSurface_SetCursor);
	CREATE_HOOK(DoEnginePostProcessing);
	CREATE_HOOK(DSP_Process);
	CREATE_HOOK(FX_FireBullets);
#ifdef SEEDPRED_DEBUG
	CREATE_HOOK(FX_FireBullets_Server);
	CREATE_HOOK(CBasePlayer_ProcessUsercmds);
#endif
	CREATE_HOOK(GetClientInterpAmount);
	CREATE_HOOK(HostState_Shutdown);
	CREATE_HOOK(HostState_Restart);
	CREATE_HOOK(IEngineTrace_TraceRay);
	CREATE_HOOK(CM_BoxTrace);
	CREATE_HOOK(CM_ClipBoxToBrush_True);
	CREATE_HOOK(CM_ClipBoxToBrush_False);
	CREATE_HOOK(CEngineTrace_ClipTraceToTrace);
	CREATE_HOOK(IEngineVGui_Paint);
	CREATE_HOOK(IMaterialSystem_FindTexture);
	CREATE_HOOK(IMatSystemSurface_OnScreenSizeChanged);
	CREATE_HOOK(IPanel_PaintTraverse);
	CREATE_HOOK(ISteamFriends_GetFriendPersonaName);
	CREATE_HOOK(ISteamNetworkingUtils_GetPingToDataCenter);
	CREATE_HOOK(CTFPartyClient_RequestQueueForMatch);
	CREATE_HOOK(IVModelRender_DrawModelExecute);
	CREATE_HOOK(CBaseAnimating_DrawModel);
	CREATE_HOOK(CBaseAnimating_InternalDrawModel);
	CREATE_HOOK(IVModelRender_ForcedMaterialOverride);
	CREATE_HOOK(KeyValues_SetInt);
	CREATE_HOOK(NotificationQueue_Add);
	CREATE_HOOK(R_ComputeLightingOrigin);
	CREATE_HOOK(R_DrawSkyBox);
	CREATE_HOOK(RecvProxy_SimulationTime);
	CREATE_HOOK(TF_IsHolidayActive);
	CREATE_HOOK(CPlayerResource_IsFakePlayer);
	CREATE_HOOK(VGuiMenuBuilder_AddMenuItem);
	CREATE_HOOK(CTFClientScoreBoardDialog_OnCommand);
};

ADD_FEATURE_CUSTOM(CHooks, Hooks, U);

#define CALL_ORIGINAL(hook, type, ...) U::Hooks.m_##hook##.fastcall<type>(__VA_ARGS__)