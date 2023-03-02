#pragma once

#include <windows.h>
#include <commctrl.h>
#include <Psapi.h>
#include "resource.h"

VOID InitProcessListView(HWND hDlg);

VOID InitModulesListView(HWND hDlg);

VOID EnumProcess(HWND hListProcess);

VOID EnumModule(HWND hListModule);