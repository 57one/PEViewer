#pragma once
#define DEV

#include <windows.h>
#include <map>
#include "resource.h"

extern TCHAR szUnknown[];
extern TCHAR szNoDebugPriv[];
extern TCHAR szAccessDenied[];
extern TCHAR szX86[];
extern TCHAR szX64[];

extern COLORREF colorGrey;
extern COLORREF colorWhite;

extern TCHAR szMachineTypeDesc[][40];
extern WORD wMachineTypeLength;
extern WORD wMachineType[];
extern std::map<WORD, WORD> machinetypeToIndex;

extern DWORD characteristicsBegin;
extern DWORD characteristicsEnd;
extern INT characteristicsCheckBoxID[];
extern TCHAR szCharacteristics[][80];

extern TCHAR szMagicTypeDesc[][15];
extern WORD wMagicTypeLength;
extern WORD wMagicType[];
extern std::map<WORD, WORD> magicTypeToIndex;