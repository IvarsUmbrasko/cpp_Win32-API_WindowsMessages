#include "TBuffer.h" 
#include <cstdio>
#include <windows.h>

TBuffer::TBuffer(HWND AWnd) {
	InitializeCriticalSection(&cs);
	Wnd = AWnd;
	for (int i = 0; i < BUFFER_SIZE; i++) {
		Buf[i].State = psEmpty;
		Buf[i].Name[0] = '\0';
	}
}

TBuffer::~TBuffer() {
	DeleteCriticalSection(&cs);
}

int TBuffer::Count() {
	EnterCriticalSection(&cs);
	int count = 0;
	for (int i = 0; i < BUFFER_SIZE; i++) {
		if (Buf[i].State != psEmpty) {
			count++;
		}
	}
	LeaveCriticalSection(&cs);
	return count;
}

int TBuffer::AddProcess(char* AName) {
	EnterCriticalSection(&cs);
	int pos = -1;
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		if (Buf[i].State == psEmpty) {
			int j = 0;
			while (AName[j] != '\0' && j < 99) {
				Buf[i].Name[j] = AName[j];
				j++;
			}
			Buf[i].Name[j] = '\0';
			Buf[i].State = psNew;
			pos = i;
			break;
		}
	}
	PostMessage(Wnd, WM_USER, pos, 0);
	LeaveCriticalSection(&cs);
	return pos;
}

void TBuffer::Get(int Id, TProcessInfo& Pi) {
	EnterCriticalSection(&cs);
	Pi = Buf[Id];
	LeaveCriticalSection(&cs);
}

void TBuffer::Set(int Id, const TProcessInfo Pi) {
	EnterCriticalSection(&cs);
	Buf[Id] = Pi;
	PostMessage(Wnd, WM_USER, Id, 0);
	LeaveCriticalSection(&cs);
}
