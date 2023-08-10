#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <stdio.h>

#include "resource.h"
#include "windowProc.h"
#pragma comment(lib, "comctl32.lib")
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,                                     \
                "/manifestdependency:\"type='win32' "       \
                "name='Microsoft.Windows.Common-Controls' " \
                "version='6.0.0.0' "                        \
                "processorArchitecture='x86' "              \
                "publicKeyToken='6595b64144ccf1df' "        \
                "language='*' "                             \
                "\"")
#elif defined _M_IA64
#pragma comment(linker,                                     \
                "/manifestdependency:\"type='win32' "       \
                "name='Microsoft.Windows.Common-Controls' " \
                "version='6.0.0.0' "                        \
                "processorArchitecture='ia64' "              \
                "publicKeyToken='6595b64144ccf1df' "        \
                "language='*' "                             \
                "\"")
#elif defined _M_X64
#pragma comment(linker,                                     \
                "/manifestdependency:\"type='win32' "       \
                "name='Microsoft.Windows.Common-Controls' " \
                "version='6.0.0.0' "                        \
                "processorArchitecture='amd64' "              \
                "publicKeyToken='6595b64144ccf1df' "        \
                "language='*' "                             \
                "\"")
#else
#pragma comment(linker,                                     \
                "/manifestdependency:\"type='win32' "       \
                "name='Microsoft.Windows.Common-Controls' " \
                "version='6.0.0.0' "                        \
                "processorArchitecture='*' "              \
                "publicKeyToken='6595b64144ccf1df' "        \
                "language='*' "                             \
                "\"")
#endif
#endif  // _UNICODE


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
  //通用控件在使用前，需要通过INITCOMMONCONTROLSEX进行初始化
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&icex);

  DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, MainDialogProc);
  return 0;
}