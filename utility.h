#pragma once

#include <windows.h>
#include <tchar.h>

// move widget
HDWP moveWidget(HWND dlgHwnd, HDWP hdwp, UINT ID, int x, int y, BOOL bLeft,
                BOOL bTop, BOOL bRight, BOOL bBottom);

void screenToClientRect(HWND dlgHwnd, LPRECT lpRect);

BOOL setDebugPrivilege(BOOL bEnable);

void printErrorCode();

void printErrorCode(LPCSTR lpcstr, BOOL bExit = FALSE);

BOOL isWow64(HANDLE hProcess);

BOOL isX86Process(HANDLE hProcess);