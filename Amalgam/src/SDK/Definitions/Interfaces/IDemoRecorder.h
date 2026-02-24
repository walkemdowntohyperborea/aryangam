#pragma once
#include "Interface.h"

class CDemoFile;
class bf_read;
class ServerClass;

class IDemoRecorder
{
public:
	virtual CDemoFile* GetDemoFile() = 0;
	virtual int	GetRecordingTick(void) = 0;
	virtual void StartRecording(const char* filename, bool bContinuously) = 0;
	virtual void SetSignonState(int state) = 0;
	virtual bool IsRecording(void) = 0;
	virtual void PauseRecording(void) = 0;
	virtual void ResumeRecording(void) = 0;
	virtual void StopRecording(void) = 0;
	virtual void RecordCommand(const char* cmdstring) = 0;  // record a console command
	virtual void RecordUserInput(int cmdnumber) = 0;  // record a user input command
	virtual void RecordMessages(bf_read& data, int bits) = 0; // add messages to current packet
	virtual void RecordPacket(void) = 0; // packet finished, write all recorded stuff to file
	virtual void RecordServerClasses(ServerClass* pClasses) = 0; // packet finished, write all recorded stuff to file
	virtual void RecordStringTables() = 0;
	virtual void ResetDemoInterpolation() = 0;
};

MAKE_INTERFACE_SIGNATURE(IDemoRecorder, DemoRecorder, "engine.dll", "48 8B 0D ? ? ? ? 8D 56", 0x0, 1);