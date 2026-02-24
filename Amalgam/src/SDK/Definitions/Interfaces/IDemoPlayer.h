#pragma once
#include "Interface.h"

class CDemoFile;
struct netpacket_t;

class IDemoPlayer
{
public:
	virtual ~IDemoPlayer() = 0;
	virtual CDemoFile* GetDemoFile() = 0;
	virtual int GetPlaybackStartTick() = 0;
	virtual int GetPlaybackTick() = 0;
	virtual int GetTotalTicks() = 0;
	virtual bool StartPlayback(const char* filename, bool bAsTimeDemo) = 0;
	virtual bool IsPlayingBack() = 0;
	virtual bool IsPlaybackPaused() = 0;
	virtual bool IsPlayingTimeDemo() = 0;
	virtual bool IsSkipping() = 0;
	virtual bool CanSkipBackwards() = 0;
	virtual void SetPlaybackTimeScale(float timescale) = 0;
	virtual void GetPlaybackTimescale() = 0;
	virtual void PausePlayback(float seconds) = 0;
	virtual void SkipToTick(int tick, bool bRelative, bool bPause) = 0;
	virtual void SetEndTick(int tick) = 0;
	virtual void ResumePlayback() = 0;
	virtual void StopPlayback() = 0;
	virtual void InterpolateViewpoint() = 0;
	virtual netpacket_t* ReadPacket() = 0;
	virtual void ResetDemoInterpolation() = 0;
	virtual int GetProtocolVersion() = 0;
	virtual bool ShouldLoopDemos() = 0;
	virtual void OnLastDemoInLoopPlayed() = 0;
	virtual bool IsLoading() = 0;
};

MAKE_INTERFACE_SIGNATURE(IDemoPlayer, DemoPlayer, "engine.dll", "48 8B 0D ? ? ? ? 40 B7", 0x0, 1);