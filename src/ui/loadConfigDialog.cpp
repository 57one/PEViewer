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

void Columns(HWND list, std::initializer_list<const wchar_t*> names) {
  ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
  int index = 0;
  for (auto name : names) {
    LVCOLUMN column = {};
    column.mask = LVCF_TEXT | LVCF_WIDTH;
    column.pszText = const_cast<wchar_t*>(name);
    column.cx = 110;
    ListView_InsertColumn(list, index++, &column);
  }
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

void showLoadConfigDirectory(HWND hwnd, const PEContext& context) {
  HWND fields = GetDlgItem(hwnd, IDC_LIST_LOAD_CONFIG_FIELDS);
  HWND handlers = GetDlgItem(hwnd, IDC_LIST_SAFE_SEH);
  ScopedDirectoryUpdate fieldsUpdate(fields);
  ScopedDirectoryUpdate handlersUpdate(handlers);
  ListView_DeleteAllItems(fields);
  ListView_DeleteAllItems(handlers);
  SetWindowText(hwnd, context.format == PEFormat::PE32Plus ? L"Load Config Directory - PE32+" : L"Load Config Directory - PE32");
  const auto details = ReadLoadConfigDetails(context);
  for (const auto& field : details.fields) {
    Row(fields, {field.name, Hex(field.offset, 4), std::to_wstring(field.size),
        field.available ? Hex(field.value, field.size * 2) : L"-", field.isVa ? L"VA" : L"Value",
        field.hasRva ? Hex(field.rva) : L"-", field.hasFileOffset ? Hex(field.fileOffset) : L"-",
        field.message.empty() ? L"OK" : field.message});
  }
  const auto& safe = details.safeSeh;
  for (SIZE_T i = 0; i < safe.entries.size(); ++i) {
    const auto& entry = safe.entries[i];
    Row(handlers, {std::to_wstring(i), Hex(entry.slotRva), Hex(entry.slotFileOffset), Hex(entry.handlerRva),
        entry.hasVa ? Hex(entry.handlerVa) : L"-", entry.hasFileOffset ? Hex(entry.fileOffset) : L"-",
        entry.message.empty() ? L"OK" : entry.message});
  }
  FitDirectoryColumns(fields);
  FitDirectoryColumns(handlers);
  std::wstring status = L"Directory RVA: " + Hex(details.rva) + L", size: " + Hex(details.size);
  if (details.hasSize) status += L". Structure Size: " + Hex(details.declaredSize);
  status += L". " + (details.message.empty() ? std::wstring(L"OK") : details.message);
  SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS, status.c_str());
  std::wstring safeStatus = L"SafeSEH (x86): ";
  if (safe.hasFields) safeStatus += std::to_wstring(safe.entries.size()) + L" / " + std::to_wstring(safe.count) +
      L" handlers. " + (safe.complete ? L"Table complete. " : L"Table incomplete. ");
  safeStatus += safe.message.empty() ? (safe.hasFields ? L"OK" : L"Unavailable") : safe.message;
  SetDlgItemText(hwnd, IDC_STATIC_SAFE_SEH_STATUS, safeStatus.c_str());
}

INT_PTR CALLBACK LoadConfigDirectoryProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM) {
  switch (message) {
    case WM_INITDIALOG:
      InitDirectoryDialogLayout(hwnd, DirectoryDialogLayout::LoadConfig);
      Columns(GetDlgItem(hwnd, IDC_LIST_LOAD_CONFIG_FIELDS),
          {L"Field", L"Structure offset", L"Bytes", L"Raw value (hex)", L"Kind", L"Target RVA", L"Target file offset", L"Status"});
      Columns(GetDlgItem(hwnd, IDC_LIST_SAFE_SEH),
          {L"#", L"Slot RVA", L"Slot file offset", L"Handler RVA", L"Handler VA", L"Handler file offset", L"Status"});
      showLoadConfigDirectory(hwnd, g_peContext);
      return TRUE;
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
