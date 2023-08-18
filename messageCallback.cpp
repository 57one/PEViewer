#include "messageCallback.h"

#include <stdio.h>

#include "windowProc.h"
void onSize(HWND mainDialogHwnd, LPSIZE mainDialogSize, WPARAM wParam,
            LPARAM lParam) {
  switch (wParam) {
    case SIZE_MAXHIDE:
    case SIZE_MAXSHOW:
    case SIZE_MINIMIZED:
      return;
  }
  int nWidth = LOWORD(lParam);
  int nHeight = HIWORD(lParam);

  // get difference
  int x, y;
  x = nWidth - (*mainDialogSize).cx;
  y = nHeight - (*mainDialogSize).cy;
  HDWP hdwp = BeginDeferWindowPos(7);
  if (hdwp == NULL) return;
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_GROUP_LIST, x, y, FALSE, FALSE,
                    TRUE, TRUE);
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_LIST_PROCESS, x, y, FALSE, FALSE,
                    TRUE, TRUE);
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_LIST_MODULE, x, y, FALSE, TRUE,
                    TRUE, TRUE);
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_GROUP_BUTTON, x, y, TRUE, FALSE,
                    TRUE, TRUE);
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_BUTTON_PE_EDITOR, x, y, TRUE,
                    FALSE, TRUE, FALSE);
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_BUTTON_PROFILE, x, y, TRUE, TRUE,
                    TRUE, TRUE);
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_BUTTON_QUIT, x, y, TRUE, TRUE,
                    TRUE, TRUE);
  EndDeferWindowPos(hdwp);
  mainDialogSize->cx = nWidth;
  mainDialogSize->cy = nHeight;
}

void onGetMinMaxInfo(MINMAXINFO* lpMinMaxInfo) {
  lpMinMaxInfo->ptMinTrackSize.x = 630;
  lpMinMaxInfo->ptMinTrackSize.y = 300;
}

void onPeEditor(HWND hwnd) {
  OPENFILENAME ofn = {0};
  TCHAR szBuffer[MAX_PATH * 10] = {0};  // file name
  HANDLE hf = NULL;                     // file handle
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = hwnd;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
  ofn.lpstrFile = szBuffer;
  ofn.lpstrFile[0] = '\0';
  ofn.nMaxFile = sizeof(szBuffer) / sizeof(*szBuffer);
  ofn.lpstrFilter = TEXT(
      "Executable "
      "Files(.exe;.scr;.dll;.ocx;.cpl;.drv)\0*.exe;*.scr;*.dll;*.ocx;*.cpl;*."
      "drv\0All Files(*.*)\0*.*\0");
  ofn.lpstrInitialDir = NULL;
  ofn.nFilterIndex = 0;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;

  // display the open dialog box.
  if (GetOpenFileName(&ofn) == TRUE) {
    /*_tprintf(TEXT("%s\n"), szBuffer);*/
    // Display the Open dialog box.
    readPeFile(szBuffer, &pFileBuffer);
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_PE_EDITOR),
                   hwnd, PeEditorProc, (LPARAM)szBuffer);
  }
}

void onPeEditorInit(HWND hwnd, LPARAM lParam) {
  // dynamically change the title of dialogbox
  TCHAR szTitle[MAX_PATH];
  GetWindowText(hwnd, szTitle, sizeof(szTitle) / sizeof(*szTitle));
  _tcscat_s(szTitle, TEXT(" -- "));
  _tcscat_s(szTitle, (PTCHAR)lParam);
  SetWindowText(hwnd, szTitle);

  // set text about dos header
  readDosHeader(hwnd, pFileBuffer);
}

void onFileHeaderInit(HWND hwnd, LPARAM lParam) {
    // set text about file header
  readFileHeader(hwnd, pFileBuffer);
}

void onFileHeader(HWND hwnd) {
  DialogBox(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDD_DIALOG_IMAGE_FILE_HEADER), hwnd,
            FileHeaderProc);
}