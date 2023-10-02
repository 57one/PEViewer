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