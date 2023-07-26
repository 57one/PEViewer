#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <stdio.h>

#include "global.h"
#include "handleProcessAndModule.h"
#include "messageCallback.h"
#pragma comment(lib, "comctl32.lib")

// mainDialog handle
HWND mainDialogHwnd = NULL;
// mainDialog Rect
SIZE mainDialogSize = {0};

BOOL CALLBACK MainDialogProc(HWND hwnd,      // handle to dialog box
                             UINT message,   // message
                             WPARAM wParam,  // first message parameter
                             LPARAM lParam   // second message parameter
) {
  switch (message) {
    case WM_INITDIALOG: {
      RECT rect = {0};
      GetClientRect(hwnd, &rect);
      mainDialogSize.cx = rect.right - rect.left;
      mainDialogSize.cy = rect.bottom - rect.top;
      mainDialogHwnd = hwnd;
      InitProcessListView(hwnd);
      InitModulesListView(hwnd);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_NOTIFY: {
      NMHDR* pNMHDR = (NMHDR*)lParam;
      // click Process in List
      if (wParam == IDC_LIST_PROCESS && pNMHDR->code == NM_CLICK) {
        EnumModules(GetDlgItem(hwnd, IDC_LIST_PROCESS),
                    GetDlgItem(hwnd, IDC_LIST_MODULE), wParam, lParam);
      }
      return TRUE;
    }
    case WM_GETMINMAXINFO: {
      // restrict window size
      onGetMinMaxInfo((MINMAXINFO*)lParam);
      return TRUE;
    }
    case WM_SIZE: {
      // resize the dialog
      onSize(mainDialogHwnd, &mainDialogSize, wParam, lParam);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_PEVIEWER: {
          return TRUE;
        }
        case IDC_BUTTON_PROFILE: {
          return TRUE;
        }
        case IDC_BUTTON_QUIT: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow) {
#ifdef DEV
  // use console to debug;
  FILE* fp;

  AllocConsole();
  freopen_s(&fp, "CONIN$", "r", stdin);
  freopen_s(&fp, "CONOUT$", "w", stdout);
  freopen_s(&fp, "CONOUT$", "w", stderr);
#endif  // DEV

  // IN Windows XP common control window must be INIT
  //通用控件在使用前，需要通过INITCOMMONCONTROLSEX进行初始化
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&icex);

  DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, MainDialogProc);
  return 0;
}