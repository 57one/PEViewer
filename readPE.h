#pragma once
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <tchar.h>

extern LPVOID* pFileBuffer;
DWORD readPeFile(IN PTCHAR lpszFile, OUT LPVOID* pFileBuffer);