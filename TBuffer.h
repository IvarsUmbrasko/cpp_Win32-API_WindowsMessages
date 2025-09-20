#pragma once
#include <windows.h>

#define BUFFER_SIZE 100

enum TProcessState { psEmpty, psNew, psRunning, psTerminated, psError };

struct TProcessInfo {
	TProcessState State;
	char Name[100];
	HANDLE Handle;
	int PID;
	int UserTime, KernelTime;
};

class TBuffer
{
private:
	HWND Wnd;
	CRITICAL_SECTION cs;
	TProcessInfo Buf[BUFFER_SIZE];

public:
	int Count();
	int AddProcess(char* AName);
	void Get(int Id, TProcessInfo &Pi);
	void Set(int Id, const TProcessInfo Pi);
	TBuffer(HWND AWnd);
	~TBuffer();
};