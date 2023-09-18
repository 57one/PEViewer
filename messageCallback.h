#pragma once

#include <stdio.h>
#include <windows.h>
#include <windowsx.h>

#include "global.h"
#include "readPE.h"
#include "resource.h"
#include "utility.h"

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

// IDC_COMBO_MACHINE_TYPE CBN_SELCHANGE
void onMachineTypeChange(HWND hwnd, HWND hCombo);

// WM_COMMAND IDC_BUTTON_FILE_HEADER_CHARACTERISTICS
void onCharacteristics(HWND hwnd);

// WM_INITDIALOG
void onCharacteristicsInit(HWND hwnd, LPARAM lParam);

// WM_COMMAND IDC_CHECKBOX
void handleCheckBoxesChecked(HWND hwnd, INT checkBoxID);

void setDateTime(HWND hwnd, SYSTEMTIME sysTime);

// WM_COMMAND IDC_BUTTON_FILE_HEADER_TIMEDATESTAMP
void onTimeDateStamp(HWND hwnd);

// WM_INITDIALOG
void onTimeDateStampInit(HWND hwnd, LPARAM lParam);

// WM_COMMAND
void onOptinalHeader(HWND hwnd);

// WM_INITDIALOG
void onOptinalHeaderInit(HWND hwnd, LPARAM lParam);

// WM_COMMAND IDC_BUTTON_OPTIONAL_HEADER32_MAGIC
void onMagicType(HWND hwnd);

// WM_INITDIALOG
void onMagicTypeInit(HWND hwnd, LPARAM lParam);

// IDC_COMBO_MAGIC_TYPE CBN_SELCHANGE
void onMagicTypeChange(HWND hwnd, HWND hCombo);

// WM_COMMAND IDC_BUTTON_OPTIONAL_HEADER32_SIZE_OF_IMAGE
void checkHeader32Image(HWND hwnd);

// WM_COMMAND IDC_BUTTON_OPTIONAL_HEADER32_IMAGE_ADD
void addHeader32Image(HWND hwnd);

// WM_COMMAND IDC_BUTTON_OPTIONAL_HEADER32_SIZE_OF_HEADERS
void checkHeader32Headers(HWND hwnd);

// WM_COMMAND IDC_BUTTON_OPTIONAL_HEADER32_HEADER_ADD
void addHeader32Headers(HWND hwnd);

// WM_COMMAND IDC_BUTTON_OPTIONAL_HEADER32_CHECKSUM
void checkHeader32CheckSum(HWND hwnd);

// WM_COMMAND IDC_BUTTON_OPTIONAL_HEADER32_SUBSTM
void onSubsystem(HWND hwnd);

// WM_INITDIALOG
void onSubsystemInit(HWND hwnd, LPARAM lParam);

// IDC_COMBO_SUBSYSTEM CBN_SELCHANGE
void onSubsystemChange(HWND hwnd, HWND hCombo);