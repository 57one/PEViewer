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
    initPE();
    DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_PE_EDITOR),
                   hwnd, (DLGPROC)PeEditorProc, (LPARAM)szBuffer);
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
      (DLGPROC)FileHeaderProc);
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
      (DLGPROC)MachineTypeProc, (LPARAM)szBuffer);
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

void onMachineTypeChange(HWND hwnd, HWND hCombo) {
  WORD index = ComboBox_GetCurSel(hCombo);
  WORD machineType = wMachineType[index];

  TCHAR szTitle[MAX_PATH];
  wsprintf(szTitle, TEXT("Machine Type -- %04X"), machineType);
  SetWindowText(hwnd, szTitle);
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
      (DLGPROC)CharacteristicsProc, (LPARAM)szBuffer);
}

void onHeaderInfoCharacteristics(HWND hwnd) {
  HWND hCharacteristics = GetDlgItem(hwnd, IDC_EDIT_HEADER_CHARACTERISTICS);
  TCHAR szBuffer[8];
  GetWindowText(hCharacteristics, szBuffer, 8);
  DialogBoxParam(GetModuleHandle(NULL),
                 MAKEINTRESOURCE(IDD_DIALOG_CHARACTERISTICS), hwnd,
      (DLGPROC)CharacteristicsProc, (LPARAM)szBuffer);
}

void onCharacteristicsInit(HWND hwnd, LPARAM lParam) {
  TCHAR szTitle[MAX_PATH];
  GetWindowText(hwnd, szTitle, sizeof(szTitle) / sizeof(*szTitle));
  _tcscat_s(szTitle, TEXT(" -- "));
  _tcscat_s(szTitle, (PTCHAR)lParam);
  SetWindowText(hwnd, szTitle);
  setCharacteristics(hwnd, lParam);
}

void handlCharacCheckBoxesChecked(HWND hwnd, INT checkBoxID) {
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

void initDateTimePicker(HWND hwnd, LPARAM lParam) {
  HWND hDateTimeHours = GetDlgItem(hwnd, IDC_DATETIMEPICKER_HOURS);
  HWND hDateTimeYears = GetDlgItem(hwnd, IDC_DATETIMEPICKER_YEARS);
  DWORD seconds = 0x0;
  _stscanf_s((PTCHAR)lParam, TEXT("%X"), &seconds);
  SYSTEMTIME sysTime = secondsToSystemTime(seconds);
  DateTime_SetSystemtime(hDateTimeHours, GDT_VALID, &sysTime);
  DateTime_SetSystemtime(hDateTimeYears, GDT_VALID, &sysTime);
}

void setDateTime(HWND hwnd, SYSTEMTIME sysTime) {
  DWORD seconds = systemTimeToSeconds(sysTime);
  HWND hTimeDateStamp = GetDlgItem(hwnd, IDC_EDIT_TIMEDATESTAMP_MORE_INFO);
  TCHAR szSeconds[10] = {0};
  wsprintf(szSeconds, TEXT("%X"), seconds);
  SetWindowText(hTimeDateStamp, szSeconds);

  TCHAR szTitle[MAX_PATH];
  wsprintf(szTitle, TEXT("Time/Date Stamp(GMT) -- %08X"), seconds);
  SetWindowText(hwnd, szTitle);
}

void onTimeDateStamp(HWND hwnd) {
  HWND hTimeDateStamp = GetDlgItem(hwnd, IDC_EDIT_TIMEDATESTAMP);
  TCHAR szBuffer[10] = {0};
  GetWindowText(hTimeDateStamp, szBuffer, 10);
  DialogBoxParam(GetModuleHandle(NULL),
                 MAKEINTRESOURCE(IDD_DIALOG_TIMEDATESTAMP), hwnd,
      (DLGPROC)TimeDateStampProc, (LPARAM)szBuffer);
}

void onHeaderInfoTimeDateStamp(HWND hwnd) {
  HWND hTimeDateStamp = GetDlgItem(hwnd, IDC_EDIT_HEADER_TIMEDATESTAMP);
  TCHAR szBuffer[10] = {0};
  GetWindowText(hTimeDateStamp, szBuffer, 10);
  DialogBoxParam(GetModuleHandle(NULL),
                 MAKEINTRESOURCE(IDD_DIALOG_TIMEDATESTAMP), hwnd,
      (DLGPROC)TimeDateStampProc, (LPARAM)szBuffer);
}

void onTimeDateStampInit(HWND hwnd, LPARAM lParam) {
  TCHAR szTitle[MAX_PATH];
  GetWindowText(hwnd, szTitle, sizeof(szTitle) / sizeof(*szTitle));
  _tcscat_s(szTitle, TEXT(" -- "));
  _tcscat_s(szTitle, (PTCHAR)lParam);
  SetWindowText(hwnd, szTitle);
  SetWindowText(GetDlgItem(hwnd, IDC_EDIT_TIMEDATESTAMP_MORE_INFO),
                (PTCHAR)lParam);
  initDateTimePicker(hwnd, lParam);
}

void onOptinalHeader(HWND hwnd) {
  DialogBox(GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDD_DIALOG_IMAGE_OPTIONAL_HEADER32), hwnd,
      (DLGPROC)OptionalHeader32Proc);
}

void onOptinalHeaderInit(HWND hwnd, LPARAM lParam) {
  // set text about optinal header
  readOptinalHeader32(hwnd, pFileBuffer);
}

void onMagicType(HWND hwnd) {
  HWND hEditMagic = GetDlgItem(hwnd, IDC_EDIT_MAGIC);
  TCHAR szBuffer[8];
  GetWindowText(hEditMagic, szBuffer, 8);
  DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_MAGIC_TYPE),
                 hwnd, (DLGPROC)MagicTypeProc, (LPARAM)szBuffer);
}

void onHeaderInfoMagicType(HWND hwnd) {
  HWND hEditMagic = GetDlgItem(hwnd, IDC_EDIT_HEADER_MAGIC);
  TCHAR szBuffer[8];
  GetWindowText(hEditMagic, szBuffer, 8);
  DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_MAGIC_TYPE),
                 hwnd, (DLGPROC)MagicTypeProc, (LPARAM)szBuffer);
}

void onMagicTypeInit(HWND hwnd, LPARAM lParam) {
  TCHAR szTitle[MAX_PATH];
  GetWindowText(hwnd, szTitle, sizeof(szTitle) / sizeof(*szTitle));
  _tcscat_s(szTitle, TEXT(" -- "));
  _tcscat_s(szTitle, (PTCHAR)lParam);
  SetWindowText(hwnd, szTitle);

  HWND hMagicType = NULL;
  hMagicType = GetDlgItem(hwnd, IDC_COMBO_MAGIC_TYPE);

  for (int i = 0; i < wMagicTypeLength; i++) {
    ComboBox_AddString(hMagicType, szMagicTypeDesc[i]);
  }

  DWORD index = 0;
  _stscanf_s((PTCHAR)lParam, TEXT("%x"), &index);
  ComboBox_SetCurSel(hMagicType, magicTypeToIndex[index]);
}

void onMagicTypeChange(HWND hwnd, HWND hCombo) {
  WORD index = ComboBox_GetCurSel(hCombo);
  WORD magicType = wMagicType[index];

  TCHAR szTitle[MAX_PATH];
  wsprintf(szTitle, TEXT("Magic Type -- %04X"), magicType);
  SetWindowText(hwnd, szTitle);
}

void checkHeader32Image(HWND hwnd) {
  DWORD sizeOfImage, sectionAlignment, remainder;
  HWND hEditHeader32Image, hsectionAlignment;
  hEditHeader32Image = GetDlgItem(hwnd, IDC_EDIT_SIZE_OF_IMAGE);
  hsectionAlignment = GetDlgItem(hwnd, IDC_EDIT_SECTION_ALIGNMENT);
  TCHAR szBuffer[10];
  GetWindowText(hEditHeader32Image, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &sizeOfImage);
  GetWindowText(hsectionAlignment, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &sectionAlignment);
  remainder = sectionAlignment == 0 ? 0 : sizeOfImage % sectionAlignment;
  sizeOfImage = sizeOfImage < pOptionalHeader32->SizeOfImage
                    ? pOptionalHeader32->SizeOfImage
                    : sizeOfImage - remainder;
  memset(szBuffer, 0, sizeof(szBuffer));
  wsprintf(szBuffer, TEXT("%08X"), sizeOfImage);
  SetWindowText(hEditHeader32Image, szBuffer);
}

void checkHeaderInfo32Image(HWND hwnd) {
  DWORD sizeOfImage, sectionAlignment, remainder;
  HWND hEditHeader32Image, hsectionAlignment;
  hEditHeader32Image = GetDlgItem(hwnd, IDC_EDIT_HEADER_SIZE_OF_IMAGE);
  hsectionAlignment = GetDlgItem(hwnd, IDC_EDIT_HEADER_SECTION_ALIGNMENT);
  TCHAR szBuffer[10];
  GetWindowText(hEditHeader32Image, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &sizeOfImage);
  GetWindowText(hsectionAlignment, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &sectionAlignment);
  remainder = sectionAlignment == 0 ? 0 : sizeOfImage % sectionAlignment;
  sizeOfImage = sizeOfImage < pOptionalHeader32->SizeOfImage
                    ? pOptionalHeader32->SizeOfImage
                    : sizeOfImage - remainder;
  memset(szBuffer, 0, sizeof(szBuffer));
  wsprintf(szBuffer, TEXT("%08X"), sizeOfImage);
  SetWindowText(hEditHeader32Image, szBuffer);
}

void addHeader32Image(HWND hwnd) {
  checkHeader32Image(hwnd);
  DWORD sizeOfImage, sectionAlignment;
  HWND hEditHeader32Image, hsectionAlignment;
  hEditHeader32Image = GetDlgItem(hwnd, IDC_EDIT_SIZE_OF_IMAGE);
  hsectionAlignment = GetDlgItem(hwnd, IDC_EDIT_SECTION_ALIGNMENT);
  TCHAR szBuffer[10];
  GetWindowText(hEditHeader32Image, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &sizeOfImage);
  GetWindowText(hsectionAlignment, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &sectionAlignment);
  sizeOfImage += sectionAlignment;
  wsprintf(szBuffer, TEXT("%08X"), sizeOfImage);
  SetWindowText(hEditHeader32Image, szBuffer);
}

void addHeaderInfo32Image(HWND hwnd) {
  checkHeaderInfo32Image(hwnd);
  DWORD sizeOfImage, sectionAlignment;
  HWND hEditHeader32Image, hsectionAlignment;
  hEditHeader32Image = GetDlgItem(hwnd, IDC_EDIT_HEADER_SIZE_OF_IMAGE);
  hsectionAlignment = GetDlgItem(hwnd, IDC_EDIT_HEADER_SECTION_ALIGNMENT);
  TCHAR szBuffer[10];
  GetWindowText(hEditHeader32Image, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &sizeOfImage);
  GetWindowText(hsectionAlignment, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &sectionAlignment);
  sizeOfImage += sectionAlignment;
  wsprintf(szBuffer, TEXT("%08X"), sizeOfImage);
  SetWindowText(hEditHeader32Image, szBuffer);
}

void checkHeader32Headers(HWND hwnd) {
  DWORD sizeOfHeaders, fileAlignment, remainder;
  HWND hEditHeader32Headers, hfileAlignment;
  hEditHeader32Headers = GetDlgItem(hwnd, IDC_EDIT_SIZE_OF_HEADERS);
  hfileAlignment = GetDlgItem(hwnd, IDC_EDIT_FILE_ALIGNMENT);
  TCHAR szBuffer[10];
  GetWindowText(hEditHeader32Headers, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &sizeOfHeaders);
  GetWindowText(hfileAlignment, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &fileAlignment);
  remainder = fileAlignment == 0 ? 0 : sizeOfHeaders % fileAlignment;
  sizeOfHeaders = sizeOfHeaders < pOptionalHeader32->SizeOfHeaders
                      ? pOptionalHeader32->SizeOfHeaders
                      : sizeOfHeaders - remainder;
  memset(szBuffer, 0, sizeof(szBuffer));
  wsprintf(szBuffer, TEXT("%08X"), sizeOfHeaders);
  SetWindowText(hEditHeader32Headers, szBuffer);
}

void addHeader32Headers(HWND hwnd) {
  checkHeader32Headers(hwnd);
  DWORD sizeOfHeaders, fileAlignment, remainder;
  HWND hEditHeader32Headers, hfileAlignment;
  hEditHeader32Headers = GetDlgItem(hwnd, IDC_EDIT_SIZE_OF_HEADERS);
  hfileAlignment = GetDlgItem(hwnd, IDC_EDIT_FILE_ALIGNMENT);
  TCHAR szBuffer[10];
  GetWindowText(hEditHeader32Headers, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &sizeOfHeaders);
  GetWindowText(hfileAlignment, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &fileAlignment);
  sizeOfHeaders += fileAlignment;
  wsprintf(szBuffer, TEXT("%08X"), sizeOfHeaders);
  SetWindowText(hEditHeader32Headers, szBuffer);
}

void checkHeader32CheckSum(HWND hwnd) {}

void checkHeaderInfo32CheckSum(HWND hwnd) {}

void onSubsystem(HWND hwnd) {
  HWND hEditSubsystem = GetDlgItem(hwnd, IDC_EDIT_SUB_STM);
  TCHAR szBuffer[8];
  GetWindowText(hEditSubsystem, szBuffer, 8);
  DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_SUBSYSTEM),
                 hwnd, (DLGPROC)SubsystemProc, (LPARAM)szBuffer);
}

void onHeaderInfoSubsystem(HWND hwnd) {
  HWND hEditSubsystem = GetDlgItem(hwnd, IDC_EDIT_HEADER_SUB_STM);
  TCHAR szBuffer[8];
  GetWindowText(hEditSubsystem, szBuffer, 8);
  DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_SUBSYSTEM),
                 hwnd, (DLGPROC)SubsystemProc, (LPARAM)szBuffer);
}

void onSubsystemInit(HWND hwnd, LPARAM lParam) {
  TCHAR szTitle[MAX_PATH];
  GetWindowText(hwnd, szTitle, sizeof(szTitle) / sizeof(*szTitle));
  _tcscat_s(szTitle, TEXT(" -- "));
  _tcscat_s(szTitle, (PTCHAR)lParam);
  SetWindowText(hwnd, szTitle);

  HWND hSubsystem = NULL;
  hSubsystem = GetDlgItem(hwnd, IDC_COMBO_SUBSYSTEM);

  for (int i = 0; i < wSubsystemLength; i++) {
    ComboBox_AddString(hSubsystem, szSubsystemDesc[i]);
  }

  DWORD index = 0;
  _stscanf_s((PTCHAR)lParam, TEXT("%x"), &index);
  ComboBox_SetCurSel(hSubsystem, subsystemToIndex[index]);
}

void onSubsystemChange(HWND hwnd, HWND hCombo) {
  WORD index = ComboBox_GetCurSel(hCombo);
  WORD subsystem = wSubsystem[index];

  TCHAR szTitle[MAX_PATH];
  wsprintf(szTitle, TEXT("Subsystem -- %04X"), subsystem);
  SetWindowText(hwnd, szTitle);
}

void setDllCharacteristics(HWND hwnd, LPARAM lParam) {
  DWORD dllCharacteristics = 0x0;
  _stscanf_s((PTCHAR)lParam, TEXT("%x"), &dllCharacteristics);
  for (DWORD i = dllCharacteristicsBegin; i <= dllCharacteristicsEnd;) {
    if (i & dllCharacteristics) {
      Button_SetCheck(
          GetDlgItem(hwnd, dllCharacteristicsCheckBoxID[dllCharacToIndex[i]]),
          BST_CHECKED);
    }
    i <<= 1;
  }
}

void onDllCharacteristics(HWND hwnd) {
  HWND hDllCharacteristics = GetDlgItem(hwnd, IDC_EDIT_DLL_CHARAC);
  TCHAR szBuffer[8];
  GetWindowText(hDllCharacteristics, szBuffer, 8);
  DialogBoxParam(GetModuleHandle(NULL),
                 MAKEINTRESOURCE(IDD_DIALOG_DLLCHARACTERISTICS), hwnd,
      (DLGPROC)DllCharacteristicsProc, (LPARAM)szBuffer);
}

void onDllCharacteristicsInit(HWND hwnd, LPARAM lParam) {
  TCHAR szTitle[MAX_PATH];
  GetWindowText(hwnd, szTitle, sizeof(szTitle) / sizeof(*szTitle));
  _tcscat_s(szTitle, TEXT(" -- "));
  _tcscat_s(szTitle, (PTCHAR)lParam);
  SetWindowText(hwnd, szTitle);
  setDllCharacteristics(hwnd, lParam);
}

void handleDllCharacCheckBoxesChecked(HWND hwnd, INT checkBoxID) {
  TCHAR szTitle[MAX_PATH];
  GetWindowText(hwnd, szTitle, sizeof(szTitle) / sizeof(*szTitle));
  DWORD dllCharacteristics = 0x0;
  _stscanf_s(szTitle, TEXT("DllCharacteristics -- %x"), &dllCharacteristics);
  for (DWORD i = dllCharacteristicsBegin; i <= dllCharacteristicsEnd;) {
    if (dllCharacteristicsCheckBoxID[dllCharacToIndex[i]] == checkBoxID) {
      // checkbox checked before WM_COMMAND Message
      if (IsDlgButtonChecked(hwnd, checkBoxID) == BST_CHECKED) {
        dllCharacteristics |= i;
      } else {
        dllCharacteristics ^= i;
      }

      break;
    }
    i <<= 1;
  }
  wsprintf(szTitle, TEXT("DllCharacteristics -- %X"), dllCharacteristics);
  SetWindowText(hwnd, szTitle);
}

void addHeader32NumRvaSize(HWND hwnd) {
  DWORD numOfRva;
  HWND hEditnumOfRva;
  hEditnumOfRva = GetDlgItem(hwnd, IDC_EDIT_NUM_OF_RVA_SIZES);
  TCHAR szBuffer[10];
  GetWindowText(hEditnumOfRva, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &numOfRva);
  if (numOfRva < 16) numOfRva += 1;
  wsprintf(szBuffer, TEXT("%08X"), numOfRva);
  SetWindowText(hEditnumOfRva, szBuffer);
}

void addHeaderInfo32NumRvaSize(HWND hwnd) {
  DWORD numOfRva;
  HWND hEditnumOfRva;
  hEditnumOfRva = GetDlgItem(hwnd, IDC_EDIT_HEADER_NUM_OF_RVA_SIZES);
  TCHAR szBuffer[10];
  GetWindowText(hEditnumOfRva, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &numOfRva);
  if (numOfRva < 16) numOfRva += 1;
  wsprintf(szBuffer, TEXT("%08X"), numOfRva);
  SetWindowText(hEditnumOfRva, szBuffer);
}

void minusHeader32NumRvaSize(HWND hwnd) {
  DWORD numOfRva;
  HWND hEditnumOfRva;
  hEditnumOfRva = GetDlgItem(hwnd, IDC_EDIT_NUM_OF_RVA_SIZES);
  TCHAR szBuffer[10];
  GetWindowText(hEditnumOfRva, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &numOfRva);
  if (numOfRva > 0) numOfRva -= 1;
  wsprintf(szBuffer, TEXT("%08X"), numOfRva);
  SetWindowText(hEditnumOfRva, szBuffer);
}

void minusHeaderInfo32NumRvaSize(HWND hwnd) {
  DWORD numOfRva;
  HWND hEditnumOfRva;
  hEditnumOfRva = GetDlgItem(hwnd, IDC_EDIT_HEADER_NUM_OF_RVA_SIZES);
  TCHAR szBuffer[10];
  GetWindowText(hEditnumOfRva, szBuffer, 10);
  _stscanf_s(szBuffer, TEXT("%x"), &numOfRva);
  if (numOfRva > 0) numOfRva -= 1;
  wsprintf(szBuffer, TEXT("%08X"), numOfRva);
  SetWindowText(hEditnumOfRva, szBuffer);
}

void onHeaderInfo(HWND hwnd) {
  DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_HEADER_INFO),
            hwnd, (DLGPROC)HeaderInfoProc);
}

void onHeaderInfoInit(HWND hwnd, LPARAM lParam) {
  // set text about [Header Info]
  readHeaderInfo(hwnd, pFileBuffer);
}

void onSection(HWND hwnd) {
  DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_SECTION), hwnd,
      (DLGPROC)SectionProc);
}

VOID InitSectionListView(HWND hwnd) {
  LV_COLUMN lv;
  HWND hListSections;

  //初始化
  memset(&lv, 0, sizeof(LV_COLUMN));
  //获取IDC_LIST_PROCESS句柄 (List的句柄)
  hListSections = GetDlgItem(hwnd, IDC_LIST_SECTIONS);
  //设置整行选中
  //点击某行的时候 显示整行选中
  SendMessage(hListSections, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT,
              LVS_EX_FULLROWSELECT);

  //第一列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("#"));
  lv.cx = 20;
  lv.iSubItem = 0;
  SendMessage(hListSections, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
  //第二列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("Name"));
  lv.cx = 60;
  lv.iSubItem = 1;
  // ListView_InsertColumn(hListSections, 0, &lv);
  SendMessage(hListSections, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
  //第三列
  lv.pszText = const_cast<wchar_t*>(TEXT("Virtual Size"));
  lv.cx = 80;
  lv.iSubItem = 2;
  SendMessage(hListSections, LVM_INSERTCOLUMN, 2, (DWORD)&lv);
  //第四列
  lv.pszText = const_cast<wchar_t*>(TEXT("Virtual Offset"));
  lv.cx = 80;
  lv.iSubItem = 3;
  // ListView_InsertColumn(hListSections, 1, &lv);
  SendMessage(hListSections, LVM_INSERTCOLUMN, 3, (DWORD)&lv);
  //第五列
  lv.pszText = const_cast<wchar_t*>(TEXT("Raw Size"));
  lv.cx = 80;
  lv.iSubItem = 4;
  ListView_InsertColumn(hListSections, 4, &lv);
  //第六列
  lv.pszText = const_cast<wchar_t*>(TEXT("Raw Offset"));
  lv.cx = 80;
  lv.iSubItem = 5;
  ListView_InsertColumn(hListSections, 5, &lv);
  //第七列
  lv.pszText = const_cast<wchar_t*>(TEXT("Characteristics"));
  lv.cx = 80;
  lv.iSubItem = 6;
  ListView_InsertColumn(hListSections, 6, &lv);

  readSections(hwnd, hListSections);
}

void onSectionInit(HWND hwnd, LPARAM lParam) { InitSectionListView(hwnd); }

void onDirectory(HWND hwnd) {
  DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_DIRECTORY), hwnd,
      (DLGPROC)DirectoryProc);
}

void onDirectoryInit(HWND hwnd, LPARAM lParam) {
  // set text about Directory
  readDirectory(hwnd);
}

void onSectionEdit(HWND hwnd, LPARAM lParam) {
  DialogBoxParam(GetModuleHandle(NULL),
                 MAKEINTRESOURCE(IDD_DIALOG_SECTION_EDIT), hwnd,
      (DLGPROC)SectionEditProc, lParam);
}

void onSectionEditInit(HWND hwnd, LPARAM lParam) {
  LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;
  readSectionEdit(hwnd, lpnmitem->iItem);
}

void onSectionCharacteristics(HWND hwnd) {
  HWND hEditectionCharacteristics =
      GetDlgItem(hwnd, IDC_EDIT_SECTION_CHARACTERISTICS);
  TCHAR szBuffer[10];
  GetWindowText(hEditectionCharacteristics, szBuffer, 10);
  DialogBoxParam(GetModuleHandle(NULL),
                 MAKEINTRESOURCE(IDD_DIALOG_SECTION_CHARACTERISTICS), hwnd,
      (DLGPROC)SectionCharacteristicsProc, (LPARAM)szBuffer);
}

void setSectionCharacteristics(HWND hwnd, LPARAM lParam) {
  DWORD SectionCharacteristics = 0x0;
  _stscanf_s((PTCHAR)lParam, TEXT("%x"), &SectionCharacteristics);
  for (DWORD i = 1; i < sectionCharacteristicsSize; i++) {
    if (sectionCharacterisValues[i] & SectionCharacteristics) {
      _tprintf(TEXT("------\n"));
      if (sectionCharacToIndex[sectionCharacterisValues[i]] != 0) {
        Button_SetCheck(
            GetDlgItem(hwnd,
                       sectionCharacteristicsCheckBoxID
                           [sectionCharacToIndex[sectionCharacterisValues[i]]]),
            BST_CHECKED);
      }
    }
  }
}

void onSectionCharacteristicsInit(HWND hwnd, LPARAM lParam) {
  setWindowTitleWithValue(hwnd, (PTCHAR)lParam);
  setSectionCharacteristics(hwnd, lParam);

  HWND hAlignBytes = NULL;
  hAlignBytes = GetDlgItem(hwnd, IDC_COMBO_ALIGN_BYTES);

  for (int i = 0; i < wSectionAlignLength; i++) {
    ComboBox_AddString(hAlignBytes, szSectionAlignBytesDesc[i]);
  }

  /* DWORD index = 0;
  _stscanf_s((PTCHAR)lParam, TEXT("%x"), &index);
  ComboBox_SetCurSel(hAlignBytes, machinetypeToIndex[index]);*/
}

BOOL onDirectoryButton(HWND hwnd, WPARAM wParam) {
  switch (LOWORD(wParam)) {
    case IDC_BUTTON_DIRECTORY_IMPORT: {
      DialogBox(GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDD_DIALOG_IMPORT_DIRECTORY), hwnd,
          (DLGPROC)ImportDirectoryProc);
      return TRUE;
    }
    case IDC_BUTTON_DIRECTORY_EXPORT: {
      DialogBox(GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDD_DIALOG_EXPORT_DIRECTORY), hwnd,
          (DLGPROC)ExportDirectoryProc);
      return TRUE;
    }
    case IDC_BUTTON_DIRECTORY_DEBUG: {
      DialogBox(GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDD_DIALOG_DEBUG_DIRECTORY), hwnd,
          (DLGPROC)DebugDirectoryProc);
      return TRUE;
    }
    case IDC_BUTTON_DIRECTORY_BOUND_IMPORT: {
      DialogBox(GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDD_DIALOG_BOUND_IMPORT_DIRECTORY), hwnd,
          (DLGPROC)BoundImportDirectoryProc);
      return TRUE;
    }
    default:
      return FALSE;
  }
}

VOID InitImportDirectory(HWND hwnd) {
  LV_COLUMN lv;
  HWND hListImportDLL;

  //初始化
  memset(&lv, 0, sizeof(LV_COLUMN));
  //获取IDC_LIST_IMPORT_DLL句柄 (List的句柄)
  hListImportDLL = GetDlgItem(hwnd, IDC_LIST_IMPORT_DLL);
  //设置整行选中
  //点击某行的时候 显示整行选中
  SendMessage(hListImportDLL, LVM_SETEXTENDEDLISTVIEWSTYLE,
              LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

  //第一列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("DLL Name"));
  lv.cx = 80;
  lv.iSubItem = 0;
  SendMessage(hListImportDLL, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
  //第二列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("Characteristics/OriginalFirstThunk"));
  lv.cx = 180;
  lv.iSubItem = 1;
  // ListView_InsertColumn(hListImportDLL, 0, &lv);
  SendMessage(hListImportDLL, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
  //第三列
  lv.pszText = const_cast<wchar_t*>(TEXT("TimeDateStamp"));
  lv.cx = 100;
  lv.iSubItem = 2;
  SendMessage(hListImportDLL, LVM_INSERTCOLUMN, 2, (DWORD)&lv);
  //第四列
  lv.pszText = const_cast<wchar_t*>(TEXT("ForwarderChain"));
  lv.cx = 90;
  lv.iSubItem = 3;
  // ListView_InsertColumn(hListImportDLL, 1, &lv);
  SendMessage(hListImportDLL, LVM_INSERTCOLUMN, 3, (DWORD)&lv);
  //第五列
  lv.pszText = const_cast<wchar_t*>(TEXT("Name(address)"));
  lv.cx = 90;
  lv.iSubItem = 4;
  ListView_InsertColumn(hListImportDLL, 4, &lv);
  //第六列
  lv.pszText = const_cast<wchar_t*>(TEXT("FirstThunk"));
  lv.cx = 70;
  lv.iSubItem = 5;
  ListView_InsertColumn(hListImportDLL, 5, &lv);

  readImportDirectory(hwnd, hListImportDLL);
}

VOID InitImportFunction(HWND hwnd) {
  LV_COLUMN lv;
  HWND hListIntThunk;

  //初始化
  memset(&lv, 0, sizeof(LV_COLUMN));
  //获取IDC_LIST_IMPORT_DLL句柄 (List的句柄)
  hListIntThunk = GetDlgItem(hwnd, IDC_LIST_INT_THUNK);
  //设置整行选中
  //点击某行的时候 显示整行选中
  SendMessage(hListIntThunk, LVM_SETEXTENDEDLISTVIEWSTYLE,
              LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

  //第一列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("API Name"));
  lv.cx = 150;
  lv.iSubItem = 0;
  SendMessage(hListIntThunk, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
  //第二列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("Thunk RVA"));
  lv.cx = 80;
  lv.iSubItem = 1;
  // ListView_InsertColumn(hListIntThunk, 0, &lv);
  SendMessage(hListIntThunk, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
  //第三列
  lv.pszText = const_cast<wchar_t*>(TEXT("Thunk RAW(FOA)"));
  lv.cx = 100;
  lv.iSubItem = 2;
  SendMessage(hListIntThunk, LVM_INSERTCOLUMN, 2, (DWORD)&lv);
  //第四列
  lv.pszText = const_cast<wchar_t*>(TEXT("Thunk Value"));
  lv.cx = 80;
  lv.iSubItem = 3;
  // ListView_InsertColumn(hListIntThunk, 1, &lv);
  SendMessage(hListIntThunk, LVM_INSERTCOLUMN, 3, (DWORD)&lv);
  //第五列
  lv.pszText = const_cast<wchar_t*>(TEXT("Hint"));
  lv.cx = 80;
  lv.iSubItem = 4;
  ListView_InsertColumn(hListIntThunk, 4, &lv);

  // Int import name table
  //readIntThunk(hwnd, hListIntThunk, (PIMAGE_IMPORT_DESCRIPTOR)0);
}

void onImportDirectoryInit(HWND hwnd) {
  InitImportDirectory(hwnd);
  InitImportFunction(hwnd);
}

VOID InitExportFunction(HWND hwnd) {
  LV_COLUMN lv;
  HWND hListExportFunc;

  //初始化
  memset(&lv, 0, sizeof(LV_COLUMN));
  //获取IDC_LIST_EXPORT_FUNC句柄 (List的句柄)
  hListExportFunc = GetDlgItem(hwnd, IDC_LIST_EXPORT_FUNC);
  SendMessage(hListExportFunc, LVM_SETEXTENDEDLISTVIEWSTYLE,
              LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

  //第一列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("Ordinal"));
  lv.cx = 80;
  lv.iSubItem = 0;
  SendMessage(hListExportFunc, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
  //第二列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("Func RVA"));
  lv.cx = 80;
  lv.iSubItem = 1;
  // ListView_InsertColumn(hListExportFunc, 0, &lv);
  SendMessage(hListExportFunc, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
  //第三列
  lv.pszText = const_cast<wchar_t*>(TEXT("Func RAW(FOA)"));
  lv.cx = 100;
  lv.iSubItem = 2;
  SendMessage(hListExportFunc, LVM_INSERTCOLUMN, 2, (DWORD)&lv);
  //第四列
  lv.pszText = const_cast<wchar_t*>(TEXT("Func Name"));
  lv.cx = 150;
  lv.iSubItem = 3;
  // ListView_InsertColumn(hListExportFunc, 1, &lv);
  SendMessage(hListExportFunc, LVM_INSERTCOLUMN, 3, (DWORD)&lv);

  readExportFunc(hwnd, hListExportFunc);
}

void onExportDirectoryInit(HWND hwnd) { 
    InitExportFunction(hwnd); 
    readExportDirectory(hwnd);
}

VOID InitBoundImportDirectory(HWND hwnd) {
  LV_COLUMN lv;
  HWND hBoundImport;

  //初始化
  memset(&lv, 0, sizeof(LV_COLUMN));
  //获取IDC_LIST_IMPORT_DLL句柄 (List的句柄)
  hBoundImport = GetDlgItem(hwnd, IDC_LIST_BOUND_IMPORT_DIRECTORY);
  //设置整行选中
  //点击某行的时候 显示整行选中
  SendMessage(hBoundImport, LVM_SETEXTENDEDLISTVIEWSTYLE,
              LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

  //第一列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("DLL Name"));
  lv.cx = 80;
  lv.iSubItem = 0;
  SendMessage(hBoundImport, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
  //第二列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("TimeDateStamp"));
  lv.cx = 100;
  lv.iSubItem = 1;
  // ListView_InsertColumn(hBoundImport, 0, &lv);
  SendMessage(hBoundImport, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
  //第三列
  lv.pszText = const_cast<wchar_t*>(TEXT("OffsetModuleName"));
  lv.cx = 100;
  lv.iSubItem = 2;
  SendMessage(hBoundImport, LVM_INSERTCOLUMN, 2, (DWORD)&lv);
  //第四列
  lv.pszText = const_cast<wchar_t*>(TEXT("NumberOfModuleForwarderRefs"));
  lv.cx = 150;
  lv.iSubItem = 3;
  // ListView_InsertColumn(hBoundImport, 1, &lv);
  SendMessage(hBoundImport, LVM_INSERTCOLUMN, 3, (DWORD)&lv);

  readBoundImportDirectory(hwnd, hBoundImport);
}

void onBoundImportDirectoryInit(HWND hwnd) { InitBoundImportDirectory(hwnd); }

VOID InitDebugDirectory(HWND hwnd) {
  LV_COLUMN lv;
  HWND hDebug;

  //初始化
  memset(&lv, 0, sizeof(LV_COLUMN));
  //获取IDC_LIST_IMPORT_DLL句柄 (List的句柄)
  hDebug = GetDlgItem(hwnd, IDC_LIST_DEBUG_DIRECTORY);
  //设置整行选中
  //点击某行的时候 显示整行选中
  SendMessage(hDebug, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT,
              LVS_EX_FULLROWSELECT);

  //第一列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("Characteristics"));
  lv.cx = 90;
  lv.iSubItem = 0;
  SendMessage(hDebug, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
  //第二列
  lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lv.pszText = const_cast<wchar_t*>(TEXT("TimeDateStamp"));
  lv.cx = 90;
  lv.iSubItem = 1;
  // ListView_InsertColumn(hDebug, 1, &lv);
  SendMessage(hDebug, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
  //第三列
  lv.pszText = const_cast<wchar_t*>(TEXT("MajorVersion"));
  lv.cx = 80;
  lv.iSubItem = 2;
  // ListView_InsertColumn(hDebug, 2, &lv);
  SendMessage(hDebug, LVM_INSERTCOLUMN, 3, (DWORD)&lv);

  lv.pszText = const_cast<wchar_t*>(TEXT("MinorVersion"));
  lv.cx = 80;
  lv.iSubItem = 3;
  SendMessage(hDebug, LVM_INSERTCOLUMN, 3, (DWORD)&lv);

  lv.pszText = const_cast<wchar_t*>(TEXT("Type"));
  lv.cx = 80;
  lv.iSubItem = 4;
  SendMessage(hDebug, LVM_INSERTCOLUMN, 4, (DWORD)&lv);

  lv.pszText = const_cast<wchar_t*>(TEXT("SizeOfData"));
  lv.cx = 80;
  lv.iSubItem = 5;
  SendMessage(hDebug, LVM_INSERTCOLUMN, 7, (DWORD)&lv);

  lv.pszText = const_cast<wchar_t*>(TEXT("AddressOfRawData"));
  lv.cx = 110;
  lv.iSubItem = 6;
  SendMessage(hDebug, LVM_INSERTCOLUMN, 6, (DWORD)&lv);

  lv.pszText = const_cast<wchar_t*>(TEXT("PointerToRawData"));
  lv.cx = 110;
  lv.iSubItem = 7;
  SendMessage(hDebug, LVM_INSERTCOLUMN, 7, (DWORD)&lv);

  readDebugDirectory(hwnd, hDebug);
}

void onDebugDirectoryInit(HWND hwnd) { InitDebugDirectory(hwnd); }