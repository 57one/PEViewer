#include <windows.h>
#include <commctrl.h>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <stdexcept>
#include <vector>

#include "pe/readPE.h"
#include "pe/directoryDetails.h"
#include "ui/windowProc.h"
#include "fixtures/pe32.h"
#include "fixtures/pe64.h"

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

void CheckPEContext(LPCTSTR filePath);
void CheckDirectoryDetails();

namespace {
void Check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

struct Dialog {
  HWND hwnd = NULL;
  Dialog(int id, DLGPROC callback, LPARAM parameter = 0) {
    HINSTANCE instance = GetModuleHandle(NULL);
    HRSRC resource = FindResource(instance, MAKEINTRESOURCE(id), RT_DIALOG);
    Check(resource != NULL, "Dialog resource is missing");
    DWORD size = SizeofResource(instance, resource);
    const BYTE* source = (const BYTE*)LockResource(LoadResource(instance, resource));
    Check(source != NULL && size >= 16, "Dialog resource cannot be loaded");
    std::vector<BYTE> dialogTemplate(source, source + size);
    // Exercise the production callbacks without showing their windows.
    WORD signature = 0;
    std::memcpy(&signature, dialogTemplate.data() + 2, sizeof(signature));
    SIZE_T styleOffset = signature == 0xFFFF ? 12 : 0;
    DWORD style = 0;
    std::memcpy(&style, dialogTemplate.data() + styleOffset, sizeof(style));
    style &= ~WS_VISIBLE;
    std::memcpy(dialogTemplate.data() + styleOffset, &style, sizeof(style));
    hwnd = CreateDialogIndirectParam(instance, (LPCDLGTEMPLATE)dialogTemplate.data(),
                                     NULL, callback, parameter);
    Check(hwnd != NULL, "Dialog creation failed");
  }
  ~Dialog() { if (hwnd) DestroyWindow(hwnd); }
  Dialog(const Dialog&) = delete;
  Dialog& operator=(const Dialog&) = delete;
};

void CheckText(HWND dialog, int controlId, const wchar_t* expected) {
  wchar_t actual[MAX_PATH] = {};
  Check(GetDlgItem(dialog, controlId) != NULL, "Expected control is missing");
  GetDlgItemText(dialog, controlId, actual, MAX_PATH);
  if (std::wcscmp(actual, expected) != 0) {
    std::fwprintf(stderr, L"Control %d: expected [%ls], got [%ls]\n", controlId, expected, actual);
    throw std::runtime_error("Control value mismatch");
  }
}

void CheckList(HWND dialog, int controlId, int column, const wchar_t* expected,
               int row = 0, int count = 1) {
  HWND list = GetDlgItem(dialog, controlId);
  Check(list != NULL, "Expected list is missing");
  Check(ListView_GetItemCount(list) == count, "Unexpected list row count");
  wchar_t actual[MAX_PATH] = {};
  ListView_GetItemText(list, row, column, actual, MAX_PATH);
  if (std::wcscmp(actual, expected) != 0) {
    std::fwprintf(stderr, L"List %d column %d: expected [%ls], got [%ls]\n", controlId, column, expected, actual);
    throw std::runtime_error("List value mismatch");
  }
}

void CheckPages() {
  bool wide = g_peContext.format == PEFormat::PE32Plus;
  Dialog editor(IDD_DIALOG_PE_EDITOR, PeEditorProc, (LPARAM)L"synthetic.exe");
  wchar_t title[MAX_PATH] = {};
  GetWindowText(editor.hwnd, title, MAX_PATH);
  Check(std::wcsstr(title, wide ? L"[PE32+]" : L"[PE32]") != NULL,
        "Editor title has the wrong file format");
  CheckText(editor.hwnd, IDC_EDIT_MAGIC_NUMBER, L"5A4D");
  Dialog fileHeader(IDD_DIALOG_IMAGE_FILE_HEADER, FileHeaderProc);
  CheckText(fileHeader.hwnd, IDC_EDIT_NUMBER_OF_SECTIONS, L"0001");
  Dialog optionalHeader(IDD_DIALOG_IMAGE_OPTIONAL_HEADER32, OptionalHeader32Proc);
  CheckText(optionalHeader.hwnd, IDC_EDIT_MAGIC, wide ? L"020B" : L"010B");
  CheckText(optionalHeader.hwnd, IDC_EDIT_IMAGE_BASE, wide ? L"0000000140000000" : L"00400000");
  Dialog summary(IDD_DIALOG_HEADER_INFO, HeaderInfoProc);
  CheckText(summary.hwnd, IDC_EDIT_HEADER_SIZE_OF_IMAGE, L"00002000");
  Dialog sections(IDD_DIALOG_SECTION, SectionProc);
  CheckList(sections.hwnd, IDC_LIST_SECTIONS, 1, pe32_fixture::kSectionName);
  CheckList(sections.hwnd, IDC_LIST_SECTIONS, 5, L"00000200");
  Dialog directories(IDD_DIALOG_DIRECTORY, DirectoryProc);
  CheckText(directories.hwnd, IDC_EDIT_IMPORT_TABLE_RVA, L"00001200");
  Check(!IsWindowEnabled(GetDlgItem(directories.hwnd, IDC_BUTTON_DIRECTORY_RESOURCE)),
        "Missing resource directory should be disabled");

  Dialog imports(IDD_DIALOG_IMPORT_DIRECTORY, ImportDirectoryProc);
  CheckList(imports.hwnd, IDC_LIST_IMPORT_DLL, 0, pe32_fixture::kImportDll);
  HWND dllList = GetDlgItem(imports.hwnd, IDC_LIST_IMPORT_DLL);
  ListView_SetItemState(dllList, 0, LVIS_SELECTED, LVIS_SELECTED);
  // Dispatch the same notification as a user selecting an imported DLL.
  NMHDR notification = {dllList, IDC_LIST_IMPORT_DLL, NM_CLICK};
  SendMessage(imports.hwnd, WM_NOTIFY, IDC_LIST_IMPORT_DLL, (LPARAM)&notification);
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 0, pe32_fixture::kImportName, 0, wide ? 3 : 1);
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 4, L"1234", 0, wide ? 3 : 1);
  if (wide) {
    CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 3, L"0000000000001270", 0, 3);
    CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 0, L"Ordinal #42", 1, 3);
    CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 3, L"800000000000002A", 1, 3);
    CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 0, L"SecondFunc", 2, 3);
    CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 2, L"00000490", 2, 3);
    CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 4, L"5678", 2, 3);
    HWND thunks = GetDlgItem(imports.hwnd, IDC_LIST_INT_THUNK);
    Check(ListView_GetColumnWidth(thunks, 3) >=
          ListView_GetStringWidth(thunks, L"800000000000002A"),
          "The thunk value column clips 64-bit values");
  }
  Dialog exports(IDD_DIALOG_EXPORT_DIRECTORY, ExportDirectoryProc);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 3, pe32_fixture::kExportName);
  Dialog boundImports(IDD_DIALOG_BOUND_IMPORT_DIRECTORY, BoundImportDirectoryProc);
  CheckList(boundImports.hwnd, IDC_LIST_BOUND_IMPORT_DIRECTORY, 0, pe32_fixture::kBoundDll);
  Dialog debug(IDD_DIALOG_DEBUG_DIRECTORY, DebugDirectoryProc);
  CheckList(debug.hwnd, IDC_LIST_DEBUG_DIRECTORY, 4, L"CODEVIEW(2)");
  CheckList(debug.hwnd, IDC_LIST_DEBUG_DIRECTORY, 7, L"00000650");

  Dialog iat(IDD_DIALOG_IAT, IatDirectoryProc);
  int iatCount = wide ? 4 : 2;
  CheckList(iat.hwnd, IDC_LIST_IAT, 1, wide ? L"000012A0" : L"00001260", 0, iatCount);
  CheckList(iat.hwnd, IDC_LIST_IAT, 2, wide ? L"000004A0" : L"00000460", 0, iatCount);
  CheckList(iat.hwnd, IDC_LIST_IAT, 3, wide ? L"0000000000001270" : L"00001270", 0, iatCount);
  CheckList(iat.hwnd, IDC_LIST_IAT, 4, L"test.dll", 0, iatCount);
  CheckList(iat.hwnd, IDC_LIST_IAT, 5, L"ImportedFunc", 0, iatCount);
  CheckList(iat.hwnd, IDC_LIST_IAT, 6, wide ? L"00001280" : L"00001250", 0, iatCount);
  CheckList(iat.hwnd, IDC_LIST_IAT, 8, L"Terminator", iatCount - 1, iatCount);
  if (wide) {
    CheckList(iat.hwnd, IDC_LIST_IAT, 3, L"800000000000002A", 1, iatCount);
    HWND iatList = GetDlgItem(iat.hwnd, IDC_LIST_IAT);
    Check(ListView_GetColumnWidth(iatList, 3) >= ListView_GetStringWidth(iatList, L"800000000000002A"),
          "The IAT raw value column clips 64-bit values");
  }

  Dialog relocs(IDD_DIALOG_RELOCATIONS, RelocationDirectoryProc);
  CheckList(relocs.hwnd, IDC_LIST_RELOC_BLOCKS, 3, L"00001000", 0, 2);
  CheckList(relocs.hwnd, IDC_LIST_RELOC_ENTRIES, 2, wide ? L"DIR64 (10)" : L"HIGHLOW (3)", 0, 2);
  CheckList(relocs.hwnd, IDC_LIST_RELOC_ENTRIES, 4, L"00001800", 0, 2);
  CheckList(relocs.hwnd, IDC_LIST_RELOC_ENTRIES, 6, wide ? L"0000000140001800" : L"00401800", 0, 2);
  HWND blocks = GetDlgItem(relocs.hwnd, IDC_LIST_RELOC_BLOCKS);
  ListView_SetItemState(blocks, 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
  CheckList(relocs.hwnd, IDC_LIST_RELOC_ENTRIES, 2, L"ABSOLUTE (0)", 0, 2);
  CheckList(relocs.hwnd, IDC_LIST_RELOC_ENTRIES, 4, L"-", 0, 2);

  HBITMAP bitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP_PEVIEWER));
  Check(bitmap != NULL, "Bitmap resource is missing");
  DeleteObject(bitmap);
}

void CheckDirectoryErrorPages(void* buffer) {
  pe32_fixture::Fill((BYTE*)buffer);
  auto nt = (PIMAGE_NT_HEADERS32)((BYTE*)buffer + 0x80);
  nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = MAXDWORD;
  nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = 7;
  Check(g_peContext.Initialize(buffer, pe32_fixture::kFileSize), "Invalid-directory context rejected");
  Dialog iat(IDD_DIALOG_IAT, IatDirectoryProc);
  Dialog relocs(IDD_DIALOG_RELOCATIONS, RelocationDirectoryProc);
  Check(ListView_GetItemCount(GetDlgItem(iat.hwnd, IDC_LIST_IAT)) == 0,
        "Invalid IAT rendered rows");
  Check(ListView_GetItemCount(GetDlgItem(relocs.hwnd, IDC_LIST_RELOC_BLOCKS)) == 0,
        "Invalid relocation directory rendered rows");
  wchar_t status[512] = {};
  GetDlgItemText(iat.hwnd, IDC_STATIC_DIRECTORY_STATUS, status, _countof(status));
  Check(std::wcsstr(status, L"outside") != NULL, "IAT error is not visible in the page");
  GetDlgItemText(relocs.hwnd, IDC_STATIC_DIRECTORY_STATUS, status, _countof(status));
  Check(std::wcsstr(status, L"Truncated") != NULL, "Relocation error is not visible in the page");
}

void CheckRealDirectoryFile(const wchar_t* path) {
  PEContext context;
  Check(context.LoadFile(path), "Cannot load real PE file");
  IatDetails iat = ReadIatDetails(context);
  RelocationDetails relocs = ReadRelocationDetails(context);
  if (context.Directory(IMAGE_DIRECTORY_ENTRY_IAT)) {
    if (!iat.message.empty()) std::fwprintf(stderr, L"IAT: %ls\n", iat.message.c_str());
    Check(iat.message.empty() && !iat.entries.empty(), "Real-file IAT parsing failed");
  }
  if (context.Directory(IMAGE_DIRECTORY_ENTRY_BASERELOC)) {
    if (!relocs.message.empty()) std::fwprintf(stderr, L"Relocations: %ls\n", relocs.message.c_str());
    Check(relocs.message.empty() && !relocs.blocks.empty(), "Real-file relocation parsing failed");
  }
  SIZE_T entries = 0;
  for (const auto& block : relocs.blocks) entries += block.entries.size();
  SIZE_T functions = 0, unresolved = 0;
  for (const auto& slot : iat.entries) {
    if (!slot.value) continue;
    if (!slot.symbol.empty()) ++functions;
    else ++unresolved;
  }
  std::wprintf(L"PASS: %ls (%ls), IAT slots=%zu (functions=%zu, unresolved=%zu), relocation blocks=%zu, entries=%zu\n", path,
               context.format == PEFormat::PE32Plus ? L"PE32+" : L"PE32",
               iat.entries.size(), functions, unresolved, relocs.blocks.size(), entries);
}

void CheckContextDirectoryReads() {
  std::vector<BYTE> image(pe32_fixture::kFileSize);
  pe32_fixture::Fill(image.data());
  PEContext context;
  Check(context.Initialize(image.data(), image.size()), "Directory test context setup failed");

  // Page rendering must use the explicitly supplied context, not the active file.
  auto descriptor = (PIMAGE_IMPORT_DESCRIPTOR)(image.data() + 0x400);
  descriptor->OriginalFirstThunk = 0;
  *(DWORD*)(image.data() + 0x460) = IMAGE_ORDINAL_FLAG32 | 42;
  Dialog imports(IDD_DIALOG_IMPORT_DIRECTORY, ImportDirectoryProc);
  HWND dlls = GetDlgItem(imports.hwnd, IDC_LIST_IMPORT_DLL);
  HWND thunks = GetDlgItem(imports.hwnd, IDC_LIST_INT_THUNK);
  ListView_DeleteAllItems(dlls);
  readImportDirectory(imports.hwnd, dlls, context);
  ListView_SetItemState(dlls, 0, LVIS_SELECTED, LVIS_SELECTED);
  readIntThunk(imports.hwnd, dlls, thunks, context);
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 0, L"Ordinal #42");
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 2, L"00000460");

  auto exported = (PIMAGE_EXPORT_DIRECTORY)(image.data() + 0x300);
  exported->NumberOfNames = 0;
  exported->Base = 0x100;
  Dialog exports(IDD_DIALOG_EXPORT_DIRECTORY, ExportDirectoryProc);
  HWND functions = GetDlgItem(exports.hwnd, IDC_LIST_EXPORT_FUNC);
  ListView_DeleteAllItems(functions);
  readExportFunc(exports.hwnd, functions, context);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 0, L"0100");
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 2, L"00000A00");
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 3, L"-");

  ((PIMAGE_DEBUG_DIRECTORY)(image.data() + 0x600))->Type = IMAGE_DEBUG_TYPE_REPRO;
  Dialog debug(IDD_DIALOG_DEBUG_DIRECTORY, DebugDirectoryProc);
  HWND debugList = GetDlgItem(debug.hwnd, IDC_LIST_DEBUG_DIRECTORY);
  ListView_DeleteAllItems(debugList);
  readDebugDirectory(debug.hwnd, debugList, context);
  CheckList(debug.hwnd, IDC_LIST_DEBUG_DIRECTORY, 4, L"UNKNOWN(10)");

  auto nt = (PIMAGE_NT_HEADERS32)(image.data() + 0x80);
  nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = MAXDWORD;
  ListView_DeleteAllItems(dlls);
  readImportDirectory(imports.hwnd, dlls, context);
  Check(ListView_GetItemCount(dlls) == 0, "An invalid directory RVA was dereferenced");
}

void CheckWideControl(HWND dialog, int controlId) {
  HWND edit = GetDlgItem(dialog, controlId);
  RECT client = {};
  GetClientRect(edit, &client);
  HDC dc = GetDC(edit);
  HGDIOBJ previous = SelectObject(dc, (HFONT)SendMessage(edit, WM_GETFONT, 0, 0));
  SIZE extent = {};
  GetTextExtentPoint32(dc, L"FFFFFFFFFFFFFFFF", 16, &extent);
  SelectObject(dc, previous);
  ReleaseDC(edit, dc);
  DWORD margins = (DWORD)SendMessage(edit, EM_GETMARGINS, 0, 0);
  Check(client.right >= extent.cx + LOWORD(margins) + HIWORD(margins) + 2,
        "A 64-bit field requires horizontal scrolling");
}

void CheckValueEditor(HWND dialog, int controlId, const PEContext& context) {
  bool wide = context.format == PEFormat::PE32Plus;
  wchar_t original[MAX_PATH] = {};
  GetDlgItemText(dialog, controlId, original, MAX_PATH);
  Check(SendDlgItemMessage(dialog, controlId, EM_GETLIMITTEXT, 0, 0) == (wide ? 18 : 10),
        "Hex input limit does not match the file format");
  HWND edit = GetDlgItem(dialog, controlId);
  SendMessage(edit, EM_SETSEL, 0, -1);
  SendMessage(edit, EM_REPLACESEL, FALSE, (LPARAM)(wide ? L"0xfedcba9876543210" : L"0xffffffff"));
  if (&context == &g_peContext)
    SendMessage(dialog, WM_COMMAND, MAKEWPARAM(controlId, EN_KILLFOCUS), (LPARAM)edit);
  else
    Check(normalizeHeaderValue(dialog, controlId, context), "Valid PE32 input was rejected");
  CheckText(dialog, controlId, wide ? L"FEDCBA9876543210" : L"FFFFFFFF");
  for (const wchar_t* invalid : {L"10000000000000000", L"-1", L"+1", L"12xyz", L"", L"0x"}) {
    SetDlgItemText(dialog, controlId, invalid);
    Check(!normalizeHeaderValue(dialog, controlId, context), "Invalid hex input was accepted");
    CheckText(dialog, controlId, original);
  }
  if (!wide) {
    SetDlgItemText(dialog, controlId, L"100000000");
    Check(!normalizeHeaderValue(dialog, controlId, context), "A 32-bit input overflowed");
    CheckText(dialog, controlId, original);
  }
  SetDlgItemText(dialog, controlId, L"  0x123  ");
  Check(normalizeHeaderValue(dialog, controlId, context), "Whitespace/prefix parsing failed");
  CheckText(dialog, controlId, wide ? L"0000000000000123" : L"00000123");
  SetDlgItemText(dialog, controlId, original);
}

void CheckImportVariants(bool wide) {
  std::vector<BYTE> image(pe32_fixture::kFileSize);
  if (wide) pe64_fixture::Fill(image.data());
  else pe32_fixture::Fill(image.data());
  PEContext context;
  Check(context.Initialize(image.data(), image.size()), "Import context setup failed");
  Dialog imports(IDD_DIALOG_IMPORT_DIRECTORY, ImportDirectoryProc);
  HWND dlls = GetDlgItem(imports.hwnd, IDC_LIST_IMPORT_DLL);
  HWND thunks = GetDlgItem(imports.hwnd, IDC_LIST_INT_THUNK);
  auto descriptor = (PIMAGE_IMPORT_DESCRIPTOR)(image.data() + 0x400);
  auto render = [&]() {
    ListView_DeleteAllItems(dlls);
    readImportDirectory(imports.hwnd, dlls, context);
    ListView_SetItemState(dlls, 0, LVIS_SELECTED, LVIS_SELECTED);
    // Rendering must obtain the lookup RVA from the context, not display text.
    ListView_SetItemText(dlls, 0, 1, const_cast<wchar_t*>(L"FFFFFFFF"));
    readIntThunk(imports.hwnd, dlls, thunks, context);
  };
  auto put = [&](DWORD raw, ULONGLONG value) {
    if (wide) std::memcpy(image.data() + raw, &value, sizeof(ULONGLONG));
    else { DWORD narrow = (DWORD)value; std::memcpy(image.data() + raw, &narrow, sizeof(DWORD)); }
  };
  render();
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 0, pe32_fixture::kImportName, 0, wide ? 3 : 1);
  descriptor->OriginalFirstThunk = 0;
  descriptor->TimeDateStamp = 0xFFFFFFFF;
  render();
  Check(ListView_GetItemCount(thunks) == 0, "A bound IAT replaced the missing lookup table");
  descriptor->TimeDateStamp = 0;
  render();
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 0, pe32_fixture::kImportName, 0, wide ? 3 : 1);
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 2, wide ? L"000004A0" : L"00000460", 0, wide ? 3 : 1);
  DWORD iatRaw = wide ? pe64_fixture::kIatRaw : 0x460;
  DWORD stride = wide ? 8 : 4;
  ULONGLONG flag = wide ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32;
  put(iatRaw, flag | 0xFFFF);
  put(iatRaw + stride, 0);
  render();
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 0, L"Ordinal #65535");
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 3, wide ? L"800000000000FFFF" : L"8000FFFF");
  put(iatRaw, flag | 0x10000 | 42);
  render();
  Check(ListView_GetItemCount(thunks) == 0, "Reserved ordinal bits were accepted");
  put(iatRaw, wide ? 0x0000000100001270ULL : 0x76543210);
  render();
  Check(ListView_GetItemCount(thunks) == 0, "A bound address or truncated name RVA was accepted");
  std::memset(image.data() + image.size() - 16, 'Z', 16);
  put(iatRaw, 0x1FF0);
  render();
  Check(ListView_GetItemCount(thunks) == 0, "An unterminated import name was read");
  descriptor->FirstThunk = wide ? 0x1FFC : 0x1FFE;
  render();
  Check(ListView_GetItemCount(thunks) == 0, "A truncated thunk was read");
  descriptor->FirstThunk = 0x2000 - stride;
  put((DWORD)image.size() - stride, flag | 42);
  render();
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 0, L"Ordinal #42");
  descriptor->FirstThunk = 0;
  render();
  Check(ListView_GetItemCount(thunks) == 0, "Missing import tables left stale rows");
}

void CheckHeaderPages(LPVOID buffer) {
  pe64_fixture::Fill((BYTE*)buffer);
  Check(g_peContext.Initialize(buffer, pe32_fixture::kFileSize),
        "PE32+ page context setup failed");
  Dialog optionalHeader(IDD_DIALOG_IMAGE_OPTIONAL_HEADER32, OptionalHeader32Proc);
  Dialog summary(IDD_DIALOG_HEADER_INFO, HeaderInfoProc);
  auto checkPE64 = [&]() {
    CheckText(optionalHeader.hwnd, IDC_GROUP_OPTIONAL_HEADER32, L"Optional Header64 [HEX]");
    CheckText(optionalHeader.hwnd, IDC_EDIT_MAGIC, L"020B");
    CheckText(optionalHeader.hwnd, IDC_EDIT_MAJ_LINK_VER, L"0E");
    CheckText(optionalHeader.hwnd, IDC_EDIT_IMAGE_BASE, L"0000000140000000");
    CheckText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_STACK_REV, L"0000000200000000");
    CheckText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_STACK_COMMIT, L"0000000100001000");
    CheckText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_HEAP_REV, L"0000000300000000");
    CheckText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_HEAP_COMMIT, L"0000000100002000");
    CheckText(optionalHeader.hwnd, IDC_EDIT_ADDR_OF_ENTRY, L"00001800");
    CheckText(optionalHeader.hwnd, IDC_EDIT_BASE_OF_CODE, L"00001000");
    CheckText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_HEADERS, L"00000200");
    CheckText(optionalHeader.hwnd, IDC_EDIT_CHECKSUM, L"ABCD1234");
    CheckText(optionalHeader.hwnd, IDC_EDIT_BASE_OF_DATA, L"N/A");
    CheckText(summary.hwnd, IDC_EDIT_HEADER_IMAGE_BASE, L"0000000140000000");
    CheckText(summary.hwnd, IDC_EDIT_HEADER_MAGIC, L"020B");
    CheckText(summary.hwnd, IDC_EDIT_HEADER_ADDR_OF_ENTRY, L"00001800");
    CheckText(summary.hwnd, IDC_EDIT_HEADER_SIZE_OF_IMAGE, L"00002000");
    CheckText(summary.hwnd, IDC_EDIT_HEADER_SIZE_OF_HEADERS, L"00000200");
    CheckText(summary.hwnd, IDC_EDIT_HEADER_SUB_STM, L"0002");
    CheckText(summary.hwnd, IDC_EDIT_HEADER_CHECKSUM, L"ABCD1234");
    CheckText(summary.hwnd, IDC_EDIT_HEADER_SIZE_OF_OPT_HEADER, L"00F0");
    CheckText(summary.hwnd, IDC_EDIT_HEADER_NUM_OF_RVA_SIZES, L"00000010");
    CheckText(summary.hwnd, IDC_EDIT_HEADER_BASE_OF_DATA, L"N/A");
    Check(!IsWindowEnabled(GetDlgItem(optionalHeader.hwnd, IDC_EDIT_BASE_OF_DATA)) &&
          !IsWindowEnabled(GetDlgItem(summary.hwnd, IDC_EDIT_HEADER_BASE_OF_DATA)),
          "PE32+ BaseOfData controls must be disabled");
  };
  checkPE64();
  wchar_t title[MAX_PATH] = {};
  GetWindowText(summary.hwnd, title, MAX_PATH);
  Check(std::wcscmp(title, L"[Header Info - PE32+]") == 0, "Summary title is incorrect");
  for (int id : {IDC_EDIT_IMAGE_BASE, IDC_EDIT_SIZE_OF_STACK_REV, IDC_EDIT_SIZE_OF_STACK_COMMIT,
                 IDC_EDIT_SIZE_OF_HEAP_REV, IDC_EDIT_SIZE_OF_HEAP_COMMIT}) {
    CheckWideControl(optionalHeader.hwnd, id);
    CheckValueEditor(optionalHeader.hwnd, id, g_peContext);
  }
  CheckWideControl(summary.hwnd, IDC_EDIT_HEADER_IMAGE_BASE);
  CheckValueEditor(summary.hwnd, IDC_EDIT_HEADER_IMAGE_BASE, g_peContext);
  Check(g_peContext.optionalHeader64->ImageBase == pe64_fixture::kImageBase,
        "Editing preview fields modified the file context");
  SendMessage(optionalHeader.hwnd, WM_COMMAND, IDC_BUTTON_OPTIONAL_HEADER32_IMAGE_ADD, 0);
  CheckText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_IMAGE, L"00003000");
  SendMessage(optionalHeader.hwnd, WM_COMMAND, IDC_BUTTON_OPTIONAL_HEADER32_HEADER_ADD, 0);
  CheckText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_HEADERS, L"00000400");
  SendMessage(summary.hwnd, WM_COMMAND, IDC_BUTTON_HEADER_IMAGE_ADD, 0);
  CheckText(summary.hwnd, IDC_EDIT_HEADER_SIZE_OF_IMAGE, L"00003000");
  SetDlgItemText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_IMAGE, L"FFFFFFFF");
  SendMessage(optionalHeader.hwnd, WM_COMMAND, IDC_BUTTON_OPTIONAL_HEADER32_IMAGE_ADD, 0);
  CheckText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_IMAGE, L"FFFFF000");
  SetDlgItemText(optionalHeader.hwnd, IDC_EDIT_SECTION_ALIGNMENT, L"0");
  SendMessage(optionalHeader.hwnd, WM_COMMAND, IDC_BUTTON_OPTIONAL_HEADER32_SIZE_OF_IMAGE, 0);
  CheckText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_IMAGE, L"00002000");
  readOptionalHeader(optionalHeader.hwnd, g_peContext);
  readHeaderInfo(summary.hwnd, g_peContext);
  checkPE64();

  writeToText(optionalHeader.hwnd, IDC_EDIT_IMAGE_BASE, ~0ULL);
  CheckText(optionalHeader.hwnd, IDC_EDIT_IMAGE_BASE, L"FFFFFFFFFFFFFFFF");
  writeToText(optionalHeader.hwnd, IDC_EDIT_IMAGE_BASE, (ULONGLONG)1);
  CheckText(optionalHeader.hwnd, IDC_EDIT_IMAGE_BASE, L"0000000000000001");
  writeToText(optionalHeader.hwnd, IDC_EDIT_IMAGE_BASE, (DWORD)MAXDWORD);
  CheckText(optionalHeader.hwnd, IDC_EDIT_IMAGE_BASE, L"FFFFFFFF");

  // Reuse both dialogs with a PE32 context while the active file remains PE32+.
  std::vector<BYTE> image(pe32_fixture::kFileSize);
  pe32_fixture::Fill(image.data());
  auto& header = ((PIMAGE_NT_HEADERS32)(image.data() + 0x80))->OptionalHeader;
  header.BaseOfData = 0x1900;
  header.SizeOfStackReserve = MAXDWORD;
  header.SizeOfStackCommit = 0x1000;
  header.SizeOfHeapReserve = 0x80000000;
  header.SizeOfHeapCommit = 0x2000;
  PEContext pe32;
  Check(pe32.Initialize(image.data(), image.size()), "PE32 header page context setup failed");
  readOptionalHeader(optionalHeader.hwnd, pe32);
  readHeaderInfo(summary.hwnd, pe32);
  CheckText(optionalHeader.hwnd, IDC_GROUP_OPTIONAL_HEADER32, L"Optional Header32 [HEX]");
  CheckText(optionalHeader.hwnd, IDC_EDIT_MAGIC, L"010B");
  CheckText(optionalHeader.hwnd, IDC_EDIT_IMAGE_BASE, L"00400000");
  CheckText(optionalHeader.hwnd, IDC_EDIT_BASE_OF_DATA, L"00001900");
  CheckText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_STACK_REV, L"FFFFFFFF");
  CheckText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_STACK_COMMIT, L"00001000");
  CheckText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_HEAP_REV, L"80000000");
  CheckText(optionalHeader.hwnd, IDC_EDIT_SIZE_OF_HEAP_COMMIT, L"00002000");
  CheckText(summary.hwnd, IDC_EDIT_HEADER_IMAGE_BASE, L"00400000");
  CheckText(summary.hwnd, IDC_EDIT_HEADER_BASE_OF_DATA, L"00001900");
  CheckText(summary.hwnd, IDC_EDIT_HEADER_SIZE_OF_OPT_HEADER, L"00E0");
  Check(IsWindowEnabled(GetDlgItem(optionalHeader.hwnd, IDC_EDIT_BASE_OF_DATA)) &&
        IsWindowEnabled(GetDlgItem(summary.hwnd, IDC_EDIT_HEADER_BASE_OF_DATA)),
        "PE32 BaseOfData controls must be re-enabled");
  CheckValueEditor(optionalHeader.hwnd, IDC_EDIT_IMAGE_BASE, pe32);
  CheckValueEditor(summary.hwnd, IDC_EDIT_HEADER_IMAGE_BASE, pe32);

  readOptionalHeader(optionalHeader.hwnd, g_peContext);
  readHeaderInfo(summary.hwnd, g_peContext);
  checkPE64();
}
}  // namespace

int wmain(int argc, wchar_t** argv) {
  std::setvbuf(stdout, NULL, _IONBF, 0);
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
  LPVOID allocatedBuffer = NULL;
  try {
    Check(argc >= 2, "Usage: portability-smoke.exe <generated-fixture-path> [real PE files...]");
    INITCOMMONCONTROLSEX controls = {sizeof(controls), ICC_WIN95_CLASSES};
    Check(InitCommonControlsEx(&controls) != FALSE, "Common control initialization failed");
    std::vector<BYTE> fixture(pe32_fixture::kFileSize);
    pe32_fixture::Fill(fixture.data());
    FILE* file = NULL;
    Check(_wfopen_s(&file, argv[1], L"wb") == 0, "Cannot create fixture file");
    SIZE_T written = std::fwrite(fixture.data(), 1, fixture.size(), file);
    int closeResult = std::fclose(file);
    Check(written == fixture.size() && closeResult == 0, "Cannot write fixture file");
    Check(g_peContext.LoadFile(argv[1]), "Production file reader failed");
    Check(g_peContext.fileSize == fixture.size() &&
          std::memcmp(g_peContext.fileBuffer, fixture.data(), fixture.size()) == 0,
          "File bytes or length differ");
    CheckPEContext(argv[1]);
    CheckDirectoryDetails();
    g_peContext.Reset();

#ifdef _WIN64
    allocatedBuffer = VirtualAlloc((LPVOID)0x0000000200000000ULL, fixture.size(),
                                    MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    Check(allocatedBuffer != NULL, "Cannot allocate the required buffer above 4 GB");
    Check((DWORD_PTR)allocatedBuffer > 0xFFFFFFFFULL, "Buffer must be above 4 GB");
#else
    allocatedBuffer = VirtualAlloc(NULL, fixture.size(), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    Check(allocatedBuffer != NULL, "Cannot allocate test buffer");
#endif
    std::memcpy(allocatedBuffer, fixture.data(), fixture.size());
    Check(g_peContext.Initialize(allocatedBuffer, fixture.size()), "Borrowed buffer initialization failed");
    Check(g_peContext.dosHeader->e_magic == IMAGE_DOS_SIGNATURE, "DOS header pointer mismatch");
    Check(g_peContext.optionalHeader32->ImageBase == pe32_fixture::kImageBase, "Optional header pointer mismatch");
    SIZE_T offset = 0;
    Check(g_peContext.RvaToFileOffset(0x80, 1, offset) && offset == 0x80,
          "Header RVA conversion mismatch");
    Check(g_peContext.RvaToFileOffset(pe32_fixture::kThunkRva, 4, offset) &&
          offset == pe32_fixture::kThunkRaw, "Section RVA conversion mismatch");
    CheckPages();
    CheckContextDirectoryReads();
    CheckHeaderPages(allocatedBuffer);
    CheckPages();
    CheckImportVariants(false);
    CheckImportVariants(true);
    CheckDirectoryErrorPages(allocatedBuffer);
    for (int i = 2; i < argc; ++i) CheckRealDirectoryFile(argv[i]);
    std::printf("PASS: %s, PE32/PE32+ pages, IAT/INT association, relocation blocks, malformed directories, 64-bit input/layout, name/ordinal thunks, bitmap\n",
                sizeof(void*) == 8 ? "x64 buffer above 4 GB" : "x86");
    g_peContext.Reset();
    VirtualFree(allocatedBuffer, 0, MEM_RELEASE);
    return 0;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "FAIL: %s\n", error.what());
    g_peContext.Reset();
    if (allocatedBuffer) VirtualFree(allocatedBuffer, 0, MEM_RELEASE);
    return 1;
  }
}
