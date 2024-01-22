#pragma once
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>
#include <commctrl.h>
#include <map>

#include "resource.h"
#include "structs.h"
extern LPVOID pFileBuffer;
extern PIMAGE_DOS_HEADER pDosHeader;
extern PIMAGE_NT_HEADERS pNTHeader;
extern PIMAGE_FILE_HEADER pFileHeader;
extern PIMAGE_OPTIONAL_HEADER32 pOptionalHeader32;

DWORD readPeFile(IN PTCHAR lpszFile, OUT LPVOID* pFileBuffer);

DWORD rvaToFileOffset(IN LPVOID pFileBuffer, IN DWORD dwRva);

VOID initPE();

VOID readDosHeader(HWND hwnd, LPVOID pFileBuffer);

VOID readFileHeader(HWND hwnd, LPVOID pFileBuffer);

VOID readOptinalHeader32(HWND hwnd, LPVOID pFileBuffer);

VOID readHeaderInfo(HWND hwnd, LPVOID pFileBuffer);

VOID readSections(HWND hwnd, HWND hListSection);

VOID readDirectory(HWND hwnd);

VOID readSectionEdit(HWND hwnd, INT ID);

VOID readImportDirectory(HWND hwnd, HWND hListImportDLL);

VOID readIntThunk(HWND hwnd, HWND hListImportDLL, HWND hListIntThunk);

VOID writeToText(HWND hwnd, INT TEXT_ID, CONST TCHAR* format, DWORD data);

VOID readExportDirectory(HWND hwnd);

VOID readExportFunc(HWND hwnd, HWND hListExportFunc);

VOID readBoundImportDirectory(HWND hwnd, HWND hBoundImport);

VOID readDebugDirectory(HWND hwnd, HWND hDebug);