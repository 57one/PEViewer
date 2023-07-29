#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <stdio.h>

#include "resource.h"
#include "windowProc.h"
#pragma comment(lib, "comctl32.lib")
// for GDI+
//#pragma comment(lib, "Gdiplus.lib")

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
  //ͨ�ÿؼ���ʹ��ǰ����Ҫͨ��INITCOMMONCONTROLSEX���г�ʼ��
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&icex);

  DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, MainDialogProc);
  return 0;
}