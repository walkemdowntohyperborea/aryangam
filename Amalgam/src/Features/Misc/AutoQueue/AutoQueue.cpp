#include "AutoQueue.h"

void CAutoQueue::Run()
{
	if (Vars::Misc::Queueing::AutoCasualQueue.Value && !I::TFPartyClient->BInQueueForMatchGroup(k_eTFMatchGroup_Casual_Default))
	{
		if (I::EngineClient->IsInGame())
			return;

		static bool bHasLoaded = false;
		if (!bHasLoaded)
		{
			I::TFPartyClient->LoadSavedCasualCriteria();
			bHasLoaded = true;
		}

		static Timer timer = {};
		if(timer.Run(1.f))
			I::TFPartyClient->RequestQueueForMatch(k_eTFMatchGroup_Casual_Default);
	}
}