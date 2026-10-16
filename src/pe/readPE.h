#pragma once
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <commctrl.h>
#include <map>

#include "resource.h"
#include "common/structs.h"
#include "PEContext.h"

VOID readDosHeader(HWND hwnd, const PEContext& context);

VOID readFileHeader(HWND hwnd, const PEContext& context);

VOID readOptinalHeader32(HWND hwnd, const PEContext& context);

VOID readHeaderInfo(HWND hwnd, const PEContext& context);

VOID readSections(HWND hwnd, HWND hListSection, const PEContext& context);

VOID readDirectory(HWND hwnd, const PEContext& context);

VOID readSectionEdit(HWND hwnd, INT ID, const PEContext& context);

VOID readImportDirectory(HWND hwnd, HWND hListImportDLL, const PEContext& context);

VOID readIntThunk(HWND hwnd, HWND hListImportDLL, HWND hListIntThunk, const PEContext& context);

VOID writeToText(HWND hwnd, INT TEXT_ID, CONST TCHAR* format, DWORD data);

VOID readExportDirectory(HWND hwnd, const PEContext& context);

VOID readExportFunc(HWND hwnd, HWND hListExportFunc, const PEContext& context);

VOID readBoundImportDirectory(HWND hwnd, HWND hBoundImport, const PEContext& context);

VOID readDebugDirectory(HWND hwnd, HWND hDebug, const PEContext& context);
