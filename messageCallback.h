#pragma once

#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#include "global.h"
#include "resource.h"
#include "utility.h"
#include "readPE.h"

// WM_SIZE
void onSize(HWND mainDialogHwnd, LPSIZE mainDialogSize, WPARAM wParam,
            LPARAM lParam);

// WM_GETMINMAXINFO
void onGetMinMaxInfo(MINMAXINFO* lpMMI);

// WM_COMMAND BUTTON_PE_EDITOR
void onPeEditor(HWND hwnd);

// WM_INITDIALOG
void onPeEditorInit(HWND hwnd, LPARAM lParam);

// WM_COMMAND IDC_BUTTON_FILE_HEADER
void onFileHeader(HWND hwnd);

// WM_INITDIALOG
void onFileHeaderInit(HWND hwnd, LPARAM lParam);

// WM_COMMAND IDC_BUTTON_FILE_HEADER_MACHINE
void onMachineType(HWND hwnd);

// WM_INITDIALOG
void onMachineTypeInit(HWND hwnd, LPARAM lParam);