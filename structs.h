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