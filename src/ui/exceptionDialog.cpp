#include "windowProc.h"
#include "directoryDialogUi.h"
#include "pe/directoryDetails.h"

#include <initializer_list>
#include <string>

namespace {
std::wstring Hex(ULONGLONG value, int width = 8) {
  wchar_t text[32] = {};
  swprintf_s(text, L"%0*I64X", width, value);
  return text;
}

void Row(HWND list, std::initializer_list<std::wstring> values) {
  int row = ListView_GetItemCount(list), column = 0;
  for (const auto& value : values) {
    if (!column) {
      LVITEM item = {};
      item.mask = LVIF_TEXT;
      item.iItem = row;
      item.pszText = const_cast<wchar_t*>(value.c_str());
      ListView_InsertItem(list, &item);
    } else ListView_SetItemText(list, row, column, const_cast<wchar_t*>(value.c_str()));
    ++column;
  }
}
}  // namespace

void showExceptionDirectory(HWND hwnd, const PEContext& context) {
  HWND list = GetDlgItem(hwnd, IDC_LIST_EXCEPTION_FUNCTIONS);
  ScopedDirectoryUpdate update(list);
  ListView_DeleteAllItems(list);
  const auto details = ReadExceptionDetails(context);
  SetWindowText(hwnd, details.supported ? L"Exception Directory - AMD64 (PE32+)" : L"Exception Directory");
  for (SIZE_T i = 0; i < details.entries.size(); ++i) {
    const auto& entry = details.entries[i];
    Row(list, {std::to_wstring(i), Hex(entry.recordRva), Hex(entry.recordFileOffset),
        Hex(entry.beginRva), Hex(entry.endRva), entry.rangeValid ? Hex(entry.endRva - entry.beginRva) : L"-",
        entry.hasCodeFileOffset ? Hex(entry.codeFileOffset) : L"-", Hex(entry.unwindRva),
        entry.hasUnwindFileOffset ? Hex(entry.unwindFileOffset) : L"-",
        entry.message.empty() ? L"OK" : entry.message});
  }
  FitDirectoryColumns(list);
  std::wstring status = L"Machine: " + Hex(details.machine, 4) + L". Directory RVA: " +
      Hex(details.rva) + L", size: " + Hex(details.size) + L". ";
  if (details.supported) status += std::to_wstring(details.entries.size()) + L" / " +
      std::to_wstring(details.totalEntries) + L" records. " + (details.complete ? L"Table complete. " : L"Table incomplete. ");
  status += details.message.empty() ? L"OK" : details.message;
  SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS, status.c_str());
}

INT_PTR CALLBACK ExceptionDirectoryProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM) {
  switch (message) {
    case WM_INITDIALOG: {
      InitDirectoryDialogLayout(hwnd, DirectoryDialogLayout::Exception);
      HWND list = GetDlgItem(hwnd, IDC_LIST_EXCEPTION_FUNCTIONS);
      ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
      const wchar_t* names[] = {L"#", L"Record RVA", L"Record file offset", L"Begin RVA", L"End RVA (exclusive)",
          L"Size", L"Code file offset", L"Unwind RVA", L"Unwind file offset", L"Status"};
      for (int i = 0; i < _countof(names); ++i) {
        LVCOLUMN column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.pszText = const_cast<wchar_t*>(names[i]);
        column.cx = 100;
        ListView_InsertColumn(list, i, &column);
      }
      showExceptionDirectory(hwnd, g_peContext);
      return TRUE;
    }
    case WM_COMMAND:
      if (LOWORD(wParam) != IDOK && LOWORD(wParam) != IDCANCEL) break;
      EndDialog(hwnd, 0);
      return TRUE;
    case WM_CLOSE:
      EndDialog(hwnd, 0);
      return TRUE;
  }
  return FALSE;
}
