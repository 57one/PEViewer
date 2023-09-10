#pragma once
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h>

#include "resource.h"
extern LPVOID pFileBuffer;
DWORD readPeFile(IN PTCHAR lpszFile, OUT LPVOID* pFileBuffer);

VOID readDosHeader(HWND hwnd, LPVOID pFileBuffer);

VOID readFileHeader(HWND hwnd, LPVOID pFileBuffer);

VOID readOptinalHeader32(HWND hwnd, LPVOID pFileBuffer);

VOID writeToText(HWND hwnd, INT TEXT_ID, CONST TCHAR* format, DWORD data);