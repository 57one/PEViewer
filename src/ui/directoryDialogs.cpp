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

std::wstring SymbolText(const ImportSymbol& symbol) {
  if (symbol.kind == ImportKind::Name) return Widen(symbol.name);
  if (symbol.kind == ImportKind::Ordinal) return L"Ordinal #" + std::to_wstring(symbol.ordinal);
  return L"-";
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
    Cell(list, row, 5, SymbolText(entry.symbol));
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

void showDirectory(HWND hwnd, const PEContext& context) {
  if (!context.IsValid()) return;
  for (DWORD i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; ++i) {
    const auto entry = i < context.directoryCount ? context.dataDirectories + i : NULL;
    SetDlgItemText(hwnd, directoryRVAEditID[i], Hex(entry ? entry->VirtualAddress : 0).c_str());
    SetDlgItemText(hwnd, directorySizeEditID[i], Hex(entry ? entry->Size : 0).c_str());
    BOOL present = context.Directory(i) != NULL;
    EnableWindow(GetDlgItem(hwnd, directoryMoreInfoButtonID[i]), present);
    EnableWindow(GetDlgItem(hwnd, directoryHexButtonID[i]), present);
  }
}

void showImportDirectory(HWND hwnd, HWND list, const PEContext& context) {
  ListView_DeleteAllItems(list);
  ListView_DeleteAllItems(GetDlgItem(hwnd, IDC_LIST_INT_THUNK));
  const ImportDetails details = ReadImportDetails(context);
  for (SIZE_T i = 0; i < details.modules.size(); ++i) {
    const auto& module = details.modules[i];
    const auto& descriptor = module.descriptor;
    int row = (int)i;
    Cell(list, row, 0, Widen(module.name));
    Cell(list, row, 1, Hex(descriptor.OriginalFirstThunk));
    Cell(list, row, 2, Hex(descriptor.TimeDateStamp));
    Cell(list, row, 3, Hex(descriptor.ForwarderChain));
    Cell(list, row, 4, Hex(descriptor.Name));
    Cell(list, row, 5, Hex(descriptor.FirstThunk));
    LVITEM item = {};
    item.mask = LVIF_PARAM;
    item.iItem = row;
    item.lParam = module.descriptorIndex;
    ListView_SetItem(list, &item);
  }
  std::wstring status = std::to_wstring(details.modules.size()) + L" imported DLLs. " + details.message;
  SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS, status.c_str());
}

void showImportThunks(HWND hwnd, HWND dllList, HWND list, const PEContext& context) {
  ListView_DeleteAllItems(list);
  int selected = ListView_GetNextItem(dllList, -1, LVNI_SELECTED);
  if (selected < 0) {
    SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS, L"Select an imported DLL.");
    return;
  }
  LVITEM item = {};
  item.mask = LVIF_PARAM;
  item.iItem = selected;
  if (!ListView_GetItem(dllList, &item)) return;
  const ImportDetails imports = ReadImportDetails(context);
  const ImportModule* module = NULL;
  for (const auto& candidate : imports.modules)
    if ((LPARAM)candidate.descriptorIndex == item.lParam) { module = &candidate; break; }
  if (!module) {
    SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS,
                   imports.message.empty() ? L"Selected import descriptor is unavailable." : imports.message.c_str());
    return;
  }
  const auto details = ReadImportThunks(context, module->descriptor);
  bool wide = context.format == PEFormat::PE32Plus;
  for (SIZE_T i = 0; i < details.entries.size(); ++i) {
    const auto& entry = details.entries[i];
    int row = (int)i;
    Cell(list, row, 0, SymbolText(entry.symbol));
    Cell(list, row, 1, Hex(entry.rva, 10));
    Cell(list, row, 2, Hex(entry.fileOffset));
    Cell(list, row, 3, Hex(entry.value, wide ? 16 : 8));
    Cell(list, row, 4, entry.symbol.kind == ImportKind::Name ? Hex(entry.symbol.hint, 4) : L"-");
  }
  Fit(list, 5);
  std::wstring status = std::to_wstring(details.entries.size()) + L" imports. ";
  if (!imports.message.empty()) status += imports.message + L" ";
  status += details.message;
  if (details.fallback) status += L" FirstThunk fallback (INT missing).";
  SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS, status.c_str());
}

void showExportDirectory(HWND hwnd, const PEContext& context) {
  const ExportDetails details = ReadExportDetails(context);
  const auto& header = details.header;
  const struct { int id; DWORD value; int width; } fields[] = {
    {IDC_EDIT_EXPORT_DIRECTORY_CHARAC, header.Characteristics, 8},
    {IDC_EDIT_EXPORT_DIRECTORY_TIMEDATESTAMP, header.TimeDateStamp, 8},
    {IDC_EDIT_EXPORT_DIRECTORY_MAJ_VER, header.MajorVersion, 4},
    {IDC_EDIT_EXPORT_DIRECTORY_MIN_VER, header.MinorVersion, 4},
    {IDC_EDIT_EXPORT_DIRECTORY_NAME, header.Name, 8},
    {IDC_EDIT_EXPORT_DIRECTORY_BASE, header.Base, 8},
    {IDC_EDIT_EXPORT_DIRECTORY_NUM_OF_FUNCS, header.NumberOfFunctions, 8},
    {IDC_EDIT_EXPORT_DIRECTORY_NUM_OF_NAMES, header.NumberOfNames, 8},
    {IDC_EDIT_EXPORT_DIRECTORY_ADDR_OF_FUNCS, header.AddressOfFunctions, 8},
    {IDC_EDIT_EXPORT_DIRECTORY_ADDR_OF_NAMES, header.AddressOfNames, 8},
    {IDC_EDIT_EXPORT_DIRECTORY_ADDR_OF_NAME_ORDS, header.AddressOfNameOrdinals, 8}
  };
  for (const auto& field : fields)
    SetDlgItemText(hwnd, field.id, details.hasHeader ? Hex(field.value, field.width).c_str() : L"");
  HWND list = GetDlgItem(hwnd, IDC_LIST_EXPORT_FUNC);
  ListView_DeleteAllItems(list);
  SIZE_T forwarders = 0;
  for (SIZE_T i = 0; i < details.entries.size(); ++i) {
    const auto& entry = details.entries[i];
    int row = (int)i;
    Cell(list, row, 0, Hex(entry.ordinal, 4));
    Cell(list, row, 1, Hex(entry.rva));
    Cell(list, row, 2, entry.hasFileOffset ? Hex(entry.fileOffset) : L"-");
    Cell(list, row, 3, Widen(entry.name));
    Cell(list, row, 4, entry.isForwarder ? L"Forwarder" : L"Export");
    Cell(list, row, 5, Widen(entry.forwarder));
    Cell(list, row, 6, entry.message.empty() ? L"OK" : entry.message);
    if (entry.isForwarder) ++forwarders;
  }
  Fit(list, 7);
  std::wstring status = std::to_wstring(details.entries.size()) + L" exports (" +
      std::to_wstring(forwarders) + L" forwarders). " + details.message;
  SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS, status.c_str());
}

void showBoundImportDirectory(HWND hwnd, HWND list, const PEContext& context) {
  const BoundImportDetails details = ReadBoundImportDetails(context);
  ListView_DeleteAllItems(list);
  for (SIZE_T i = 0; i < details.entries.size(); ++i) {
    const auto& entry = details.entries[i];
    int row = (int)i;
    Cell(list, row, 0, Widen(entry.name));
    Cell(list, row, 1, Hex(entry.descriptor.TimeDateStamp) + L" H");
    Cell(list, row, 2, Hex(entry.descriptor.OffsetModuleName, 4) + L" H");
    Cell(list, row, 3, Hex(entry.descriptor.NumberOfModuleForwarderRefs, 4) + L" H");
  }
  std::wstring status = std::to_wstring(details.entries.size()) + L" bound imports. " + details.message;
  SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS, status.c_str());
}

void showDebugDirectory(HWND hwnd, HWND list, const PEContext& context) {
  const DebugDetails details = ReadDebugDetails(context);
  ListView_DeleteAllItems(list);
  for (SIZE_T i = 0; i < details.entries.size(); ++i) {
    const auto& entry = details.entries[i];
    int row = (int)i;
    LPCTSTR type = entry.Type < 7 ? szDebugInfoType[entry.Type] :
                   entry.Type == IMAGE_DEBUG_TYPE_BORLAND ? TEXT("BORLAND") : TEXT("UNKNOWN");
    Cell(list, row, 0, Hex(entry.Characteristics));
    Cell(list, row, 1, Hex(entry.TimeDateStamp));
    Cell(list, row, 2, Hex(entry.MajorVersion, 4));
    Cell(list, row, 3, Hex(entry.MinorVersion, 4));
    Cell(list, row, 4, std::wstring(type) + L"(" + Hex(entry.Type, 1) + L")");
    Cell(list, row, 5, Hex(entry.SizeOfData));
    Cell(list, row, 6, Hex(entry.AddressOfRawData));
    Cell(list, row, 7, Hex(entry.PointerToRawData));
  }
  std::wstring status = std::to_wstring(details.entries.size()) + L" debug records. " + details.message;
  SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS, status.c_str());
}

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
