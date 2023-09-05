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

SYSTEMTIME secondsToSystemTime(DWORD seconds) {
  SYSTEMTIME sysTime = {0};
  struct tm time;
  time_t seconds_t = seconds;
  gmtime_s(&time, &seconds_t);
  // time.tm_year = current year - 1900
  sysTime.wYear = time.tm_year + 1900;
  // time.tm_mon (0 ~ 11) sysTime.wMonth (0 ~ 12)
  sysTime.wMonth = time.tm_mon + 1;
  // time.tm_mday/sysTime.wDay (1 ~ 31)
  sysTime.wDay = time.tm_mday;
  // time.tm_wday/sysTime.wDayOfWeek (0 ~ 6)
  sysTime.wDayOfWeek = time.tm_wday;
  // time.tm_hour/sysTime.wHour (0 ~ 23)
  sysTime.wHour = time.tm_hour;
  // time.tm_min/sysTime.wMinute (0 ~ 59)
  sysTime.wMinute = time.tm_min;
  // time.tm_sec/sysTime.wSecond (0 ~ 59)
  sysTime.wSecond = time.tm_sec;
  return sysTime;
}

DWORD systemTimeToSeconds(SYSTEMTIME sysTime) { 
    struct tm t = {0}; 
    time_t seconds = 0;
    long timezoneSeconds = 0;
    t.tm_year = sysTime.wYear - 1900;
    t.tm_mon = sysTime.wMonth - 1;
    t.tm_mday = sysTime.wDay;
    t.tm_wday = sysTime.wDayOfWeek;
    t.tm_hour = sysTime.wHour;
    t.tm_min = sysTime.wMinute;
    t.tm_sec = sysTime.wSecond;
    _get_timezone(&timezoneSeconds);
    // remove timezone
    seconds = mktime(&t) - timezoneSeconds;
    return seconds;
}