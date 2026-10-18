#pragma once
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <commctrl.h>

#include "resource.h"
#include "common/structs.h"
#include "PEContext.h"

VOID readDosHeader(HWND hwnd, const PEContext& context);

VOID readFileHeader(HWND hwnd, const PEContext& context);

VOID readOptionalHeader(HWND hwnd, const PEContext& context);

VOID readHeaderInfo(HWND hwnd, const PEContext& context);

VOID readSections(HWND hwnd, HWND hListSection, const PEContext& context);

VOID readSectionEdit(HWND hwnd, INT ID, const PEContext& context);

// NULL displays N/A and disables the control; other text enables it.
VOID writeText(HWND hwnd, INT controlId, LPCTSTR text);

VOID writeToText(HWND hwnd, INT TEXT_ID, CONST TCHAR* format, DWORD data);

// Default hexadecimal widths for 32-bit and 64-bit PE fields.
VOID writeToText(HWND hwnd, INT TEXT_ID, DWORD data);
VOID writeToText(HWND hwnd, INT TEXT_ID, ULONGLONG data);
