#include "windowProc.h"

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
        case IDC_BUTTON_PE_EDITOR: {
          onPeEditor(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_ABOUT: {
          DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_ABOUT),
                    hwnd, AboutProc);
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

BOOL CALLBACK AboutProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  // need value for GDI+ init
  /*GdiplusStartupInput m_gdiplusStartupInput;
  ULONG_PTR m_gdiplusToken;*/

  switch (message) {
    case WM_INITDIALOG: {
      // dynamic set png/jpeg/xxx as PICTURE CONTROL source
      // Init GDI+
      /*GdiplusStartup(&m_gdiplusToken, &m_gdiplusStartupInput, NULL);
      Bitmap *bitmap;
      bitmap = Bitmap::FromFile(L"x:\\xxx\\xxx.png");
      HBITMAP hbitmap;
      bitmap->GetHBITMAP(0, &hbitmap);
      HWND hPic = GetDlgItem(hwnd, IDC_ABOUT_PIC);
      SendMessage(hPic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbitmap);
      GdiplusShutdown(m_gdiplusToken);*/
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_OK: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK PeEditorProc(HWND hwnd, UINT message, WPARAM wParam,
                           LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onPeEditorInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_FILE_HEADER: {
          onFileHeader(hwnd);
          return true;
        }
        case IDC_BUTTON_OPTION_HEADER: {
          return true;
        }
        case IDC_BUTTON_HEADER_INFO: {
          return true;
        }
        case IDC_BUTTON_SECTION: {
          return true;
        }
        case IDC_BUTTON_DIRECTORY: {
          return true;
        }
        case IDC_BUTTON_TOOLS: {
          return true;
        }
        case IDC_BUTTON_DOS_OK: {
          return TRUE;
        }
        case IDC_BUTTON_DOS_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK FileHeaderProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onFileHeaderInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
      }
    }
  }
  return FALSE;
}