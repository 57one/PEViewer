#include <windows.h>
#include <tchar.h>
#include "resource.h"



BOOL CALLBACK MainDialogProc(
	HWND hwnd,  // handle to dialog box			
	UINT uMsg,     // message			
	WPARAM wParam, // first message parameter			
	LPARAM lParam  // second message parameter			
)
{
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{

    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, MainDialogProc);
    return 0;
}