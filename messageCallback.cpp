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

void onFileHeader(HWND hwnd) {
  DialogBox(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDD_DIALOG_IMAGE_FILE_HEADER), hwnd,
            FileHeaderProc);
}

void onFileHeaderInit(HWND hwnd, LPARAM lParam) {
  // set text about file header
  readFileHeader(hwnd, pFileBuffer);
}

void onMachineType(HWND hwnd) {
  HWND hEditMachine = GetDlgItem(hwnd, IDC_EDIT_MACHINE);
  TCHAR szBuffer[8];
  GetWindowText(hEditMachine, szBuffer, 8);
  DialogBoxParam(GetModuleHandle(NULL),
                 MAKEINTRESOURCE(IDD_DIALOG_MACHINE_TYPE), hwnd,
                 MachineTypeProc, (LPARAM)szBuffer);
}

void onMachineTypeInit(HWND hwnd, LPARAM lParam) {
  TCHAR szTitle[MAX_PATH];
  GetWindowText(hwnd, szTitle, sizeof(szTitle) / sizeof(*szTitle));
  _tcscat_s(szTitle, TEXT(" -- "));
  _tcscat_s(szTitle, (PTCHAR)lParam);
  SetWindowText(hwnd, szTitle);

  HWND hMachineType = NULL;
  hMachineType = GetDlgItem(hwnd, IDC_COMBO_MACHINE_TYPE);

  // ComboBox_AddString(hMachineType, TEXT("123"));
  // same as
  // SendMessage(hMachineType, CB_ADDSTRING, 0, (LPARAM)TEXT("123"));
  for (int i = 0; i < wMachineTypeLength; i++) {
    ComboBox_AddString(hMachineType, szMachineTypeDesc[i]);
  }

  DWORD index = 0;
  _stscanf_s((PTCHAR)lParam, TEXT("%x"), &index);
  ComboBox_SetCurSel(hMachineType, machinetypeToIndex[index]);
}

void setCharacteristics(HWND hwnd, LPARAM lParam) {
  DWORD Characteristics = 0x0;
  _stscanf_s((PTCHAR)lParam, TEXT("%x"), &Characteristics);
  for (DWORD i = characteristicsBegin, index = 0; i <= characteristicsEnd;) {
    if (i & Characteristics) {
      Button_SetCheck(GetDlgItem(hwnd, characteristicsCheckBoxID[index]),
                      BST_CHECKED);
    }
    i <<= 1;
    index++;
  }
}

void onCharacteristics(HWND hwnd) {
  HWND hCharacteristics = GetDlgItem(hwnd, IDC_EDIT_CHARACTERISTICS);
  TCHAR szBuffer[8];
  GetWindowText(hCharacteristics, szBuffer, 8);
  DialogBoxParam(GetModuleHandle(NULL),
                 MAKEINTRESOURCE(IDD_DIALOG_CHARACTERISTICS), hwnd,
                 CharacteristicsProc, (LPARAM)szBuffer);
}

void onCharacteristicsInit(HWND hwnd, LPARAM lParam) {
  TCHAR szTitle[MAX_PATH];
  GetWindowText(hwnd, szTitle, sizeof(szTitle) / sizeof(*szTitle));
  _tcscat_s(szTitle, TEXT(" -- "));
  _tcscat_s(szTitle, (PTCHAR)lParam);
  SetWindowText(hwnd, szTitle);
  setCharacteristics(hwnd, lParam);
}

void handleCheckBoxesChecked(HWND hwnd, INT checkBoxID) {
  TCHAR szTitle[MAX_PATH];
  GetWindowText(hwnd, szTitle, sizeof(szTitle) / sizeof(*szTitle));
  DWORD Characteristics = 0x0;
  _stscanf_s(szTitle, TEXT("Characteristics -- %x"), &Characteristics);
  for (DWORD i = characteristicsBegin, index = 0; i <= characteristicsEnd;) {
    if (characteristicsCheckBoxID[index] == checkBoxID) {
      // checkbox checked before WM_COMMAND Message
      if (IsDlgButtonChecked(hwnd, checkBoxID) == BST_CHECKED) {
        Characteristics |= i;
      } else {
        Characteristics ^= i;
      }
      
      break;
    }
    i <<= 1;
    index++;
  }
  wsprintf(szTitle, TEXT("Characteristics -- %x"), Characteristics);
  SetWindowText(hwnd, szTitle);
}