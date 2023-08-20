#include "handleProcessAndModule.h"

#include <string>

VOID InitProcessListView(HWND hDlg) {
  LV_COLUMN lv;
  HWND hListProcess;

  //初始化
  memset(&lv, 0, sizeof(LV_COLUMN));
  //获取IDC_LIST_PROCESS句柄 (List的句柄)
  hListProcess = GetDlgItem(hDlg, IDC_LIST_PROCESS);
  //设置整行选中
  //点击某行的时候 显示整行选中
  SendMessage(hListProcess, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT,
              LVS_EX_FULLROWSELECT);
  
  //第一列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("Process"));  //列标题
  lv.cx = 100;                                         //列宽
  lv.iSubItem = 0;                                     //第几列
  // ListView_InsertColumn(hListProcess, 0, &lv);
  SendMessage(hListProcess, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
  //第二列
  lv.pszText = const_cast<wchar_t*>(TEXT("Architecture"));
  lv.cx = 50;
  lv.iSubItem = 1;
  SendMessage(hListProcess, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
  //第三列
  lv.pszText = const_cast<wchar_t*>(TEXT("PID"));
  lv.cx = 80;
  lv.iSubItem = 2;
  // ListView_InsertColumn(hListProcess, 1, &lv);
  SendMessage(hListProcess, LVM_INSERTCOLUMN, 2, (DWORD)&lv);
  //第四列
  lv.pszText = const_cast<wchar_t*>(TEXT("Image Base"));
  lv.cx = 80;
  lv.iSubItem = 3;
  ListView_InsertColumn(hListProcess, 3, &lv);
  //第五列
  lv.pszText = const_cast<wchar_t*>(TEXT("Size of Image"));
  lv.cx = 80;
  lv.iSubItem = 4;
  ListView_InsertColumn(hListProcess, 4, &lv);

  EnumProcess(hListProcess);
}

VOID InitModulesListView(HWND hDlg) {
  LV_COLUMN lv;
  HWND hListModule;

  //初始化
  memset(&lv, 0, sizeof(LV_COLUMN));
  //获取IDC_LIST_MODULE的句柄 (module的句柄)
  hListModule = GetDlgItem(hDlg, IDC_LIST_MODULE);
  //设置整行选中
  SendMessage(hListModule, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT,
              LVS_EX_FULLROWSELECT);

  //第一列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("Module"));  //列标题
  lv.cx = 100;                                        //列宽
  lv.iSubItem = 0;                                    //第几列
  SendMessage(hListModule, LVM_INSERTCOLUMN, 0, (DWORD)&lv);

  //第二列
  lv.pszText = const_cast<wchar_t*>(TEXT("Path"));
  lv.cx = 250;
  lv.iSubItem = 1;
  ListView_InsertColumn(hListModule, 1, &lv);

  // EnumModule(hListModule);
}

VOID DrawProcess(HWND hListProcess, int Row, ProcessInfo processInfo,
                 COLORREF colorRef) {
  LV_ITEM vitem;
  //初始化
  memset(&vitem, 0, sizeof(LV_ITEM));
  vitem.mask = LVIF_TEXT;

  vitem.pszText = processInfo.szProcessName;
  vitem.iItem = Row;   //第几行
  vitem.iSubItem = 0;  //第几列
  // 这是一个宏 和 sendMessage做的事情一样
  // ListView_InsertItem(hListProcess, &vitem);
  //凡事皆消息
  SendMessage(hListProcess, LVM_INSERTITEM, 0, (DWORD)&vitem);
  SendMessage(hListProcess, LVM_SETINSERTMARKCOLOR, 0, (DWORD)colorRef);

  //第一行写完之后 后面全是 SETITEM
  vitem.pszText = processInfo.szArchitecture;
  vitem.iSubItem = 1;
  ListView_SetItem(hListProcess, &vitem);

  vitem.pszText = processInfo.szProcessId;
  vitem.iSubItem = 2;
  ListView_SetItem(hListProcess, &vitem);

  vitem.pszText = processInfo.szImageBase;
  vitem.iSubItem = 3;
  ListView_SetItem(hListProcess, &vitem);

  vitem.pszText = processInfo.szSizeofImage;
  vitem.iSubItem = 4;
  ListView_SetItem(hListProcess, &vitem);
}

VOID DrawModules(HWND hListModule, int Row, ModuleInfo moduleInfo) {
  LV_ITEM vitem;

  //初始化
  memset(&vitem, 0, sizeof(LV_ITEM));
  vitem.mask = LVIF_TEXT;

  vitem.pszText = moduleInfo.szModName;
  vitem.iItem = Row;   //第几行
  vitem.iSubItem = 0;  //第几列
  SendMessage(hListModule, LVM_INSERTITEM, 0, (DWORD)&vitem);

  //第一行写完之后 后面全是 SETITEM
  vitem.pszText = moduleInfo.szModAbsolutePath;
  vitem.iSubItem = 1;
  ListView_SetItem(hListModule, &vitem);
}

VOID EnumProcess(HWND hListProcess) {
  DWORD iProcesses[1024], cbNeeded, cProcesses;
  if (!EnumProcesses(iProcesses, sizeof(iProcesses), &cbNeeded)) {
    return;
  }
  // number of Process;
  cProcesses = cbNeeded / sizeof(DWORD);
  HANDLE hProcess;
  ProcessInfo processInfo;

  TCHAR szExeName[MAX_PATH] = TEXT("<unknown>");
  DWORD exeNameSize = MAX_PATH;
  TCHAR* pChr = NULL;

  COLORREF colorRef;
  UINT32 count = 0;
#ifdef DEV
  UINT32 totalcount = 0;
  UINT32 nullcount = 0;
  UINT32 errorcount = 0;
  UINT32 x64count = 0;
#endif
  setDebugPrivilege(TRUE);

  HMODULE hMod;
  MODULEINFO module_info;
  colorRef = (count % 2 == 1) ? colorGrey : colorWhite;
  for (UINT32 i = 0; i < cProcesses; i++) {
    if (iProcesses[i] != 0) {
      totalcount += 1;
      hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                             iProcesses[i]);
      if (hProcess == NULL) {
        nullcount += 1;
        wsprintf(processInfo.szProcessName, TEXT("%s"), szAccessDenied);
        wsprintf(processInfo.szProcessId, TEXT("%08ld"), iProcesses[i]);
        wsprintf(processInfo.szArchitecture, TEXT("%s"), szAccessDenied);
        wsprintf(processInfo.szImageBase, TEXT("%s"), szAccessDenied);
        wsprintf(processInfo.szSizeofImage, TEXT("%s"), szAccessDenied);
        DrawProcess(hListProcess, count, processInfo, colorRef);
        count++;
      } else {
        memset(&module_info, 0, sizeof(module_info));
        if (isX86Process(hProcess)) {
          if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
            GetModuleInformation(hProcess, hMod, &module_info,
                                 sizeof(module_info));

            GetModuleBaseName(
                hProcess, hMod, processInfo.szProcessName,
                sizeof(processInfo.szProcessName) / sizeof(TCHAR));
            wsprintf(processInfo.szArchitecture, TEXT("%s"), szX86);
            wsprintf(processInfo.szProcessId, TEXT("%08ld"), iProcesses[i]);
            wsprintf(processInfo.szImageBase, TEXT("%08X"),
                     DWORD(module_info.lpBaseOfDll));
            wsprintf(processInfo.szSizeofImage, TEXT("%08X"),
                     module_info.SizeOfImage);

            DrawProcess(hListProcess, count, processInfo, colorRef);
            count++;
          } else {
          }
        } else {
          exeNameSize = MAX_PATH;
          QueryFullProcessImageName(hProcess, 0x0, szExeName, &exeNameSize);
          pChr = _tcsrchr(szExeName, TEXT('\\'));
          wsprintf(processInfo.szProcessName, TEXT("%s"), pChr + 1);
          wsprintf(processInfo.szProcessId, TEXT("%08ld"), iProcesses[i]);
          wsprintf(processInfo.szArchitecture, TEXT("%s"), szX64);
          wsprintf(processInfo.szImageBase, TEXT("%s"), szUnknown);
          wsprintf(processInfo.szSizeofImage, TEXT("%s"), szUnknown);
          DrawProcess(hListProcess, count, processInfo, colorRef);
          count++;
        }
        CloseHandle(hProcess);
      }
    }
  }
  setDebugPrivilege(FALSE);
#ifdef DEV
  _tprintf(TEXT("%d\n"), cProcesses);
  _tprintf(TEXT("count: %d\n"), count);
  _tprintf(TEXT("nullcount: %d\n"), nullcount);
  _tprintf(TEXT("x64count: %d\n"), x64count);
  _tprintf(TEXT("errorcount: %d\n"), errorcount);
  _tprintf(TEXT("totalcount :%d\n"), totalcount);
#endif
}

// use ToolHelp32 API
VOID EnumProcess2(HWND hListProcess) {
  setDebugPrivilege(TRUE);
  HANDLE hProcessSnap, hModuleSnap;
  PROCESSENTRY32 pe32;
  MODULEENTRY32 me32;
  TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
  TCHAR szImageBase[MAX_PATH] = TEXT("<unknown>");
  TCHAR szSizeofImage[MAX_PATH] = TEXT("<unknown>");
  TCHAR szBuf[6] = {0};

  ProcessInfo processInfo;
  DWORD dwPID;
  UINT32 count = 0;
  UINT32 totalcount = 0;
  UINT32 nullcount = 0;
  COLORREF colorRef = RGB(0xf1, 0xf2, 0xf6);
  hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
  if (hProcessSnap == INVALID_HANDLE_VALUE) {
    printErrorCode(TEXT("CreateToolhelp32Snapshot (of processes)"));
    return;
  }
  pe32.dwSize = sizeof(PROCESSENTRY32);

  if (!Process32First(hProcessSnap, &pe32)) {
    printErrorCode(TEXT("Process32First"));
    CloseHandle(hProcessSnap);
    return;
  }
  do {
    dwPID = pe32.th32ProcessID;
    totalcount += 1;
    hModuleSnap = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwPID);
    printf("%d\n", dwPID);
    if (hModuleSnap == INVALID_HANDLE_VALUE) {
      printErrorCode(TEXT("CreateToolhelp32Snapshot (of modules)"));
      continue;
    }
    me32.dwSize = sizeof(MODULEENTRY32);
    if (!Module32First(hModuleSnap, &me32)) {
      printErrorCode(TEXT("Module32First"));
      continue;
    }

    CloseHandle(hModuleSnap);
    wsprintf(processInfo.szImageBase, TEXT("%08X"), (DWORD)(me32.modBaseAddr));
    wsprintf(processInfo.szSizeofImage, TEXT("%08X"), me32.modBaseSize);
    wsprintf(processInfo.szProcessName, TEXT("%s"), me32.szModule);
    // GetModuleBaseName(hProcess, hMods[0], szProcessName,
    //                  sizeof(szProcessName) / sizeof(TCHAR));
    wsprintf(processInfo.szProcessId, TEXT("%ld"), dwPID);
    DrawProcess(hListProcess, count, processInfo, colorRef);
    count++;

  } while (Process32Next(hProcessSnap, &pe32));

  printf("%d\n", count);
  printf("%d\n", totalcount);
  CloseHandle(hProcessSnap);

  setDebugPrivilege(FALSE);
}

VOID EnumModules(HWND hListProcess, HWND hListModule, WPARAM wParam,
                 LPARAM lParam) {
  DWORD dwRowId;
  TCHAR szPid[0x20] = {0};
  LV_ITEM lv;

  //初始化
  memset(&lv, 0, sizeof(LV_ITEM));
  memset(szPid, 0, 0x20);
  //获取选择行
  dwRowId = SendMessage(hListProcess, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
  if (dwRowId == -1) {
    MessageBox(NULL, TEXT("Please select a process"), TEXT("Error"), MB_OK);
    return;
  }
  setDebugPrivilege(TRUE);
  //获取PID
  lv.iSubItem = 2;       //要获取的列
  lv.pszText = szPid;    //指定存储查询结果的缓冲区
  lv.cchTextMax = 0x20;  //指定缓冲区大小
  SendMessage(hListProcess, LVM_GETITEMTEXT, dwRowId, (DWORD)&lv);

  DWORD processID;
  HMODULE hMods[1024];
  HANDLE hProcess;
  DWORD cbNeeded;
  UINT32 count = 0;
  ModuleInfo moduleInfo;

  processID = _tcstoul(szPid, NULL, 10);
  hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                         processID);
  if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
    for (UINT32 i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
      // Get the full path to the module's file.
      if (GetModuleBaseName(hProcess, hMods[i], moduleInfo.szModName,
                            sizeof(moduleInfo.szModName) / sizeof(TCHAR))) {
      }

      if (GetModuleFileNameEx(
              hProcess, hMods[i], moduleInfo.szModAbsolutePath,
              sizeof(moduleInfo.szModAbsolutePath) / sizeof(TCHAR))) {
      }

      DrawModules(hListModule, count, moduleInfo);
      count++;
    }
  }
  setDebugPrivilege(FALSE);
}