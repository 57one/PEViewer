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
#include "fixtures/resources.h"
#include "fixtures/tls.h"
#include "fixtures/delay-imports.h"
#include "fixtures/exceptions.h"

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

void CheckPEContext(LPCTSTR filePath);
void CheckDirectoryDetails();
void CheckResourceDetails();
void CheckTlsDetails();
void CheckDelayImportDetails();
void CheckExceptionDetails();

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

RECT ControlRect(HWND dialog, HWND control) {
  RECT rect = {};
  GetWindowRect(control, &rect);
  MapWindowPoints(NULL, dialog, (POINT*)&rect, 2);
  return rect;
}

void DispatchDirectoryUpdates(HWND dialog) {
  MSG message = {};
  while (PeekMessage(&message, dialog, WM_APP, 0xBFFF, PM_REMOVE)) DispatchMessage(&message);
}

struct ListUpdates {
  HWND list;
  int clears = 0;
  bool disabled = false;
  bool unbatched = false;
  static LRESULT CALLBACK Observe(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                  UINT_PTR, DWORD_PTR reference) {
    auto updates = (ListUpdates*)reference;
    if (message == WM_SETREDRAW) updates->disabled = !wParam;
    if (message == LVM_DELETEALLITEMS || message == LVM_INSERTITEMW) {
      if (!updates->disabled) updates->unbatched = true;
      if (message == LVM_DELETEALLITEMS) ++updates->clears;
    }
    return DefSubclassProc(window, message, wParam, lParam);
  }
  explicit ListUpdates(HWND control) : list(control) {
    Check(SetWindowSubclass(list, Observe, 2, (DWORD_PTR)this) != FALSE,
          "Cannot observe directory list updates");
  }
  ~ListUpdates() { RemoveWindowSubclass(list, Observe, 2); }
};

void CheckDirectoryWindowBehavior(void* buffer, bool wide) {
  delay_fixture::Fill((BYTE*)buffer, wide);
  Check(g_peContext.Initialize(buffer, pe32_fixture::kFileSize), "Resize context rejected");
  const struct {
    int resource;
    DLGPROC callback;
    int first, second, close;
  } pages[] = {
    {IDD_DIALOG_DIRECTORY, DirectoryProc, 0, 0, IDC_BUTTON_DIRECTORY_CANCEL},
    {IDD_DIALOG_IMPORT_DIRECTORY, ImportDirectoryProc, IDC_LIST_IMPORT_DLL, IDC_LIST_INT_THUNK,
      IDC_BUTTON_IMPORT_DIRECTORY_CLOSE},
    {IDD_DIALOG_EXPORT_DIRECTORY, ExportDirectoryProc, IDC_LIST_EXPORT_FUNC, 0, 0},
    {IDD_DIALOG_BOUND_IMPORT_DIRECTORY, BoundImportDirectoryProc, IDC_LIST_BOUND_IMPORT_DIRECTORY,
      0, IDC_BUTTON_BOUND_IMPORT_DIRECTORY_CLOSE},
    {IDD_DIALOG_DEBUG_DIRECTORY, DebugDirectoryProc, IDC_LIST_DEBUG_DIRECTORY, IDC_LIST_DEBUG_DIRECTORY_INFO,
      IDC_BUTTON_DEBUG_DIRECTORY_CLOSE},
    {IDD_DIALOG_IAT, IatDirectoryProc, IDC_LIST_IAT, 0, IDCANCEL},
    {IDD_DIALOG_RELOCATIONS, RelocationDirectoryProc, IDC_LIST_RELOC_BLOCKS, IDC_LIST_RELOC_ENTRIES, IDCANCEL},
    {IDD_DIALOG_RESOURCES, ResourceDirectoryProc, IDC_TREE_RESOURCES, IDC_LIST_RESOURCE_DETAILS, IDCANCEL},
    {IDD_DIALOG_TLS, TlsDirectoryProc, IDC_LIST_TLS_FIELDS, IDC_LIST_TLS_CALLBACKS, IDCANCEL},
    {IDD_DIALOG_DELAY_IMPORT, DelayImportDirectoryProc, IDC_LIST_DELAY_MODULES, IDC_LIST_DELAY_FUNCTIONS, IDCANCEL},
    {IDD_DIALOG_EXCEPTIONS, ExceptionDirectoryProc, IDC_LIST_EXCEPTION_FUNCTIONS, 0, IDCANCEL}
  };
  for (const auto& page : pages) {
    Dialog dialog(page.resource, page.callback);
    LONG_PTR style = GetWindowLongPtr(dialog.hwnd, GWL_STYLE);
    Check((style & WS_THICKFRAME) && (style & WS_MAXIMIZEBOX), "Directory window cannot resize/maximize");
    RECT window = {}, client = {};
    GetWindowRect(dialog.hwnd, &window);
    GetClientRect(dialog.hwnd, &client);
    // Paint into a memory DC: check the actual parent/static brush contract,
    // including the empty areas inside sibling group boxes.
    HDC paint = CreateCompatibleDC(NULL);
    BITMAPINFO bitmapInfo = {};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = bitmapInfo.bmiHeader.biHeight = 8;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    void* pixels = NULL;
    HBITMAP bitmap = CreateDIBSection(paint, &bitmapInfo, DIB_RGB_COLORS, &pixels, NULL, 0);
    Check(paint && bitmap, "Cannot create directory background test surface");
    HGDIOBJ previousBitmap = SelectObject(paint, bitmap);
    SetPixel(paint, 1, 1, RGB(255, 0, 255));
    SendMessage(dialog.hwnd, WM_ERASEBKGND, (WPARAM)paint, 0);
    Check(GetPixel(paint, 1, 1) == GetSysColor(COLOR_BTNFACE), "Directory background is not erased consistently");
    HWND status = GetDlgItem(dialog.hwnd, IDC_STATIC_DIRECTORY_STATUS);
    if (status) {
      HBRUSH brush = (HBRUSH)SendMessage(dialog.hwnd, WM_CTLCOLORSTATIC, (WPARAM)paint, (LPARAM)status);
      Check(brush == GetSysColorBrush(COLOR_BTNFACE) && GetBkMode(paint) == TRANSPARENT &&
            GetBkColor(paint) == GetSysColor(COLOR_BTNFACE), "Static text has a different background");
    }
    SelectObject(paint, previousBitmap);
    DeleteObject(bitmap);
    DeleteDC(paint);
    for (HWND child = GetWindow(dialog.hwnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
      wchar_t className[32] = {};
      GetClassName(child, className, _countof(className));
      if (!lstrcmpi(className, L"Button") &&
          (GetWindowLongPtr(child, GWL_STYLE) & BS_TYPEMASK) == BS_GROUPBOX) {
        Check(!(GetWindowLongPtr(dialog.hwnd, GWL_STYLE) & WS_CLIPCHILDREN),
              "Parent background incorrectly clips out a transparent group box");
        Check((GetWindowLongPtr(child, GWL_STYLE) & WS_CLIPSIBLINGS) != 0,
              "Group box can paint over sibling tables");
      }
    }
    MINMAXINFO limits = {};
    SendMessage(dialog.hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&limits);
    Check(limits.ptMinTrackSize.x == window.right - window.left &&
          limits.ptMinTrackSize.y == window.bottom - window.top, "Directory minimum size is missing");
    HWND first = GetDlgItem(dialog.hwnd, page.first);
    HWND second = GetDlgItem(dialog.hwnd, page.second);
    HWND close = GetDlgItem(dialog.hwnd, page.close);
    RECT originalFirst = first ? ControlRect(dialog.hwnd, first) : RECT{};
    RECT originalSecond = second ? ControlRect(dialog.hwnd, second) : RECT{};
    RECT originalClose = close ? ControlRect(dialog.hwnd, close) : RECT{};
    for (int cycle = 0; cycle < 3; ++cycle) {
      SetWindowPos(dialog.hwnd, NULL, 0, 0, window.right - window.left + 320,
          window.bottom - window.top + 180, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
      RECT grown = {};
      GetClientRect(dialog.hwnd, &grown);
      Check(grown.right == client.right + 320 && grown.bottom == client.bottom + 180,
            "Directory client did not grow with the window");
      if (first) {
        RECT rect = ControlRect(dialog.hwnd, first);
        Check(rect.right - rect.left > originalFirst.right - originalFirst.left &&
              rect.bottom - rect.top > originalFirst.bottom - originalFirst.top,
              "Directory table did not grow with the window");
      }
      if (second) {
        RECT rect = ControlRect(dialog.hwnd, second);
        Check(rect.right - rect.left > originalSecond.right - originalSecond.left &&
              rect.bottom - rect.top > originalSecond.bottom - originalSecond.top,
              "Directory detail table did not grow");
      }
      if (close) {
        RECT rect = ControlRect(dialog.hwnd, close);
        Check(grown.right - rect.right == client.right - originalClose.right &&
              grown.bottom - rect.bottom == client.bottom - originalClose.bottom &&
              rect.right - rect.left == originalClose.right - originalClose.left,
              "Directory Close button lost its size or bottom/right margins");
      }
      for (HWND child = GetWindow(dialog.hwnd, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
        RECT rect = ControlRect(dialog.hwnd, child);
        Check(rect.left >= 0 && rect.top >= 0 && rect.right <= grown.right && rect.bottom <= grown.bottom,
              "Directory control moved outside the client area");
      }
      SetWindowPos(dialog.hwnd, NULL, 0, 0, window.right - window.left, window.bottom - window.top,
                   SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
      if (first) {
        RECT restored = ControlRect(dialog.hwnd, first);
        Check(EqualRect(&restored, &originalFirst) != FALSE, "Repeated resizing accumulated layout drift");
      }
    }
    Check(!IsWindowVisible(dialog.hwnd), "Directory updates exposed a hidden dialog");
  }

  {
    Dialog dialog(IDD_DIALOG_DELAY_IMPORT, DelayImportDirectoryProc);
    HWND modules = GetDlgItem(dialog.hwnd, IDC_LIST_DELAY_MODULES);
    HWND functions = GetDlgItem(dialog.hwnd, IDC_LIST_DELAY_FUNCTIONS);
    ListUpdates updates(functions);
    int widened = ListView_GetColumnWidth(functions, 1) + 1200;
    ListView_SetColumnWidth(functions, 1, widened);
    ListView_Scroll(functions, 180, 0);
    int horizontal = GetScrollPos(functions, SB_HORZ);
    Check(horizontal > 0, "Horizontal scroll test did not reach offscreen columns");
    ListView_SetItemState(modules, 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    DispatchDirectoryUpdates(dialog.hwnd);
    Check(updates.clears == 1 && !updates.unbatched && !updates.disabled,
          "Delay selection redraws an empty/intermediate list or rebuilds twice");
    Check(ListView_GetItemCount(functions) == 1, "Delay selection did not refresh its functions");
    Check(ListView_GetColumnWidth(functions, 1) == widened && GetScrollPos(functions, SB_HORZ) == horizontal,
          "Delay selection reset column widths or horizontal scroll");
    ListView_SetItemState(modules, 1, 0, LVIS_FOCUSED);
    DispatchDirectoryUpdates(dialog.hwnd);
    Check(updates.clears == 1, "A focus-only notification rebuilt delay functions");
    ListView_SetItemState(modules, -1, 0, LVIS_SELECTED);
    DispatchDirectoryUpdates(dialog.hwnd);
    Check(ListView_GetItemCount(functions) == 0 && updates.clears == 2,
          "Clearing selection left stale delay functions");
  }
  {
    Dialog dialog(IDD_DIALOG_IMPORT_DIRECTORY, ImportDirectoryProc);
    HWND modules = GetDlgItem(dialog.hwnd, IDC_LIST_IMPORT_DLL);
    HWND functions = GetDlgItem(dialog.hwnd, IDC_LIST_INT_THUNK);
    ListUpdates updates(functions);
    ListView_SetItemState(modules, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    Check(updates.clears == 1 && !updates.unbatched && ListView_GetItemCount(functions) > 0,
          "Import keyboard-style selection did not refresh in one batch");
    ListView_SetItemState(modules, 0, 0, LVIS_FOCUSED);
    Check(updates.clears == 1, "Import focus changes rebuild the function list");
  }
  {
    Dialog dialog(IDD_DIALOG_RELOCATIONS, RelocationDirectoryProc);
    HWND blocks = GetDlgItem(dialog.hwnd, IDC_LIST_RELOC_BLOCKS);
    ListUpdates updates(GetDlgItem(dialog.hwnd, IDC_LIST_RELOC_ENTRIES));
    ListView_SetItemState(blocks, 0, 0, LVIS_FOCUSED);
    Check(updates.clears == 0, "Relocation focus changes rebuild the entries");
    ListView_SetItemState(blocks, 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    DispatchDirectoryUpdates(dialog.hwnd);
    Check(updates.clears == 1 && !updates.unbatched, "Relocation switch is not a single batch");
  }
  resource_fixture::Fill((BYTE*)buffer, wide);
  Check(g_peContext.Initialize(buffer, pe32_fixture::kFileSize), "Resource update context rejected");
  {
    Dialog dialog(IDD_DIALOG_RESOURCES, ResourceDirectoryProc);
    HWND tree = GetDlgItem(dialog.hwnd, IDC_TREE_RESOURCES);
    ListUpdates updates(GetDlgItem(dialog.hwnd, IDC_LIST_RESOURCE_DETAILS));
    TreeView_SelectItem(tree, TreeView_GetChild(tree, TreeView_GetRoot(tree)));
    Check(updates.clears == 1 && !updates.unbatched && !updates.disabled,
          "Resource selection does not batch its detail updates");
  }
}

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
  Dialog resources(IDD_DIALOG_RESOURCES, ResourceDirectoryProc);
  Check(TreeView_GetCount(GetDlgItem(resources.hwnd, IDC_TREE_RESOURCES)) == 0,
        "Missing resource directory rendered a tree");

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
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 4, L"Export");
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 5, L"-");
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

void CheckRealDirectoryFile(const wchar_t* path, const wchar_t* resourceReferencePath) {
  PEContext context;
  Check(context.LoadFile(path), "Cannot load real PE file");
  IatDetails iat = ReadIatDetails(context);
  RelocationDetails relocs = ReadRelocationDetails(context);
  const auto imports = ReadImportDetails(context);
  const auto exports = ReadExportDetails(context);
  const auto bound = ReadBoundImportDetails(context);
  const auto debug = ReadDebugDetails(context);
  const auto tls = ReadTlsDetails(context);
  const auto delay = ReadDelayImportDetails(context);
  const auto exceptions = ReadExceptionDetails(context);
  if (exceptions.present && exceptions.supported) {
    if (!exceptions.message.empty()) std::fwprintf(stderr, L"Exceptions: %ls\n", exceptions.message.c_str());
    Check(exceptions.complete && exceptions.message.empty(), "Real AMD64 exception table rejected");
    for (const auto& entry : exceptions.entries)
      Check(entry.rangeValid && entry.hasCodeFileOffset && entry.hasUnwindFileOffset && entry.message.empty(),
            "Real AMD64 exception record range/reference invalid");
  }
  std::wprintf(L"      Exception Machine=%04X records=%zu size=%08lX\n",
      exceptions.machine, exceptions.entries.size(), exceptions.size);
  if (!exceptions.entries.empty()) {
    const auto& first = exceptions.entries.front();
    const auto& last = exceptions.entries.back();
    std::wprintf(L"      Exception first=%08lX %08lX %08lX last=%08lX %08lX %08lX\n",
        first.beginRva, first.endRva, first.unwindRva, last.beginRva, last.endRva, last.unwindRva);
  }
  if (delay.present) {
    if (!delay.message.empty()) std::fwprintf(stderr, L"Delay imports: %ls\n", delay.message.c_str());
    Check(delay.terminated && delay.message.empty(), "Real-file delay descriptor parsing failed");
    SIZE_T functions = 0;
    for (const auto& module : delay.modules) {
      const auto thunks = ReadDelayImportThunks(context, module);
      if (!thunks.message.empty())
        std::fwprintf(stderr, L"Delay functions: %ls\n", thunks.message.c_str());
      Check(module.message.empty() && thunks.terminated && thunks.message.empty(),
            "Real-file delay thunk parsing failed");
      functions += thunks.entries.size();
      std::printf("      Delay DLL=%s functions=%zu\n", module.name.c_str(), thunks.entries.size());
    }
    std::printf("      Delay imports: DLLs=%zu functions=%zu\n", delay.modules.size(), functions);
  }
  if (tls.present) {
    if (!tls.message.empty()) std::fwprintf(stderr, L"TLS: %ls\n", tls.message.c_str());
    Check(tls.hasHeader && tls.terminated && tls.rawDataValid && tls.message.empty(),
          "Real-file TLS directory parsing failed");
    for (const auto& callback : tls.callbacks)
      Check(callback.target.hasFileOffset && callback.target.message.empty(),
            "Real-file TLS callback VA conversion failed");
  }
  const auto resources = ReadResourceDetails(context);
  if (resources.present) {
    if (!resources.message.empty()) std::fwprintf(stderr, L"Resources: %ls\n", resources.message.c_str());
    Check(resources.hasRoot && resources.message.empty(), "Real-file resource parsing failed");
    // Use a private copy so system MUI satellites and already-loaded modules cannot replace disk resources.
    FILE* reference = NULL;
    Check(_wfopen_s(&reference, resourceReferencePath, L"wb") == 0, "Cannot create resource reference copy");
    SIZE_T written = std::fwrite(context.fileBuffer, 1, context.fileSize, reference);
    int closed = std::fclose(reference);
    Check(written == context.fileSize && closed == 0, "Cannot write resource reference copy");
    HMODULE module = LoadLibraryEx(resourceReferencePath, NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
    Check(module != NULL, "Cannot map real-file resources for API comparison");
    try {
      for (const auto& leaf : resources.nodes) {
        if (!leaf.hasData || leaf.depth != 2 || leaf.named || leaf.id > MAXWORD) continue;
        const auto& name = resources.nodes[leaf.parent];
        const auto& type = resources.nodes[name.parent];
        if ((!name.named && name.id > MAXWORD) || (!type.named && type.id > MAXWORD)) continue;
        LPCWSTR typeName = type.named ? type.name.c_str() : MAKEINTRESOURCE(type.id);
        LPCWSTR resourceName = name.named ? name.name.c_str() : MAKEINTRESOURCE(name.id);
        HRSRC resource = FindResourceEx(module, typeName, resourceName, (WORD)leaf.id);
        if (!resource || SizeofResource(module, resource) != leaf.data.Size)
          std::fwprintf(stderr, L"Resource API mismatch: %ls type=%ls name=%ls language=%04X disk=%lu API=%lu\n",
              path, type.named ? type.name.c_str() : std::to_wstring(type.id).c_str(),
              name.named ? name.name.c_str() : std::to_wstring(name.id).c_str(), leaf.id,
              leaf.data.Size, resource ? SizeofResource(module, resource) : 0);
        Check(resource && SizeofResource(module, resource) == leaf.data.Size,
              "Resource size differs from Windows resource APIs");
        if (leaf.data.Size) {
          const void* bytes = LockResource(LoadResource(module, resource));
          Check(bytes && std::memcmp(bytes, context.ReadRva(leaf.data.OffsetToData, leaf.data.Size),
                                    leaf.data.Size) == 0, "Resource payload differs from Windows resource APIs");
        }
      }
    } catch (...) { FreeLibrary(module); throw; }
    FreeLibrary(module);
  }
  SIZE_T importedFunctions = 0;
  if (imports.present) {
    Check(imports.message.empty() && imports.terminated, "Real-file import descriptors failed");
    for (const auto& module : imports.modules) {
      const auto thunks = ReadImportThunks(context, module.descriptor);
      Check(thunks.message.empty() && thunks.terminated, "Real-file import function decoding failed");
      importedFunctions += thunks.entries.size();
    }
  }
  if (exports.present) {
    if (!exports.message.empty()) std::fwprintf(stderr, L"Exports: %ls\n", exports.message.c_str());
    Check(exports.hasHeader && exports.message.empty(), "Real-file export parsing failed");
  }
  SIZE_T forwarders = 0;
  for (const auto& entry : exports.entries) {
    if (entry.isForwarder) {
      Check(!entry.forwarder.empty() && entry.message.empty() && entry.hasFileOffset,
            "Real-file forwarder target was not decoded");
      ++forwarders;
    } else Check(entry.forwarder.empty(), "Ordinary export acquired a forwarder target");
  }
  if (bound.present) Check(bound.terminated && bound.message.empty(), "Real-file bound import parsing failed");
  if (debug.present) Check(debug.message.empty(), "Real-file debug directory parsing failed");
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
    if (slot.symbol.kind == ImportKind::Name || slot.symbol.kind == ImportKind::Ordinal) ++functions;
    else ++unresolved;
  }
  std::wprintf(L"PASS: %ls (%ls), IAT slots=%zu (functions=%zu, unresolved=%zu), relocation blocks=%zu, entries=%zu\n", path,
               context.format == PEFormat::PE32Plus ? L"PE32+" : L"PE32",
               iat.entries.size(), functions, unresolved, relocs.blocks.size(), entries);
  std::wprintf(L"      import functions=%zu, exports=%zu (forwarders=%zu), bound imports=%zu, debug records=%zu\n",
               importedFunctions, exports.entries.size(), forwarders, bound.entries.size(), debug.entries.size());
  std::wprintf(L"      resource entries=%zu, resource nodes=%zu\n", resources.resourceCount, resources.nodes.size());
  std::wprintf(L"      TLS present=%u, callbacks=%zu\n", tls.present ? 1 : 0, tls.callbacks.size());
  if (tls.hasHeader) {
    std::wprintf(L"      TLS start=%016I64X end=%016I64X index=%016I64X array=%016I64X zero=%08lX characteristics=%08lX\n",
        tls.rawDataStart.va, tls.rawDataEnd.va, tls.index.va, tls.callbackArray.va,
        tls.sizeOfZeroFill, tls.characteristics);
    for (const auto& callback : tls.callbacks)
      std::wprintf(L"      TLS callback=%016I64X RVA=%08lX file=%zX\n", callback.target.va,
                    callback.target.rva, callback.target.fileOffset);
  }
}

void CheckDelayImportPage(void* buffer, bool wide) {
  auto image = (BYTE*)buffer;
  delay_fixture::Fill(image, wide);
  Check(g_peContext.Initialize(buffer, pe32_fixture::kFileSize), "Delay page context rejected");
  std::vector<BYTE> otherImage(pe32_fixture::kFileSize);
  PEContext other;
  Dialog directories(IDD_DIALOG_DIRECTORY, DirectoryProc);
  Check(IsWindowEnabled(GetDlgItem(directories.hwnd, IDC_BUTTON_DIRECTORY_DELAY_IMPORT)),
        "Present delay directory button disabled");
  Dialog dialog(IDD_DIALOG_DELAY_IMPORT, DelayImportDirectoryProc);
  HWND modules = GetDlgItem(dialog.hwnd, IDC_LIST_DELAY_MODULES);
  HWND functions = GetDlgItem(dialog.hwnd, IDC_LIST_DELAY_FUNCTIONS);
  wchar_t title[80] = {};
  GetWindowText(dialog.hwnd, title, _countof(title));
  Check(std::wcscmp(title, wide ? L"Delay Import Directory - PE32+" : L"Delay Import Directory - PE32") == 0,
        "Delay page title has wrong format");
  CheckList(dialog.hwnd, IDC_LIST_DELAY_MODULES, 0, L"delay-test.dll", 0, 2);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_MODULES, 2, L"00000001 (RVA)", 0, 2);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_FUNCTIONS, 1, L"DelayedFunc", 0, 3);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_FUNCTIONS, 2, L"1234", 0, 3);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_FUNCTIONS, 1, L"Ordinal #42", 1, 3);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_FUNCTIONS, 3, wide ? L"00001B08" : L"00001B04", 1, 3);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_FUNCTIONS, 8, wide ? L"0000000140001800" : L"00401800", 0, 3);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_FUNCTIONS, 9, wide ? L"00007FFB12345678" : L"77001000", 0, 3);
  CheckText(dialog.hwnd, IDC_STATIC_DIRECTORY_STATUS, L"2 DLLs. Descriptors complete. OK");
  CheckText(dialog.hwnd, IDC_STATIC_DELAY_FUNCTION_STATUS, L"3 functions. INT complete. OK");
  Check(ListView_GetColumnWidth(functions, 8) >= ListView_GetStringWidth(functions,
          wide ? L"0000000140001800" : L"00401800"), "Delay IAT column clips pointer values");
  ListView_SetItemState(modules, 0, 0, LVIS_SELECTED);
  ListView_SetItemState(modules, 1, LVIS_SELECTED, LVIS_SELECTED);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_FUNCTIONS, 1, L"Ordinal #7", 0, 1);
  ListView_SetItemText(modules, 0, 0, const_cast<wchar_t*>(L"edited display text"));
  ListView_SetItemState(modules, 1, 0, LVIS_SELECTED);
  ListView_SetItemState(modules, 0, LVIS_SELECTED, LVIS_SELECTED);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_FUNCTIONS, 1, L"DelayedFunc", 0, 3);

  delay_fixture::Value(image, delay_fixture::kLookupRva,
      (wide ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32) | 0x10000 | 3, wide);
  showDelayImportDirectory(dialog.hwnd, g_peContext);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_FUNCTIONS, 11, L"Invalid ordinal reserved bits", 0, 3);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_FUNCTIONS, 1, L"SecondDelayFunc", 2, 3);

  // Switch both context and descriptor mode, while the global context remains unchanged.
  delay_fixture::Fill(otherImage.data(), !wide, true);
  Check(other.Initialize(otherImage.data(), otherImage.size()), "Alternate delay context rejected");
  showDelayImportDirectory(dialog.hwnd, other);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_MODULES, 2, L"00000000 (VA)", 0, 2);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_MODULES, 4,
            wide ? L"00401B00 -> 00001B00" : L"10001B00 -> 00001B00", 0, 2);
  CheckList(dialog.hwnd, IDC_LIST_DELAY_FUNCTIONS, 8,
            wide ? L"00401800" : L"0000000010001800", 0, 3);
  auto directory = const_cast<IMAGE_DATA_DIRECTORY*>(other.Directory(IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT));
  directory->Size = 31;
  showDelayImportDirectory(dialog.hwnd, other);
  Check(ListView_GetItemCount(modules) == 0 && ListView_GetItemCount(functions) == 0,
        "Failed delay reload left stale DLLs or functions");
  CheckText(dialog.hwnd, IDC_STATIC_DELAY_FUNCTION_STATUS, L"");
  *directory = {};
  showDelayImportDirectory(dialog.hwnd, other);
  showDirectory(directories.hwnd, other);
  Check(!IsWindowEnabled(GetDlgItem(directories.hwnd, IDC_BUTTON_DIRECTORY_DELAY_IMPORT)),
        "Missing delay directory button enabled");
}

void CheckExceptionPage(void* buffer) {
  auto image = (BYTE*)buffer;
  exception_fixture::Fill(image);
  Check(g_peContext.Initialize(buffer, pe32_fixture::kFileSize), "Exception page fixture rejected");
  Dialog directories(IDD_DIALOG_DIRECTORY, DirectoryProc);
  Check(IsWindowEnabled(GetDlgItem(directories.hwnd, IDC_BUTTON_DIRECTORY_EXCEPTION)),
        "Present exception directory button disabled");
  Dialog dialog(IDD_DIALOG_EXCEPTIONS, ExceptionDirectoryProc);
  HWND list = GetDlgItem(dialog.hwnd, IDC_LIST_EXCEPTION_FUNCTIONS);
  wchar_t title[100] = {};
  GetWindowText(dialog.hwnd, title, _countof(title));
  Check(!std::wcscmp(title, L"Exception Directory - AMD64 (PE32+)"), "Exception title lacks architecture");
  CheckList(dialog.hwnd, IDC_LIST_EXCEPTION_FUNCTIONS, 1, L"00001900", 0, 3);
  CheckList(dialog.hwnd, IDC_LIST_EXCEPTION_FUNCTIONS, 2, L"00000B00", 0, 3);
  CheckList(dialog.hwnd, IDC_LIST_EXCEPTION_FUNCTIONS, 3, L"00001810", 1, 3);
  CheckList(dialog.hwnd, IDC_LIST_EXCEPTION_FUNCTIONS, 4, L"00001830", 1, 3);
  CheckList(dialog.hwnd, IDC_LIST_EXCEPTION_FUNCTIONS, 5, L"00000020", 1, 3);
  CheckList(dialog.hwnd, IDC_LIST_EXCEPTION_FUNCTIONS, 6, L"00000A10", 1, 3);
  CheckList(dialog.hwnd, IDC_LIST_EXCEPTION_FUNCTIONS, 7, L"00001A04", 1, 3);
  CheckList(dialog.hwnd, IDC_LIST_EXCEPTION_FUNCTIONS, 8, L"00000C04", 1, 3);
  CheckList(dialog.hwnd, IDC_LIST_EXCEPTION_FUNCTIONS, 9, L"OK", 1, 3);
  Check(ListView_GetColumnWidth(list, 4) >= ListView_GetStringWidth(list, L"00001830"),
        "Exception RVA text clipped");
  ListView_SetColumnWidth(list, 3, 520);
  ListUpdates updates(list);
  exception_fixture::Record(image, 0, 0x1800, 0x1800, 0);
  showExceptionDirectory(dialog.hwnd, g_peContext);
  wchar_t text[512] = {};
  ListView_GetItemText(list, 0, 9, text, _countof(text));
  Check(std::wcsstr(text, L"Invalid function range") != NULL, "Invalid exception row has no error text");
  CheckList(dialog.hwnd, IDC_LIST_EXCEPTION_FUNCTIONS, 9, L"OK", 1, 3);
  Check(!updates.unbatched && updates.clears == 1 && ListView_GetColumnWidth(list, 3) >= 520,
        "Exception refresh flickers or shrinks user column width");

  std::vector<BYTE> otherImage(pe32_fixture::kFileSize);
  exception_fixture::Fill(otherImage.data(), false);
  PEContext other;
  Check(other.Initialize(otherImage.data(), otherImage.size()), "PE32 exception page context rejected");
  showExceptionDirectory(dialog.hwnd, other);
  Check(ListView_GetItemCount(list) == 0, "Unsupported Machine retained old AMD64 rows");
  GetDlgItemText(dialog.hwnd, IDC_STATIC_DIRECTORY_STATUS, text, _countof(text));
  Check(std::wcsstr(text, L"Unsupported") != NULL, "Unsupported architecture has no visible status");
  exception_fixture::Fill(otherImage.data());
  Check(other.Initialize(otherImage.data(), otherImage.size()), "Alternate exception context rejected");
  showExceptionDirectory(dialog.hwnd, other);
  CheckList(dialog.hwnd, IDC_LIST_EXCEPTION_FUNCTIONS, 9, L"OK", 0, 3);
  auto directory = const_cast<IMAGE_DATA_DIRECTORY*>(other.Directory(IMAGE_DIRECTORY_ENTRY_EXCEPTION));
  directory->Size = 1;
  showExceptionDirectory(dialog.hwnd, other);
  Check(ListView_GetItemCount(list) == 0, "Truncated table retained old exception rows");
  GetDlgItemText(dialog.hwnd, IDC_STATIC_DIRECTORY_STATUS, text, _countof(text));
  Check(std::wcsstr(text, L"Truncated") != NULL, "Partial exception record has no visible status");
  *directory = {};
  showExceptionDirectory(dialog.hwnd, other);
  showDirectory(directories.hwnd, other);
  Check(!IsWindowEnabled(GetDlgItem(directories.hwnd, IDC_BUTTON_DIRECTORY_EXCEPTION)),
        "Missing exception directory button remains enabled");
}

void CheckTlsPage(void* buffer, bool wide) {
  auto data = (BYTE*)buffer;
  tls_fixture::Fill(data, wide);
  Check(g_peContext.Initialize(buffer, pe32_fixture::kFileSize), "TLS page context rejected");
  Dialog directories(IDD_DIALOG_DIRECTORY, DirectoryProc);
  Check(IsWindowEnabled(GetDlgItem(directories.hwnd, IDC_BUTTON_DIRECTORY_TLS)) != FALSE,
        "Present TLS directory button is disabled");
  Dialog dialog(IDD_DIALOG_TLS, TlsDirectoryProc);
  HWND fields = GetDlgItem(dialog.hwnd, IDC_LIST_TLS_FIELDS);
  HWND callbacks = GetDlgItem(dialog.hwnd, IDC_LIST_TLS_CALLBACKS);
  wchar_t title[80] = {};
  GetWindowText(dialog.hwnd, title, _countof(title));
  Check(std::wcscmp(title, wide ? L"TLS Directory - PE32+" : L"TLS Directory - PE32") == 0,
        "TLS page format title mismatch");
  CheckList(dialog.hwnd, IDC_LIST_TLS_FIELDS, 1, wide ? L"0000000140001A00" : L"00401A00", 0, 8);
  CheckList(dialog.hwnd, IDC_LIST_TLS_FIELDS, 2, L"00001A00", 0, 8);
  CheckList(dialog.hwnd, IDC_LIST_TLS_FIELDS, 3, L"00000C00", 0, 8);
  CheckList(dialog.hwnd, IDC_LIST_TLS_FIELDS, 1, L"00000020", 4, 8);
  CheckList(dialog.hwnd, IDC_LIST_TLS_FIELDS, 1, L"00500000", 5, 8);
  CheckList(dialog.hwnd, IDC_LIST_TLS_CALLBACKS, 3, wide ? L"0000000140001800" : L"00401800", 0, 2);
  CheckList(dialog.hwnd, IDC_LIST_TLS_CALLBACKS, 1, wide ? L"00001988" : L"00001984", 1, 2);
  CheckList(dialog.hwnd, IDC_LIST_TLS_CALLBACKS, 4, L"00001810", 1, 2);
  CheckList(dialog.hwnd, IDC_LIST_TLS_CALLBACKS, 5, L"00000A10", 1, 2);
  CheckList(dialog.hwnd, IDC_LIST_TLS_CALLBACKS, 6, L"OK", 1, 2);
  Check(ListView_GetColumnWidth(callbacks, 3) >= ListView_GetStringWidth(callbacks, wide ? L"0000000140001800" : L"00401800"),
        "TLS callback VA column clips full 64-bit values");
  CheckText(dialog.hwnd, IDC_STATIC_DIRECTORY_STATUS, L"2 callbacks. Callback list complete. OK");

  ULONGLONG base = wide ? pe64_fixture::kImageBase : pe32_fixture::kImageBase;
  tls_fixture::WriteValue(data + tls_fixture::kCallbacksRaw, base - 1, wide);
  showTlsDirectory(dialog.hwnd, g_peContext);
  CheckList(dialog.hwnd, IDC_LIST_TLS_CALLBACKS, 6, L"VA is below ImageBase", 0, 2);
  CheckList(dialog.hwnd, IDC_LIST_TLS_CALLBACKS, 6, L"OK", 1, 2);

  // Reload using another context; the active file must not determine the displayed format.
  std::vector<BYTE> image(pe32_fixture::kFileSize);
  tls_fixture::Fill(image.data(), !wide);
  PEContext other;
  Check(other.Initialize(image.data(), image.size()), "Alternate TLS context rejected");
  showTlsDirectory(dialog.hwnd, other);
  CheckList(dialog.hwnd, IDC_LIST_TLS_CALLBACKS, 3, wide ? L"00401800" : L"0000000140001800", 0, 2);
  auto directory = const_cast<IMAGE_DATA_DIRECTORY*>(other.Directory(IMAGE_DIRECTORY_ENTRY_TLS));
  directory->Size = 1;
  showTlsDirectory(dialog.hwnd, other);
  Check(ListView_GetItemCount(fields) == 0 && ListView_GetItemCount(callbacks) == 0,
        "Truncated TLS header left stale fields or rows");
  CheckText(dialog.hwnd, IDC_STATIC_DIRECTORY_STATUS, L"Truncated TLS directory header.");
  *directory = {};
  showTlsDirectory(dialog.hwnd, other);
  CheckText(dialog.hwnd, IDC_STATIC_DIRECTORY_STATUS, L"No TLS directory.");
  showDirectory(directories.hwnd, other);
  Check(!IsWindowEnabled(GetDlgItem(directories.hwnd, IDC_BUTTON_DIRECTORY_TLS)),
        "Missing TLS directory button remains enabled");
}

void CheckResourcePage(void* buffer, bool wide) {
  auto data = (BYTE*)buffer;
  resource_fixture::Fill(data, wide);
  Check(g_peContext.Initialize(buffer, pe32_fixture::kFileSize), "Resource page context rejected");
  Dialog directories(IDD_DIALOG_DIRECTORY, DirectoryProc);
  Check(IsWindowEnabled(GetDlgItem(directories.hwnd, IDC_BUTTON_DIRECTORY_RESOURCE)) != FALSE,
        "Present resource directory is disabled");
  Dialog dialog(IDD_DIALOG_RESOURCES, ResourceDirectoryProc);
  HWND tree = GetDlgItem(dialog.hwnd, IDC_TREE_RESOURCES);
  HWND list = GetDlgItem(dialog.hwnd, IDC_LIST_RESOURCE_DETAILS);
  auto field = [&](const wchar_t* name, const wchar_t* value) {
    wchar_t actual[512] = {};
    for (int row = 0; row < ListView_GetItemCount(list); ++row) {
      ListView_GetItemText(list, row, 0, actual, _countof(actual));
      if (std::wcscmp(actual, name)) continue;
      ListView_GetItemText(list, row, 1, actual, _countof(actual));
      Check(std::wcscmp(actual, value) == 0, "Resource details field mismatch");
      return;
    }
    throw std::runtime_error("Resource details field missing");
  };
  auto text = [&](HTREEITEM item, const wchar_t* expected) {
    wchar_t actual[512] = {};
    TVITEM query = {};
    query.mask = TVIF_TEXT;
    query.hItem = item;
    query.pszText = actual;
    query.cchTextMax = _countof(actual);
    Check(item && TreeView_GetItem(tree, &query), "Resource tree item missing");
    Check(std::wcscmp(actual, expected) == 0, "Resource tree label mismatch");
  };
  wchar_t title[80] = {};
  GetWindowText(dialog.hwnd, title, _countof(title));
  Check(std::wcsstr(title, wide ? L"PE32+" : L"PE32") != NULL, "Resource page format title mismatch");
  Check(TreeView_GetCount(tree) == 8, "Resource page tree node count mismatch");
  HTREEITEM root = TreeView_GetRoot(tree);
  HTREEITEM type = TreeView_GetChild(tree, root);
  HTREEITEM name = TreeView_GetChild(tree, type);
  HTREEITEM leaf = TreeView_GetChild(tree, name);
  text(type, L"Type: \"\u81EA\u5B9A\u4E49\u7C7B\u578B\"");
  text(name, L"Name: \"\u914D\u7F6E\"");
  text(leaf, L"Language: #1033 (0x0409)");
  text(TreeView_GetNextSibling(tree, type), L"Type: RCDATA (#10)");
  field(L"Directory RVA (hex)", L"00001900");
  TreeView_SelectItem(tree, leaf);
  field(L"Data RVA (hex)", L"00001D00");
  field(L"File offset (hex)", L"00000F00");
  field(L"Size (hex)", L"00000004");
  field(L"CodePage", L"1200");
  TreeView_SelectItem(tree, TreeView_GetNextSibling(tree, leaf));
  field(L"CodePage", L"65001");
  resource_fixture::Name(data + resource_fixture::kRaw, 0x1A0, L"A\0B", 3);
  showResourceDirectory(dialog.hwnd, g_peContext);
  type = TreeView_GetChild(tree, TreeView_GetRoot(tree));
  text(TreeView_GetChild(tree, type), L"Name: \"A\\u0000B\"");

  // Reload using an explicit context, while the active document still has valid resources.
  std::vector<BYTE> invalid(pe32_fixture::kFileSize);
  resource_fixture::Fill(invalid.data(), wide);
  PEContext context;
  Check(context.Initialize(invalid.data(), invalid.size()), "Resource reload context rejected");
  auto directory = const_cast<IMAGE_DATA_DIRECTORY*>(context.Directory(IMAGE_DIRECTORY_ENTRY_RESOURCE));
  resource_fixture::Entry(invalid.data() + resource_fixture::kRaw, 0x10, 0x80000180, 0x80000000);
  showResourceDirectory(dialog.hwnd, context);
  Check(TreeView_GetCount(tree) == 5, "Cyclic resource tree lost valid siblings or retained old nodes");
  wchar_t status[512] = {};
  GetDlgItemText(dialog.hwnd, IDC_STATIC_DIRECTORY_STATUS, status, _countof(status));
  Check(std::wcsstr(status, L"Cyclic") != NULL, "Resource cycle error is not visible");
  directory->VirtualAddress = 0x3000;
  showResourceDirectory(dialog.hwnd, context);
  Check(TreeView_GetCount(tree) == 0 && ListView_GetItemCount(list) == 0,
        "Invalid resource reload left stale tree or details");
  GetDlgItemText(dialog.hwnd, IDC_STATIC_DIRECTORY_STATUS, status, _countof(status));
  Check(std::wcsstr(status, L"outside") != NULL, "Resource range error is not visible");
  *directory = {};
  showResourceDirectory(dialog.hwnd, context);
  GetDlgItemText(dialog.hwnd, IDC_STATIC_DIRECTORY_STATUS, status, _countof(status));
  Check(std::wcsstr(status, L"No resource directory") != NULL, "Missing resource status is not visible");
}

void CheckExportForwarderPage(void* buffer, bool wide) {
  auto data = (BYTE*)buffer;
  if (wide) pe64_fixture::Fill(data); else pe32_fixture::Fill(data);
  pe32_fixture::FillForwardedExports(data);
  Check(g_peContext.Initialize(buffer, pe32_fixture::kFileSize), "Forwarder page context rejected");
  auto directory = const_cast<IMAGE_DATA_DIRECTORY*>(g_peContext.Directory(IMAGE_DIRECTORY_ENTRY_EXPORT));
  directory->Size = pe32_fixture::kForwardedExportSize;
  Dialog exports(IDD_DIALOG_EXPORT_DIRECTORY, ExportDirectoryProc);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 4, L"Export", 0, 3);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 1, L"00001190", 1, 3);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 2, L"00000390", 1, 3);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 3, L"ForwardedName", 1, 3);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 4, L"Forwarder", 1, 3);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 5, L"MYDLL.expfunc", 1, 3);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 5, L"MYDLL.#27", 2, 3);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 6, L"OK", 1, 3);
  HWND list = GetDlgItem(exports.hwnd, IDC_LIST_EXPORT_FUNC);
  Check(ListView_GetColumnWidth(list, 5) >= ListView_GetStringWidth(list, L"MYDLL.expfunc"),
        "Forwarder target column clips its content");
  wchar_t status[512] = {};
  GetDlgItemText(exports.hwnd, IDC_STATIC_DIRECTORY_STATUS, status, _countof(status));
  Check(std::wcsstr(status, L"2 forwarders") != NULL, "Forwarder count is not visible");
  // Retain the row and the distinction when the target lacks a null inside the directory.
  directory->Size = 0x1190 - directory->VirtualAddress + (DWORD)std::strlen("MYDLL.expfunc");
  showExportDirectory(exports.hwnd, g_peContext);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 4, L"Forwarder", 1, 3);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 5, L"-", 1, 3);
  wchar_t note[256] = {};
  ListView_GetItemText(list, 1, 6, note, _countof(note));
  Check(std::wcsstr(note, L"forwarder") != NULL, "Invalid forwarder row has no visible status");
  GetDlgItemText(exports.hwnd, IDC_STATIC_DIRECTORY_STATUS, status, _countof(status));
  Check(std::wcsstr(status, L"unterminated") != NULL, "Invalid forwarder is not reported in page status");
  directory->Size = pe32_fixture::kForwardedExportSize;
  ((PIMAGE_EXPORT_DIRECTORY)(data + 0x300))->NumberOfNames = 0;
  showExportDirectory(exports.hwnd, g_peContext);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 3, L"-", 1, 3);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 5, L"MYDLL.expfunc", 1, 3);
}

int CALLBACK ReverseDescriptorOrder(LPARAM left, LPARAM right, LPARAM) {
  return left > right ? -1 : left < right ? 1 : 0;
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
  showImportDirectory(imports.hwnd, dlls, context);
  ListView_SetItemState(dlls, 0, LVIS_SELECTED, LVIS_SELECTED);
  showImportThunks(imports.hwnd, dlls, thunks, context);
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 0, L"Ordinal #42");
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 2, L"00000460");

  auto exported = (PIMAGE_EXPORT_DIRECTORY)(image.data() + 0x300);
  exported->NumberOfNames = 0;
  exported->Base = 0x100;
  Dialog exports(IDD_DIALOG_EXPORT_DIRECTORY, ExportDirectoryProc);
  HWND functions = GetDlgItem(exports.hwnd, IDC_LIST_EXPORT_FUNC);
  ListView_DeleteAllItems(functions);
  showExportDirectory(exports.hwnd, context);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 0, L"0100");
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 2, L"00000A00");
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 3, L"-");

  ((PIMAGE_DEBUG_DIRECTORY)(image.data() + 0x600))->Type = IMAGE_DEBUG_TYPE_REPRO;
  Dialog debug(IDD_DIALOG_DEBUG_DIRECTORY, DebugDirectoryProc);
  HWND debugList = GetDlgItem(debug.hwnd, IDC_LIST_DEBUG_DIRECTORY);
  ListView_DeleteAllItems(debugList);
  showDebugDirectory(debug.hwnd, debugList, context);
  CheckList(debug.hwnd, IDC_LIST_DEBUG_DIRECTORY, 4, L"UNKNOWN(10)");

  // A row's descriptor identity must survive UI sorting and changed display text.
  auto directories = const_cast<IMAGE_DATA_DIRECTORY*>(context.dataDirectories);
  directories[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = 3 * sizeof(IMAGE_IMPORT_DESCRIPTOR);
  descriptor[1] = {};
  descriptor[1].OriginalFirstThunk = 0x1700;
  descriptor[1].FirstThunk = 0x1740;
  descriptor[1].Name = 0x1750;
  std::memcpy(image.data() + 0x950, "second.dll", 11);
  *(DWORD*)(image.data() + 0x900) = IMAGE_ORDINAL_FLAG32 | 7;
  showImportDirectory(imports.hwnd, dlls, context);
  ListView_SortItems(dlls, ReverseDescriptorOrder, 0);
  ListView_SetItemState(dlls, 0, LVIS_SELECTED, LVIS_SELECTED);
  showImportThunks(imports.hwnd, dlls, thunks, context);
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 0, L"Ordinal #7");

  auto nt = (PIMAGE_NT_HEADERS32)(image.data() + 0x80);
  nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = MAXDWORD;
  ListView_DeleteAllItems(dlls);
  showImportDirectory(imports.hwnd, dlls, context);
  Check(ListView_GetItemCount(dlls) == 0, "An invalid directory RVA was dereferenced");
  wchar_t status[512] = {};
  GetDlgItemText(imports.hwnd, IDC_STATIC_DIRECTORY_STATUS, status, _countof(status));
  Check(std::wcsstr(status, L"outside") != NULL, "Import parse failure is not visible");
  directories[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress = MAXDWORD;
  showExportDirectory(exports.hwnd, context);
  Check(ListView_GetItemCount(functions) == 0, "Invalid export left stale rows");
  CheckText(exports.hwnd, IDC_EDIT_EXPORT_DIRECTORY_BASE, L"");
  GetDlgItemText(exports.hwnd, IDC_STATIC_DIRECTORY_STATUS, status, _countof(status));
  Check(std::wcsstr(status, L"Invalid") != NULL, "Export parse failure is not visible");
  Dialog bound(IDD_DIALOG_BOUND_IMPORT_DIRECTORY, BoundImportDirectoryProc);
  directories[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = MAXDWORD;
  showBoundImportDirectory(bound.hwnd, GetDlgItem(bound.hwnd, IDC_LIST_BOUND_IMPORT_DIRECTORY), context);
  Check(ListView_GetItemCount(GetDlgItem(bound.hwnd, IDC_LIST_BOUND_IMPORT_DIRECTORY)) == 0,
        "Invalid bound import left stale rows");
  GetDlgItemText(bound.hwnd, IDC_STATIC_DIRECTORY_STATUS, status, _countof(status));
  Check(std::wcsstr(status, L"outside") != NULL, "Bound import parse failure is not visible");
  directories[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = MAXDWORD;
  showDebugDirectory(debug.hwnd, debugList, context);
  Check(ListView_GetItemCount(debugList) == 0, "Invalid debug directory left stale rows");
  GetDlgItemText(debug.hwnd, IDC_STATIC_DIRECTORY_STATUS, status, _countof(status));
  Check(std::wcsstr(status, L"outside") != NULL, "Debug parse failure is not visible");
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
    showImportDirectory(imports.hwnd, dlls, context);
    ListView_SetItemState(dlls, 0, LVIS_SELECTED, LVIS_SELECTED);
    // Rendering must obtain the lookup RVA from the context, not display text.
    ListView_SetItemText(dlls, 0, 1, const_cast<wchar_t*>(L"FFFFFFFF"));
    showImportThunks(imports.hwnd, dlls, thunks, context);
  };
  auto put = [&](DWORD raw, ULONGLONG value) {
    if (wide) std::memcpy(image.data() + raw, &value, sizeof(ULONGLONG));
    else { DWORD narrow = (DWORD)value; std::memcpy(image.data() + raw, &narrow, sizeof(DWORD)); }
  };
  render();
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 0, pe32_fixture::kImportName, 0, wide ? 3 : 1);
  auto directories = const_cast<IMAGE_DATA_DIRECTORY*>(context.dataDirectories);
  directories[IMAGE_DIRECTORY_ENTRY_IAT] = {};
  render();
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 4, L"1234", 0, wide ? 3 : 1);
  if (wide) CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 4, L"5678", 2, 3);
  descriptor->OriginalFirstThunk = 0;
  descriptor->TimeDateStamp = 0xFFFFFFFF;
  render();
  Check(ListView_GetItemCount(thunks) == 0, "A bound IAT replaced the missing lookup table");
  descriptor->TimeDateStamp = 0;
  render();
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 0, pe32_fixture::kImportName, 0, wide ? 3 : 1);
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 2, wide ? L"000004A0" : L"00000460", 0, wide ? 3 : 1);
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 4, L"1234", 0, wide ? 3 : 1);
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
    CheckResourceDetails();
    CheckTlsDetails();
    CheckDelayImportDetails();
    CheckExceptionDetails();
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
    CheckExportForwarderPage(allocatedBuffer, false);
    CheckExportForwarderPage(allocatedBuffer, true);
    CheckResourcePage(allocatedBuffer, false);
    CheckResourcePage(allocatedBuffer, true);
    CheckTlsPage(allocatedBuffer, false);
    CheckTlsPage(allocatedBuffer, true);
    CheckDelayImportPage(allocatedBuffer, false);
    CheckDelayImportPage(allocatedBuffer, true);
    CheckExceptionPage(allocatedBuffer);
    CheckDirectoryWindowBehavior(allocatedBuffer, false);
    CheckDirectoryWindowBehavior(allocatedBuffer, true);
    CheckDirectoryErrorPages(allocatedBuffer);
    for (int i = 2; i < argc; ++i) {
      std::wstring reference = std::wstring(argv[1]) + L".resources-" + std::to_wstring(i) + L".dll";
      CheckRealDirectoryFile(argv[i], reference.c_str());
    }
    std::printf("PASS: %s, AMD64 exception table, PE32/PE32+ pages, delay imports/RVA/VA, TLS fields/callbacks, resource tree, IAT/INT association, export forwarders, relocation blocks, malformed directories, 64-bit input/layout, name/ordinal thunks, bitmap\n",
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
