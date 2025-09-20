#undef UNICODE
#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#include "resource.h"
#include "TBuffer.h"
#define WM_UPDATELIST WM_USER

#pragma warning(disable : 4996)

TBuffer* Buffer;
bool Terminate = false;
HWND hMain;
HICON hIcon;

BOOL CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
	hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_OS), IMAGE_ICON, 32, 32, 0);
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAINDIALOG), NULL, DLGPROC(MainWndProc));
	return 0;
}

bool BrowseFileName(HWND Wnd, char* FileName) {
	OPENFILENAME ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = Wnd;
	ofn.lpstrFilter = ("All\0*.*\0");
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.lpstrDefExt = "exe";
	return GetOpenFileName(&ofn);
}

void CreateProcess(int pos, TProcessInfo& process) {
	char buf[100];

	int i = 0;
	while (i < 99 && process.Name[i] != '\0') {
		buf[i] = process.Name[i];
		i++;
	}
	buf[i] = '\0';

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	if (!CreateProcess(NULL, buf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		process.State = psError;
		Buffer->Set(pos, process);
	}
	else {
		process.State = psRunning;
		process.Handle = pi.hProcess;
		process.PID = pi.dwProcessId;
		Buffer->Set(pos, process);
		CloseHandle(pi.hThread);
	}
}

DWORD FileTimeToMilliseconds(FILETIME ft) {
	ULARGE_INTEGER i;
	i.LowPart = ft.dwLowDateTime;
	i.HighPart = ft.dwHighDateTime;
	return i.QuadPart / 10000;
}

DWORD WINAPI ProcessThread(void* APos) {
	int pos = (int)APos;

	TProcessInfo process;
	Buffer->Get(pos, process);
	CreateProcess(pos, process);

	DWORD exitCode = 0;
	while (!Terminate) {
		if (GetExitCodeProcess(process.Handle, &exitCode)) {
			if (exitCode != STILL_ACTIVE) {
				process.State = psTerminated;
				CloseHandle(process.Handle);
				Buffer->Set(pos, process);
				break;
			}
			else {
				FILETIME startTime, endTime, kernel, user;

				if (GetProcessTimes(process.Handle, &startTime, &endTime, &kernel, &user)) {
					process.UserTime = FileTimeToMilliseconds(user);
					process.KernelTime = FileTimeToMilliseconds(kernel);
					Buffer->Set(pos, process);
				}
			}
		}
		Sleep(100);
	}
	return 0;
}

void UpdateButtons(HWND hWnd) {
	int row = SendDlgItemMessage(hWnd, IDC_LIST, LVM_GETNEXTITEM, -1, LVNI_SELECTED);

	if (row == -1) {
		EnableWindow(GetDlgItem(hWnd, IDC_KILLBTN), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_DELETEBTN), FALSE);
	}
	else {
		LVITEM item = { 0 };
		item.mask = LVIF_PARAM;
		item.iItem = row;
		SendDlgItemMessage(hWnd, IDC_LIST, LVM_GETITEM, 0, (LPARAM)&item);
		int pos = (int)item.lParam;

		TProcessInfo process;
		Buffer->Get(pos, process);

		if (process.State == psRunning) {
			EnableWindow(GetDlgItem(hWnd, IDC_KILLBTN), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDC_DELETEBTN), FALSE);
		}
		else {
			EnableWindow(GetDlgItem(hWnd, IDC_KILLBTN), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_DELETEBTN), TRUE);
		}
	}
}

BOOL CALLBACK MainWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	switch (Msg) {
	case WM_INITDIALOG:
		Buffer = new TBuffer(hWnd);
		hMain = hWnd;
		SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		{
			LVITEM item = { 0 };
			item.mask = LVIF_PARAM;
			item.iItem = 0;
			item.lParam = 0;
			SendDlgItemMessage(hWnd, IDC_LIST, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

			LVCOLUMN col = { 0 };
			col.mask = LVCF_TEXT | LVCF_WIDTH;
			col.pszText = (LPSTR)"State";
			col.cx = 100;
			SendDlgItemMessage(hWnd, IDC_LIST, LVM_INSERTCOLUMN, 0, (LPARAM)&col);

			col.pszText = (LPSTR)"Name";
			col.cx = 100;
			SendDlgItemMessage(hWnd, IDC_LIST, LVM_INSERTCOLUMN, 1, (LPARAM)&col);

			col.pszText = (LPSTR)"PID";
			col.cx = 100;
			SendDlgItemMessage(hWnd, IDC_LIST, LVM_INSERTCOLUMN, 2, (LPARAM)&col);

			col.pszText = (LPSTR)"User time";
			col.cx = 100;
			SendDlgItemMessage(hWnd, IDC_LIST, LVM_INSERTCOLUMN, 3, (LPARAM)&col);

			col.pszText = (LPSTR)"Kernel time";
			col.cx = 100;
			SendDlgItemMessage(hWnd, IDC_LIST, LVM_INSERTCOLUMN, 4, (LPARAM)&col);

			char filename[MAX_PATH] = "notepad.exe";
			SetDlgItemText(hWnd, IDC_COMMANDLINE, filename);
		}
		return TRUE;
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->idFrom == IDC_LIST) {
			UpdateButtons(hWnd);
			return TRUE;
		}
		return FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BROWSEBTN:
		{
			char filename[MAX_PATH] = "";
			if (BrowseFileName(hWnd, filename))
			{
				SetDlgItemText(hWnd, IDC_COMMANDLINE, filename);
			}
		}
		return TRUE;
		case IDC_STARTBTN:
		{
			char buf[100];
			GetDlgItemText(hWnd, IDC_COMMANDLINE, buf, 100);
			int pos = Buffer->AddProcess(buf);

			LVITEM item = { 0 };
			item.mask = LVIF_PARAM;
			item.lParam = pos;
			item.iItem = pos;
			SendDlgItemMessage(hWnd, IDC_LIST, LVM_INSERTITEM, 0, (LPARAM)&item);
			HANDLE hThread = CreateThread(NULL, 0, ProcessThread, (LPVOID)pos, 0, NULL);
			if (hThread != NULL) {
				CloseHandle(hThread);
			}
		}

		return TRUE;
		case IDC_DELETEBTN:
		{
			int row = SendDlgItemMessage(hWnd, IDC_LIST, LVM_GETSELECTIONMARK, 0, 0);
			LVITEM item = { 0 };
			item.mask = LVIF_PARAM;
			item.iItem = row;
			SendDlgItemMessage(hWnd, IDC_LIST, LVM_GETITEM, 0, (LPARAM)&item);
			int pos = (int)item.lParam;

			TProcessInfo process;
			Buffer->Get(pos, process);

			if (process.State != psRunning) {
				SendDlgItemMessage(hWnd, IDC_LIST, LVM_DELETEITEM, row, 0);
				process.State = psEmpty;
				Buffer->Set(pos, process);
			}
		}
		return TRUE;

		case IDC_KILLBTN:
		{
			int row = SendDlgItemMessage(hWnd, IDC_LIST, LVM_GETSELECTIONMARK, 0, 0);
			LVITEM item = { 0 };
			item.mask = LVIF_PARAM;
			item.iItem = row;
			SendDlgItemMessage(hWnd, IDC_LIST, LVM_GETITEM, 0, (LPARAM)&item);

			int pos = (int)item.lParam;
			TProcessInfo process;
			Buffer->Get(pos, process);

			if (process.State == psRunning) {
				if (TerminateProcess(process.Handle, 0)) {
					process.State = psTerminated;
					CloseHandle(process.Handle);
					Buffer->Set(pos, process);
				}
			}
		}
		return TRUE;
		case IDCANCEL:
			DestroyWindow(hWnd);
			return TRUE;
		}
		return FALSE;
	case WM_UPDATELIST:
	{
		char buf[100];
		LVFINDINFO fitem = { 0 };
		fitem.flags = LVFI_PARAM;
		fitem.lParam = (LPARAM)wParam;
		int row = SendDlgItemMessage(hWnd, IDC_LIST, LVM_FINDITEM, -1, (LPARAM)&fitem);

		TProcessInfo process;
		Buffer->Get((int)wParam, process);

		LVITEM item = { 0 };
		item.mask = LVIF_TEXT;
		item.iItem = row;

		switch (process.State) {
		case psRunning:
			sprintf(buf, "Running");
			break;
		case psTerminated:
			sprintf(buf, "Terminated");
			break;
		case psError:
			sprintf(buf, "Error");
			break;
		default:
			sprintf(buf, "New");
			break;
		}

		item.iSubItem = 0;
		item.pszText = buf;
		SendDlgItemMessage(hWnd, IDC_LIST, LVM_SETITEM, 0, (LPARAM)&item);

		item.iSubItem = 1;
		item.pszText = process.Name;
		SendDlgItemMessage(hWnd, IDC_LIST, LVM_SETITEM, 0, (LPARAM)&item);

		sprintf(buf, "%d", process.PID);
		item.iSubItem = 2;
		item.pszText = buf;
		SendDlgItemMessage(hWnd, IDC_LIST, LVM_SETITEM, 0, (LPARAM)&item);

		sprintf(buf, "%d", process.UserTime);
		item.iSubItem = 3;
		item.pszText = buf;
		SendDlgItemMessage(hWnd, IDC_LIST, LVM_SETITEM, 0, (LPARAM)&item);

		sprintf(buf, "%d", process.KernelTime);
		item.iSubItem = 4;
		item.pszText = buf;
		SendDlgItemMessage(hWnd, IDC_LIST, LVM_SETITEM, 0, (LPARAM)&item);
	}
	return TRUE;
	case WM_DESTROY:
		delete Buffer;
		Terminate = true;
		Sleep(100);
		PostQuitMessage(0);
		return TRUE;
	}
	return FALSE;
}