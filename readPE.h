#pragma once
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>

#include "resource.h"
extern LPVOID pFileBuffer;
extern PIMAGE_DOS_HEADER pDosHeader;
extern PIMAGE_NT_HEADERS pNTHeader;
extern PIMAGE_FILE_HEADER pFileHeader;
extern PIMAGE_OPTIONAL_HEADER32 pOptionalHeader32;

DWORD readPeFile(IN PTCHAR lpszFile, OUT LPVOID* pFileBuffer);

VOID initPE();

VOID readDosHeader(HWND hwnd, LPVOID pFileBuffer);

VOID readFileHeader(HWND hwnd, LPVOID pFileBuffer);

VOID readOptinalHeader32(HWND hwnd, LPVOID pFileBuffer);

VOID readHeaderInfo(HWND hwnd, LPVOID pFileBuffer);

VOID writeToText(HWND hwnd, INT TEXT_ID, CONST TCHAR* format, DWORD data);