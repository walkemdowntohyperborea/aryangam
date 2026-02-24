#include "MovementRecorder.h"

#include <regex>
#include <fstream>

void CMovementRecorder::WriteVec(std::ostream& os, const std::vector<PlaybackCmd>& vCmds)
{
	typename std::vector<PlaybackCmd>::size_type size = vCmds.size();
	os.write((char*)&size, sizeof(size));
	os.write((char*)&size, vCmds.size() * sizeof(PlaybackCmd));
}

void CMovementRecorder::ReadVec(std::istream& is, std::vector<PlaybackCmd>& vCmds)
{
	typename std::vector<PlaybackCmd>::size_type size = 0;
	is.read((char*)&size, sizeof(size));
	vCmds.resize(size);
	is.read((char*)&vCmds[0], vCmds.size() * sizeof(PlaybackCmd));
}

void CMovementRecorder::SaveToFile()
{
	std::string sMapName = SDK::GetLevelName();
	std::string temp = sMapName;
	std::string sResult = "test/";

	std::regex re("\/(?!.*\/)(.*)");
	std::smatch match;

	if (std::regex_search(temp, match, re) && !m_vCmds.empty())
	{
		sResult += match.str(1);
		sResult += ".bin";
		std::ofstream out(sResult, std::ios::out | std::ios::binary);
		WriteVec(out, m_vCmds);
		out.close();
	}
}

void CMovementRecorder::ReadFromFile()
{
	std::string sMapName = SDK::GetLevelName();

	m_vCmds = {};

	
}

void CMovementRecorder::Record(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	//if(!Vars::Misc::MovementRecorder::Record.Value)
	//	return;

	static bool bTemp = false;
	if (m_bRecord)
	{
		PlaybackCmd temp = {};
		temp.buttons = pCmd->buttons;
		temp.forwardmove = pCmd->forwardmove;
		temp.sidemove = pCmd->sidemove;
		temp.upmove = pCmd->upmove;
		temp.viewangles = pCmd->viewangles;
		temp.pos = pLocal->m_vecOrigin();

		if (bTemp)
			m_vCmds = {};
		m_vCmds.push_back(temp);
		bTemp = false;
	}
	else
		bTemp = true;
}

static inline float DistanceBetweenCross(float x, float y)
{
	float flYDistance = (y - H::Draw.m_nScreenH / 2.f);
	float flXDistance = (x - H::Draw.m_nScreenW / 2.f);
	return sqrt(pow(flYDistance, 2) + pow(flXDistance, 2));
}

void CMovementRecorder::Play(CTFPlayer* pLocal, CUserCmd* pCmd)
{
	//if(!Vars::Misc::MovementRecorder::Record.Value)
	//	return;

	if (m_vCmds.empty())
		return;

	if (m_bPlay)
	{
		if (!m_bIsPlayingBack)
		{
			float flDistance = pLocal->m_vecOrigin().DistTo(m_vCmds[0].pos);

			if (flDistance <= 1.f)
				m_bIsPlayingBack = true;
			else
			{
				if (!m_bCrossDist)
				{
					Vec3 vViewPos = m_vCmds[0].pos;
					vViewPos.z += 64.f;

					Vec3 vTemp;
					float flCrossDist = 0.f;

					if (SDK::W2S(vViewPos, vTemp))
						flCrossDist = DistanceBetweenCross(vTemp.x, vTemp.y);
					else
						flCrossDist = 10000.f;

					if (flCrossDist <= 1.f)
						m_bCrossDist = true;
				}
				else
				{
					Vec3 vFinal = m_vCmds[0].pos - pLocal->m_vecOrigin();

					Vec3 vWishAngle; Math::VectorAngles(vFinal, vWishAngle);
					
					I::EngineClient->SetViewAngles(vWishAngle);
				}
			}
		}
	}
}