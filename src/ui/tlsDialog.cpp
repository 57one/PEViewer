#include "windowProc.h"
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
    column.cx = 120;
    ListView_InsertColumn(list, index++, &column);
  }
}

void Row(HWND list, std::initializer_list<std::wstring> values) {
  int column = 0, row = ListView_GetItemCount(list);
  for (const auto& value : values) {
    if (!column) {
      LVITEM item = {};
      item.mask = LVIF_TEXT;
      item.iItem = row;
      item.pszText = const_cast<wchar_t*>(value.c_str());
      ListView_InsertItem(list, &item);
    } else {
      ListView_SetItemText(list, row, column, const_cast<wchar_t*>(value.c_str()));
    }
    ++column;
  }
}

void FitColumns(HWND list) {
  int count = Header_GetItemCount(ListView_GetHeader(list));
  for (int i = 0; i < count; ++i) ListView_SetColumnWidth(list, i, LVSCW_AUTOSIZE_USEHEADER);
}

void AddressField(HWND list, const wchar_t* field, const TlsAddress& address, int width) {
  Row(list, {field, Hex(address.va, width), address.hasRva ? Hex(address.rva) : L"-",
             address.hasFileOffset ? Hex(address.fileOffset) : L"-",
             address.message.empty() ? L"OK" : address.message});
}
}  // namespace

void showTlsDirectory(HWND hwnd, const PEContext& context) {
  HWND fields = GetDlgItem(hwnd, IDC_LIST_TLS_FIELDS);
  HWND callbacks = GetDlgItem(hwnd, IDC_LIST_TLS_CALLBACKS);
  ListView_DeleteAllItems(fields);
  ListView_DeleteAllItems(callbacks);
  SetWindowText(hwnd, context.format == PEFormat::PE32Plus ? L"TLS Directory - PE32+" : L"TLS Directory - PE32");
  const auto details = ReadTlsDetails(context);
  int width = context.format == PEFormat::PE32Plus ? 16 : 8;
  if (details.hasHeader) {
    AddressField(fields, L"StartAddressOfRawData", details.rawDataStart, width);
    AddressField(fields, L"EndAddressOfRawData (exclusive)", details.rawDataEnd, width);
    AddressField(fields, L"AddressOfIndex", details.index, width);
    AddressField(fields, L"AddressOfCallBacks", details.callbackArray, width);
    Row(fields, {L"SizeOfZeroFill", Hex(details.sizeOfZeroFill), L"-", L"-", L"-"});
    Row(fields, {L"Characteristics", Hex(details.characteristics), L"-", L"-", L"-"});
    Row(fields, {L"Initialization template size", Hex(details.rawDataSize), L"-", L"-",
                  details.rawDataValid ? L"Valid" : L"Invalid"});
    Row(fields, {L"ImageBase", Hex(details.imageBase, width), L"-", L"-", L"Preferred image base"});
  }
  for (SIZE_T i = 0; i < details.callbacks.size(); ++i) {
    const auto& entry = details.callbacks[i];
    const auto& target = entry.target;
    Row(callbacks, {std::to_wstring(i), Hex(entry.slotRva), Hex(entry.slotFileOffset), Hex(target.va, width),
                    target.hasRva ? Hex(target.rva) : L"-",
                    target.hasFileOffset ? Hex(target.fileOffset) : L"-",
                    target.message.empty() ? L"OK" : target.message});
  }
  FitColumns(fields);
  FitColumns(callbacks);
  std::wstring status = details.hasHeader ? std::to_wstring(details.callbacks.size()) +
      L" callbacks. " + (details.terminated ? L"Callback list complete. " : L"Callback list incomplete. ") : L"";
  status += details.message.empty() ? L"OK" : details.message;
  SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS, status.c_str());
}

INT_PTR CALLBACK TlsDirectoryProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM) {
  switch (message) {
    case WM_INITDIALOG:
      Columns(GetDlgItem(hwnd, IDC_LIST_TLS_FIELDS), {L"Field", L"Value (hex / VA)", L"RVA", L"File offset", L"Status"});
      Columns(GetDlgItem(hwnd, IDC_LIST_TLS_CALLBACKS),
              {L"Index", L"Slot RVA", L"Slot file offset", L"Callback VA", L"Target RVA", L"Target file offset", L"Status"});
      showTlsDirectory(hwnd, g_peContext);
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
