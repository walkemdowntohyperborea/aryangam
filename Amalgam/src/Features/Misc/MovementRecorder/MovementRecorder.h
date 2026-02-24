#pragma once

#include "../../../SDK/SDK.h"

#include <ostream>
#include <istream>

struct PlaybackCmd
{
	QAngle viewangles = {};
	Vec3 pos = {};
	float forwardmove = 0.0f;
	float sidemove = 0.0f;
	float upmove = 0.0f;
	int buttons = 0;
};

class CMovementRecorder
{
private:
	bool m_bRecord;
	bool m_bPlay;
	bool m_bCrossDist;
	bool m_bAimToFirstRecord;
	bool m_bRecordAfterDonePlaying;

	Vec3 m_vViewPos;
	Vec3 m_vLastPos;

	bool m_bIsPlayingBack;

	std::vector<PlaybackCmd> m_vCmds;

	void WriteVec(std::ostream& os, const std::vector<PlaybackCmd>& vCmds);
	void ReadVec(std::istream& is, std::vector<PlaybackCmd>& vCmds);
	
	void SaveToFile();
	void ReadFromFile();

	void Record(CTFPlayer* pLocal, CUserCmd* pCmd);
	void Play(CTFPlayer* pLocal, CUserCmd* pCmd);

public:
	void CreateMove(CTFPlayer* pLocal, CUserCmd* pCmd);
};

ADD_FEATURE(CMovementRecorder, MovementRecorder);