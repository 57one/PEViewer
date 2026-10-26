#include "windowProc.h"
#include "pe/directoryDetails.h"

#include <initializer_list>
#include <memory>
#include <string>

namespace {
struct DelayPage {
  const PEContext* context = NULL;
  DelayImportDetails details;
};

std::wstring Hex(ULONGLONG value, int width = 8) {
  wchar_t text[32] = {};
  swprintf_s(text, L"%0*I64X", width, value);
  return text;
}

std::wstring Widen(const std::string& value) {
  return std::wstring(value.begin(), value.end());  // PE import names are ASCII.
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

void Row(HWND list, std::initializer_list<std::wstring> values, LPARAM identity = 0) {
  int row = ListView_GetItemCount(list), column = 0;
  for (const auto& value : values) {
    if (!column) {
      LVITEM item = {};
      item.mask = LVIF_TEXT | LVIF_PARAM;
      item.iItem = row;
      item.pszText = const_cast<wchar_t*>(value.c_str());
      item.lParam = identity;
      ListView_InsertItem(list, &item);
    } else {
      ListView_SetItemText(list, row, column, const_cast<wchar_t*>(value.c_str()));
    }
    ++column;
  }
}

void Fit(HWND list) {
  int count = Header_GetItemCount(ListView_GetHeader(list));
  for (int i = 0; i < count; ++i) {
    ListView_SetColumnWidth(list, i, LVSCW_AUTOSIZE);
    int content = ListView_GetColumnWidth(list, i);
    ListView_SetColumnWidth(list, i, LVSCW_AUTOSIZE_USEHEADER);
    int heading = ListView_GetColumnWidth(list, i);
    int width = content > heading ? content : heading;
    ListView_SetColumnWidth(list, i, width > 480 ? 480 : width);
  }
}

std::wstring Address(DWORD raw, const DelayImportAddress& address, bool rvaBased) {
  if (!raw) return L"-";
  std::wstring text = Hex(raw);
  if (!rvaBased && address.hasRva) text += L" -> " + Hex(address.rva);
  return text;
}

std::wstring Symbol(const ImportSymbol& symbol) {
  if (symbol.kind == ImportKind::Name) return Widen(symbol.name);
  if (symbol.kind == ImportKind::Ordinal) return L"Ordinal #" + std::to_wstring(symbol.ordinal);
  return L"-";
}

std::wstring Value(const DelayImportSlot& slot, int width) {
  return slot.hasValue ? Hex(slot.value, width) : L"-";
}

void ShowFunctions(HWND hwnd) {
  HWND functions = GetDlgItem(hwnd, IDC_LIST_DELAY_FUNCTIONS);
  ListView_DeleteAllItems(functions);
  SetDlgItemText(hwnd, IDC_STATIC_DELAY_FUNCTION_STATUS, L"");
  auto page = (DelayPage*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  if (!page || !page->context) return;
  HWND modules = GetDlgItem(hwnd, IDC_LIST_DELAY_MODULES);
  int row = ListView_GetNextItem(modules, -1, LVNI_SELECTED);
  if (row < 0) return;
  LVITEM item = {};
  item.mask = LVIF_PARAM;
  item.iItem = row;
  if (!ListView_GetItem(modules, &item) || item.lParam <= 0 ||
      (SIZE_T)item.lParam > page->details.modules.size()) return;
  const auto& module = page->details.modules[(SIZE_T)item.lParam - 1];
  const auto details = ReadDelayImportThunks(*page->context, module);
  int width = page->context->format == PEFormat::PE32Plus ? 16 : 8;
  for (SIZE_T i = 0; i < details.entries.size(); ++i) {
    const auto& entry = details.entries[i];
    Row(functions, {std::to_wstring(i), Symbol(entry.symbol),
        entry.symbol.kind == ImportKind::Name ? Hex(entry.symbol.hint, 4) : L"-",
        Hex(entry.lookup.rva), Hex(entry.lookup.fileOffset), Value(entry.lookup, width),
        entry.iat.hasRva ? Hex(entry.iat.rva) : L"-",
        entry.iat.hasValue ? Hex(entry.iat.fileOffset) : L"-", Value(entry.iat, width),
        Value(entry.boundIat, width), Value(entry.unloadIat, width),
        entry.message.empty() ? L"OK" : entry.message});
  }
  Fit(functions);
  std::wstring status = std::to_wstring(details.entries.size()) + L" functions. " +
      (details.terminated ? L"INT complete. " : L"INT incomplete. ");
  status += details.message.empty() ? L"OK" : details.message;
  SetDlgItemText(hwnd, IDC_STATIC_DELAY_FUNCTION_STATUS, status.c_str());
}
}  // namespace

void showDelayImportDirectory(HWND hwnd, const PEContext& context) {
  auto old = (DelayPage*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
  HWND modules = GetDlgItem(hwnd, IDC_LIST_DELAY_MODULES);
  ListView_DeleteAllItems(modules);
  ListView_DeleteAllItems(GetDlgItem(hwnd, IDC_LIST_DELAY_FUNCTIONS));
  SetDlgItemText(hwnd, IDC_STATIC_DELAY_FUNCTION_STATUS, L"");
  delete old;
  auto page = std::make_unique<DelayPage>();
  page->context = &context;
  page->details = ReadDelayImportDetails(context);
  SetWindowText(hwnd, context.format == PEFormat::PE32Plus ?
                L"Delay Import Directory - PE32+" : L"Delay Import Directory - PE32");
  for (SIZE_T i = 0; i < page->details.modules.size(); ++i) {
    const auto& module = page->details.modules[i];
    const auto& descriptor = module.descriptor;
    Row(modules, {module.name.empty() ? L"<invalid DLL name>" : Widen(module.name),
        Hex(module.descriptorRva), Hex(descriptor.Attributes.AllAttributes) +
          (module.supported ? (module.rvaBased ? L" (RVA)" : L" (VA)") : L" (unsupported)"),
        Address(descriptor.DllNameRVA, module.dllName, module.rvaBased),
        Address(descriptor.ImportNameTableRVA, module.lookup, module.rvaBased),
        Address(descriptor.ImportAddressTableRVA, module.iat, module.rvaBased),
        Address(descriptor.ModuleHandleRVA, module.moduleHandle, module.rvaBased),
        Address(descriptor.BoundImportAddressTableRVA, module.boundIat, module.rvaBased),
        Address(descriptor.UnloadInformationTableRVA, module.unloadIat, module.rvaBased),
        Hex(descriptor.TimeDateStamp), module.message.empty() ? L"OK" : module.message}, (LPARAM)(i + 1));
  }
  Fit(modules);
  std::wstring status = std::to_wstring(page->details.modules.size()) + L" DLLs. " +
      (page->details.terminated ? L"Descriptors complete. " : L"Descriptors incomplete. ");
  status += page->details.message.empty() ? L"OK" : page->details.message;
  SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS, status.c_str());
  bool selectFirst = !page->details.modules.empty();
  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)page.release());
  if (selectFirst) ListView_SetItemState(modules, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
}

INT_PTR CALLBACK DelayImportDirectoryProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG:
      Columns(GetDlgItem(hwnd, IDC_LIST_DELAY_MODULES), {L"DLL", L"Descriptor RVA", L"Attributes / mode",
          L"Name address", L"INT (raw / RVA)", L"IAT (raw / RVA)", L"Module handle",
          L"Bound IAT", L"Unload IAT", L"Timestamp", L"Status"});
      Columns(GetDlgItem(hwnd, IDC_LIST_DELAY_FUNCTIONS), {L"#", L"Function / ordinal", L"Hint",
          L"INT RVA", L"INT file offset", L"INT raw value", L"IAT RVA", L"IAT file offset",
          L"IAT raw value", L"Bound IAT value", L"Unload IAT value", L"Status"});
      showDelayImportDirectory(hwnd, g_peContext);
      return TRUE;
    case WM_NOTIFY: {
      const auto header = (const NMHDR*)lParam;
      if (header && header->idFrom == IDC_LIST_DELAY_MODULES && header->code == LVN_ITEMCHANGED) {
        const auto change = (const NMLISTVIEW*)lParam;
        if ((change->uChanged & LVIF_STATE) && ((change->uOldState ^ change->uNewState) & LVIS_SELECTED))
          ShowFunctions(hwnd);
      }
      break;
    }
    case WM_COMMAND:
      if (LOWORD(wParam) != IDOK && LOWORD(wParam) != IDCANCEL) break;
      EndDialog(hwnd, 0);
      return TRUE;
    case WM_CLOSE:
      EndDialog(hwnd, 0);
      return TRUE;
    case WM_NCDESTROY:
      delete (DelayPage*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
      SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
      break;
  }
  return FALSE;
}
