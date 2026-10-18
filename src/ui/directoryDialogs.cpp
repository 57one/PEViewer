#include "windowProc.h"

#include "pe/directoryDetails.h"

#include <string>

namespace {
std::wstring Hex(ULONGLONG value, int width = 8) {
  wchar_t text[32] = {};
  swprintf_s(text, L"%0*I64X", width, value);
  return text;
}

std::wstring Widen(const std::string& text) {
  if (text.empty()) return L"-";
  int size = MultiByteToWideChar(CP_OEMCP, 0, text.c_str(), -1, NULL, 0);
  if (!size) return L"<invalid text>";
  std::wstring result(size, L'\0');
  MultiByteToWideChar(CP_OEMCP, 0, text.c_str(), -1, &result[0], size);
  result.resize(size - 1);
  return result;
}

void Columns(HWND list, const wchar_t* const* names, const int* widths, int count) {
  ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
  for (int i = 0; i < count; ++i) {
    LVCOLUMN column = {};
    column.mask = LVCF_TEXT | LVCF_WIDTH;
    column.pszText = const_cast<wchar_t*>(names[i]);
    column.cx = widths[i];
    ListView_InsertColumn(list, i, &column);
  }
}

void Cell(HWND list, int row, int column, const std::wstring& value) {
  if (!column) {
    LVITEM item = {};
    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.pszText = const_cast<wchar_t*>(value.c_str());
    ListView_InsertItem(list, &item);
  } else {
    ListView_SetItemText(list, row, column, const_cast<wchar_t*>(value.c_str()));
  }
}

void Fit(HWND list, int count) {
  for (int i = 0; i < count; ++i) {
    ListView_SetColumnWidth(list, i, LVSCW_AUTOSIZE);
    int content = ListView_GetColumnWidth(list, i);
    ListView_SetColumnWidth(list, i, LVSCW_AUTOSIZE_USEHEADER);
    int heading = ListView_GetColumnWidth(list, i);
    // Users can widen a column to inspect exceptionally long names/status text.
    int width = content > heading ? content : heading;
    ListView_SetColumnWidth(list, i, width > 420 ? 420 : width);
  }
}

void InitIat(HWND hwnd) {
  bool wide = g_peContext.format == PEFormat::PE32Plus;
  SetWindowText(hwnd, wide ? L"IAT Details - PE32+ (disk file)" : L"IAT Details - PE32 (disk file)");
  HWND list = GetDlgItem(hwnd, IDC_LIST_IAT);
  const wchar_t* names[] = {L"#", L"IAT RVA", L"File offset", L"IAT raw value", L"DLL",
                           L"Function / ordinal", L"INT RVA", L"INT raw value", L"Status"};
  const int widths[] = {40, 85, 85, 135, 110, 150, 85, 135, 150};
  Columns(list, names, widths, _countof(names));
  IatDetails details = ReadIatDetails(g_peContext);
  SendMessage(list, WM_SETREDRAW, FALSE, 0);
  for (SIZE_T i = 0; i < details.entries.size(); ++i) {
    const IatEntry& entry = details.entries[i];
    int row = (int)i;
    Cell(list, row, 0, std::to_wstring(i));
    Cell(list, row, 1, Hex(entry.rva));
    Cell(list, row, 2, Hex(entry.fileOffset));
    Cell(list, row, 3, Hex(entry.value, wide ? 16 : 8));
    Cell(list, row, 4, Widen(entry.dll));
    Cell(list, row, 5, Widen(entry.symbol));
    Cell(list, row, 6, entry.hasLookup ? Hex(entry.lookupRva) : L"-");
    Cell(list, row, 7, entry.hasLookup ? Hex(entry.lookupValue, wide ? 16 : 8) : L"-");
    Cell(list, row, 8, entry.note.empty() ? L"OK" : entry.note);
  }
  Fit(list, _countof(names));
  SendMessage(list, WM_SETREDRAW, TRUE, 0);
  std::wstring status = std::to_wstring(details.entries.size()) +
      L" slots (including null/padding). Values are from the disk file. ";
  status += details.message.empty() ? L"Names are matched using the import descriptors and INT." : details.message;
  SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS, status.c_str());
}

void ShowRelocationEntries(HWND hwnd) {
  HWND list = GetDlgItem(hwnd, IDC_LIST_RELOC_ENTRIES);
  ListView_DeleteAllItems(list);
  auto details = (const RelocationDetails*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  int selected = ListView_GetNextItem(GetDlgItem(hwnd, IDC_LIST_RELOC_BLOCKS), -1, LVNI_SELECTED);
  if (!details || selected < 0 || (SIZE_T)selected >= details->blocks.size()) return;
  const auto& entries = details->blocks[selected].entries;
  SendMessage(list, WM_SETREDRAW, FALSE, 0);
  for (SIZE_T i = 0; i < entries.size(); ++i) {
    const auto& entry = entries[i];
    int row = (int)i;
    Cell(list, row, 0, std::to_wstring(i));
    Cell(list, row, 1, Hex(entry.raw, 4));
    Cell(list, row, 2, entry.typeName);
    Cell(list, row, 3, Hex(entry.offset, 3));
    Cell(list, row, 4, entry.hasTarget ? Hex(entry.targetRva) : L"-");
    Cell(list, row, 5, entry.hasFileOffset ? Hex(entry.fileOffset) : L"-");
    Cell(list, row, 6, entry.hasValue ? Hex(entry.value, entry.valueSize * 2) : L"-");
    Cell(list, row, 7, entry.hasAdjustment ? Hex((WORD)entry.adjustment, 4) : L"-");
    Cell(list, row, 8, entry.note.empty() ? L"OK" : entry.note);
  }
  Fit(list, 9);
  SendMessage(list, WM_SETREDRAW, TRUE, 0);
  InvalidateRect(list, NULL, TRUE);
}

void InitRelocations(HWND hwnd) {
  bool wide = g_peContext.format == PEFormat::PE32Plus;
  SetWindowText(hwnd, wide ? L"Base Relocations - PE32+" : L"Base Relocations - PE32");
  HWND blocks = GetDlgItem(hwnd, IDC_LIST_RELOC_BLOCKS);
  const wchar_t* blockNames[] = {L"#", L"Block RVA", L"File offset", L"Page RVA", L"SizeOfBlock", L"Entries"};
  const int blockWidths[] = {45, 100, 100, 100, 100, 80};
  Columns(blocks, blockNames, blockWidths, _countof(blockNames));
  const wchar_t* entryNames[] = {L"#", L"Raw WORD", L"Type", L"Offset", L"Target RVA",
                               L"File offset", L"Original value", L"HIGHADJ word", L"Status"};
  const int entryWidths[] = {40, 80, 130, 65, 90, 90, 135, 95, 130};
  Columns(GetDlgItem(hwnd, IDC_LIST_RELOC_ENTRIES), entryNames, entryWidths, _countof(entryNames));
  auto details = new RelocationDetails(ReadRelocationDetails(g_peContext));
  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)details);
  for (SIZE_T i = 0; i < details->blocks.size(); ++i) {
    const auto& block = details->blocks[i];
    int row = (int)i;
    Cell(blocks, row, 0, std::to_wstring(i));
    Cell(blocks, row, 1, Hex(block.rva));
    Cell(blocks, row, 2, Hex(block.fileOffset));
    Cell(blocks, row, 3, Hex(block.pageRva));
    Cell(blocks, row, 4, Hex(block.size));
    Cell(blocks, row, 5, std::to_wstring(block.entries.size()));
  }
  Fit(blocks, _countof(blockNames));
  std::wstring status = std::to_wstring(details->blocks.size()) + L" blocks. ";
  status += details->message.empty() ? L"Select a block to inspect its entries. Original values are from the disk file." :
                                      details->message;
  SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS, status.c_str());
  if (!details->blocks.empty()) ListView_SetItemState(blocks, 0, LVIS_SELECTED | LVIS_FOCUSED,
                                                    LVIS_SELECTED | LVIS_FOCUSED);
}
}  // namespace

INT_PTR CALLBACK IatDirectoryProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: InitIat(hwnd); return TRUE;
    case WM_CLOSE: EndDialog(hwnd, 0); return TRUE;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == IDOK) {
        EndDialog(hwnd, 0);
        return TRUE;
      }
      break;
  }
  return FALSE;
}

INT_PTR CALLBACK RelocationDirectoryProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: InitRelocations(hwnd); return TRUE;
    case WM_CLOSE: EndDialog(hwnd, 0); return TRUE;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == IDOK) {
        EndDialog(hwnd, 0);
        return TRUE;
      }
      break;
    case WM_NOTIFY: {
      auto notification = (const NMHDR*)lParam;
      if (notification && notification->idFrom == IDC_LIST_RELOC_BLOCKS &&
          notification->code == LVN_ITEMCHANGED) {
        ShowRelocationEntries(hwnd);
        return TRUE;
      }
      break;
    }
    case WM_DESTROY:
      delete (RelocationDetails*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
      SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
      return TRUE;
  }
  return FALSE;
}
