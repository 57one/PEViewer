#pragma once

#include <windows.h>
#include <commctrl.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <tchar.h>

#include "global.h"
#include "resource.h"
#include "structs.h"
#include "utility.h"

VOID InitProcessListView(HWND hDlg);

VOID InitModulesListView(HWND hDlg);

VOID EnumProcess(HWND hListProcess);

VOID EnumModule(HWND hListModule);

VOID EnumModules(HWND hListProcess, HWND hListModule, WPARAM wParam, LPARAM lParam);