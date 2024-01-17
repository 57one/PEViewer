#pragma once

typedef struct ProcessInfo {
  TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
  TCHAR szArchitecture[MAX_PATH] = TEXT("<unknown>");
  TCHAR szProcessId[MAX_PATH] = TEXT("<unknown>");
  TCHAR szImageBase[MAX_PATH] = TEXT("<unknown>");
  TCHAR szSizeofImage[MAX_PATH] = TEXT("<unknown>");
} ProcessInfo;

typedef struct ModuleInfo {
  TCHAR szModName[MAX_PATH] = TEXT("<unknown>");
  TCHAR szModAbsolutePath[MAX_PATH] = TEXT("<unknown>");
} ModuleInfo;

typedef struct SectionInfo {
  TCHAR szIndex[MAX_PATH] = TEXT("<unknown>");
  TCHAR szSectionName[MAX_PATH] = TEXT("<unknown>");
  TCHAR szVirtualSize[MAX_PATH] = TEXT("<unknown>");
  TCHAR szVirtualOffset[MAX_PATH] = TEXT("<unknown>");
  TCHAR szRawSize[MAX_PATH] = TEXT("<unknown>");
  TCHAR szRawOffset[MAX_PATH] = TEXT("<unknown>");
  TCHAR szCharacteristics[MAX_PATH] = TEXT("<unknown>");
} SectionInfo;

typedef struct ImportDescInfo {
  TCHAR szDllName[MAX_PATH] = TEXT("<unknown>");
  TCHAR szOriginalFirstThunk[MAX_PATH] = TEXT("<unknown>");
  TCHAR szTimeDateStamp[MAX_PATH] = TEXT("<unknown>");
  TCHAR szForwarderChain[MAX_PATH] = TEXT("<unknown>");
  TCHAR szName[MAX_PATH] = TEXT("<unknown>");
  TCHAR szFirstThunk[MAX_PATH] = TEXT("<unknown>");
} ImportDescInfo;

typedef struct IntThunkInfo {
  TCHAR szAPIName[MAX_PATH] = TEXT("<unknown>");
  TCHAR szThunkRVA[MAX_PATH] = TEXT("<unknown>");
  TCHAR szThunkRAW[MAX_PATH] = TEXT("<unknown>");
  TCHAR szThunkValue[MAX_PATH] = TEXT("<unknown>");
  TCHAR szHint[MAX_PATH] = TEXT("<unknown>");
} IntThunkInfo;

typedef struct ExportFuncInfo {
  TCHAR szOrdinal[MAX_PATH] = TEXT("<unknown>");
  TCHAR szFuncRVA[MAX_PATH] = TEXT("<unknown>");
  TCHAR szFuncRAW[MAX_PATH] = TEXT("<unknown>");
  TCHAR szFuncName[MAX_PATH] = TEXT("<unknown>");
} ExportFuncInfo;

typedef struct BoundImportInfo {
  TCHAR szDllName[MAX_PATH] = TEXT("<unknown>");
  TCHAR szTimeDateStamp[MAX_PATH] = TEXT("<unknown>");
  TCHAR szOffsetModuleName[MAX_PATH] = TEXT("<unknown>");
  TCHAR szNumberOfModuleForwarderRefs[MAX_PATH] = TEXT("<unknown>");
} BoundImportInfo;