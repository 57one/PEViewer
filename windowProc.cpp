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
      // to do
      // fix bug of complie in x64 mode
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
                    hwnd, (DLGPROC)AboutProc);
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
          return TRUE;
        }
        case IDC_BUTTON_OPTION_HEADER: {
          onOptinalHeader(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_HEADER_INFO: {
          onHeaderInfo(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_SECTION: {
          onSection(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_DIRECTORY: {
          onDirectory(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_TOOLS: {
          return TRUE;
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
        case IDC_BUTTON_FILE_HEADER_MACHINE: {
          onMachineType(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_TIMEDATESTAMP: {
          onTimeDateStamp(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_FILE_HEADER_CHARACTERISTICS: {
          onCharacteristics(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_FILE_HEADER_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK MachineTypeProc(HWND hwnd, UINT message, WPARAM wParam,
                              LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onMachineTypeInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_MACHINE_TYPE_OK: {
          return TRUE;
        }
        case IDC_BUTTON_MACHINE_TYPE_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
        case IDC_COMBO_MACHINE_TYPE: {
          if (HIWORD(wParam) == CBN_SELCHANGE) {
            onMachineTypeChange(hwnd, (HWND)lParam);
          }
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK CharacteristicsProc(HWND hwnd, UINT message, WPARAM wParam,
                                  LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onCharacteristicsInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_CHARACTERISTICS_OK: {
          return TRUE;
        }
        case IDC_BUTTON_CHARACTERISTICS_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
        default: {
          handlCharacCheckBoxesChecked(hwnd, LOWORD(wParam));
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK TimeDateStampProc(HWND hwnd, UINT message, WPARAM wParam,
                                LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onTimeDateStampInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_TIMEDATESTAMP_OK: {
          return TRUE;
        }
        case IDC_BUTTON_TIMEDATESTAMP_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
      }
    }
    case WM_NOTIFY: {
      switch (LOWORD(wParam)) {
        case IDC_DATETIMEPICKER_HOURS:
        case IDC_DATETIMEPICKER_YEARS: {
          // here should use reinterpret_cast to cast
          // but at first i forgot it(c++ rookie) and for backward compatibility
          NMHDR* pnmhdr = (NMHDR*)lParam;
          if (pnmhdr != NULL && pnmhdr->code == DTN_DATETIMECHANGE) {
            LPNMDATETIMECHANGE lpChange = (LPNMDATETIMECHANGE)lParam;
            if (lpChange->dwFlags & GDT_NONE) {
            } else {
              setDateTime(hwnd, lpChange->st);
            }
          }
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK OptionalHeader32Proc(HWND hwnd, UINT message, WPARAM wParam,
                                   LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onOptinalHeaderInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_OPTIONAL_HEADER32_MAGIC: {
          onMagicType(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_OPTIONAL_HEADER32_OK: {
          return TRUE;
        }
        case IDC_BUTTON_OPTIONAL_HEADER32_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
        case IDC_BUTTON_OPTIONAL_HEADER32_ENTRY: {
          return TRUE;
        }
        case IDC_BUTTON_OPTIONAL_HEADER32_SIZE_OF_IMAGE: {
          checkHeader32Image(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_OPTIONAL_HEADER32_IMAGE_ADD: {
          addHeader32Image(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_OPTIONAL_HEADER32_SIZE_OF_HEADERS: {
          checkHeader32Headers(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_OPTIONAL_HEADER32_HEADER_ADD: {
          addHeader32Headers(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_OPTIONAL_HEADER32_CHECKSUM: {
          checkHeader32CheckSum(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_OPTIONAL_HEADER32_SUBSTM: {
          onSubsystem(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_OPTIONAL_HEADER32_DLL_CHARAC: {
          onDllCharacteristics(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_OPTIONAL_HEADER32_NUM_RVA_SIZE_ADD: {
          addHeader32NumRvaSize(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_OPTIONAL_HEADER32_NUM_RVA_SIZE_MINUS: {
          minusHeader32NumRvaSize(hwnd);
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK MagicTypeProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onMagicTypeInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_MAGIC_TYPE_OK: {
          return TRUE;
        }
        case IDC_BUTTON_MAGIC_TYPE_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
        case IDC_COMBO_MAGIC_TYPE: {
          if (HIWORD(wParam) == CBN_SELCHANGE) {
            onMagicTypeChange(hwnd, (HWND)lParam);
          }
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK SubsystemProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onSubsystemInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_SUBSYSTEM_OK: {
          return TRUE;
        }
        case IDC_BUTTON_SUBSYSTEM_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
        case IDC_COMBO_SUBSYSTEM: {
          if (HIWORD(wParam) == CBN_SELCHANGE) {
            onSubsystemChange(hwnd, (HWND)lParam);
          }
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK DllCharacteristicsProc(HWND hwnd, UINT message, WPARAM wParam,
                                     LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onDllCharacteristicsInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_DLLCHARACTERISTICS_OK: {
          return TRUE;
        }
        case IDC_BUTTON_DLLCHARACTERISTICS_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
        default: {
          handleDllCharacCheckBoxesChecked(hwnd, LOWORD(wParam));
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK HeaderInfoProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onHeaderInfoInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_HEADER_ENTRY: {
          return TRUE;
        }
        case IDC_BUTTON_HEADER_SIZE_OF_IMAGE: {
          checkHeaderInfo32Image(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_HEADER_IMAGE_ADD: {
          addHeaderInfo32Image(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_HEADER_MAGIC: {
          onHeaderInfoMagicType(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_HEADER_SUBSTM: {
          onHeaderInfoSubsystem(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_HEADER_TIMEDATESTAMP: {
          onHeaderInfoTimeDateStamp(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_HEADER_CHARACTERISTICS: {
          onHeaderInfoCharacteristics(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_HEADER_CHECKSUM: {
          checkHeaderInfo32CheckSum(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_HEADER_NUM_RVA_SIZE_ADD: {
          addHeaderInfo32NumRvaSize(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_HEADER_NUM_RVA_SIZE_MINUS: {
          minusHeaderInfo32NumRvaSize(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_HEADER_OK: {
          return TRUE;
        }
        case IDC_BUTTON_HEADER_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK SectionProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onSectionInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_NOTIFY: {
      NMHDR* pNMHDR = (NMHDR*)lParam;
      // click Process in List
      if (wParam == IDC_LIST_SECTIONS && pNMHDR->code == NM_DBLCLK) {
        onSectionEdit(hwnd, lParam);
      }
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_SECTION_CANCEL : {
          EndDialog(hwnd, 0);
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK SectionEditProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onSectionEditInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_SECTION_CHARACTERISTICS: {
          onSectionCharacteristics(hwnd);
          return TRUE;
        }
        case IDC_BUTTON_SECTION_EDIT_OK: {
          return TRUE;
        }
        case IDC_BUTTON_SECTION_EDIT_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK SectionCharacteristicsProc(HWND hwnd, UINT message, WPARAM wParam,
                              LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onSectionCharacteristicsInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_SECTION_EDIT_OK: {
          return TRUE;
        }
        case IDC_BUTTON_SECTION_EDIT_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK DirectoryProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onDirectoryInit(hwnd, lParam);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_DIRECTORY_OK: {
          return TRUE;
        }
        case IDC_BUTTON_DIRECTORY_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
        default: {
          return onDirectoryButton(hwnd, wParam);
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK ImportDirectoryProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam) {
  switch (message) { 
    case WM_INITDIALOG: {
      onImportDirectoryInit(hwnd);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_IMPORT_DIRECTORY_CLOSE: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
      }
    }
    case WM_NOTIFY: {
      NMHDR* pNMHDR = (NMHDR*)lParam;
      // click DLL in List
      if (wParam == IDC_LIST_IMPORT_DLL && pNMHDR->code == NM_CLICK) {
        
        readIntThunk(hwnd, GetDlgItem(hwnd, IDC_LIST_IMPORT_DLL),
                     GetDlgItem(hwnd, IDC_LIST_INT_THUNK));
      }
      return TRUE;
    }
  }
  return FALSE;
}

BOOL CALLBACK ExportDirectoryProc(HWND hwnd, UINT message, WPARAM wParam,
                                  LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onExportDirectoryInit(hwnd);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_EXPORT_DIRECTORY_OK: {
          return TRUE;
        }
        case IDC_BUTTON_EXPORT_DIRECTORY_CANCEL: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK BoundImportDirectoryProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onBoundImportDirectoryInit(hwnd);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_BOUND_IMPORT_DIRECTORY_CLOSE: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}

BOOL CALLBACK DebugDirectoryProc(HWND hwnd, UINT message, WPARAM wParam,
    LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      onDebugDirectoryInit(hwnd);
      return TRUE;
    }
    case WM_CLOSE: {
      EndDialog(hwnd, 0);
      return TRUE;
    }
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_BUTTON_DEBUG_DIRECTORY_CLOSE: {
          EndDialog(hwnd, 0);
          return TRUE;
        }
      }
    }
    case WM_NOTIFY: {
        //NMHDR* pNMHDR = (NMHDR*)lParam;
        //// click DLL in List
        //if (wParam == IDC_LIST_IMPORT_DLL && pNMHDR->code == NM_CLICK) {

        //    readIntThunk(hwnd, GetDlgItem(hwnd, IDC_LIST_IMPORT_DLL),
        //        GetDlgItem(hwnd, IDC_LIST_INT_THUNK));
        //}
        return TRUE;
    }
  }
  return FALSE;
}