#include "utility.h"

#include <stdio.h>

HDWP moveWidget(HWND dlgHwnd, HDWP hdwp, UINT ID, int x, int y, BOOL bLeft,
                BOOL bTop, BOOL bRight, BOOL bBottom) {
  HWND hwnd = GetDlgItem(dlgHwnd, ID);
  RECT rect = {0};
  GetWindowRect(hwnd, &rect);
  // Screen position to Client position
  screenToClientRect(dlgHwnd, &rect);
  if (bLeft) {
    rect.left += x;
  }
  if (bRight) {
    rect.right += x;
  }
  if (bTop) {
    rect.top += y;
  }
  if (bBottom) {
    rect.bottom += y;
  }
  // move widget
  return DeferWindowPos(hdwp, hwnd, 0, rect.left, rect.top,
                        rect.right - rect.left, rect.bottom - rect.top,
                        SWP_NOZORDER);
}

void screenToClientRect(HWND dlgHwnd, LPRECT lpRect) {
  ScreenToClient(dlgHwnd, (LPPOINT)lpRect);
  ScreenToClient(dlgHwnd, (LPPOINT)lpRect + 1);
}

BOOL setDebugPrivilege(BOOL bEnable) {
  HANDLE hToken = INVALID_HANDLE_VALUE;
  HANDLE hProcess =
      OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
  if (!OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
                        &hToken)) {
    return FALSE;
  } else if (hToken == NULL || hToken == INVALID_HANDLE_VALUE) {
    return FALSE;
  }

  LUID luid;
  if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
    return FALSE;
  }

  // adjust privilege
  TOKEN_PRIVILEGES tokenPrivileges;
  tokenPrivileges.PrivilegeCount = 1;
  tokenPrivileges.Privileges[0].Luid = luid;
  tokenPrivileges.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;
  if (!AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, NULL, NULL,
                             NULL)) {
    return FALSE;
  }
  BOOL bRet = FALSE;
  bRet = (GetLastError() == ERROR_SUCCESS);
  if (bRet) {
    printf("Adjust Token success\n");
  } else {
    printErrorCode();
  }
  CloseHandle(hToken);
  CloseHandle(hProcess);
  return TRUE;
}

void printMessage(LPCTSTR lpcstr) {
  _tprintf(TEXT("\n-----%s-----\n"), lpcstr);
}

void printErrorCode() {
  DWORD error = GetLastError();
  _tprintf(TEXT("[errorCode: %ld]\n"), error);
}

void printErrorCode(LPCTSTR lpcstr, BOOL bExit) {
  DWORD error = GetLastError();
  _tprintf(TEXT("[errorCode: %ld]: %s\n"), error, lpcstr);
  if (bExit) {
    exit(1);
  }
}

BOOL isWow64(HANDLE hProcess) {
  BOOL bIsWow64 = FALSE;

  typedef BOOL(WINAPI * LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
  LPFN_ISWOW64PROCESS fnIsWow64Process;
  fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
      GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

  if (NULL != fnIsWow64Process) {
    if (!fnIsWow64Process(hProcess, &bIsWow64)) {
      printErrorCode(TEXT("isWow64"));
    }
  }
  return bIsWow64;
}

BOOL isX86Process(HANDLE hProcess) {
  SYSTEM_INFO systeminfo = {0};
  GetNativeSystemInfo(&systeminfo);

  // x86 architecture
  if (systeminfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
    return TRUE;
  }

  // check whether the process is 32bit process that is running in x64
  // architecture
  return isWow64(hProcess);
}