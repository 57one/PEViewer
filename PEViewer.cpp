#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"
#include "handleProcessAndModule.h"
#pragma comment(lib,"comctl32.lib")	

BOOL CALLBACK MainDialogProc(
	HWND hwnd,  // handle to dialog box			
	UINT message,     // message			
	WPARAM wParam, // first message parameter			
	LPARAM lParam  // second message parameter			
)
{
	switch(message) {
		case WM_INITDIALOG: {
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
			if (wParam == IDC_LIST_PROCESS && pNMHDR->code == NM_CLICK) {
				EnumModules(GetDlgItem(hwnd, IDC_LIST_PROCESS), GetDlgItem(hwnd, IDC_LIST_MODULE), wParam, lParam);
			}
		}
	}
	return FALSE;
}


int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
	AttachConsole(-1);
	//IN Windows XP common control window must be INIT
	//通用控件在使用前，需要通过INITCOMMONCONTROLSEX进行初始化				
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icex);

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, MainDialogProc);
    return 0;
}