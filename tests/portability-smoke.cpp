#include <windows.h>
#include <commctrl.h>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <stdexcept>
#include <vector>

#include "pe/readPE.h"
#include "ui/windowProc.h"
#include "fixtures/pe32.h"

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

void CheckPEContext(LPCTSTR filePath);

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

void CheckList(HWND dialog, int controlId, int column, const wchar_t* expected) {
  HWND list = GetDlgItem(dialog, controlId);
  Check(list != NULL, "Expected list is missing");
  Check(ListView_GetItemCount(list) == 1, "Expected exactly one list row");
  wchar_t actual[MAX_PATH] = {};
  ListView_GetItemText(list, 0, column, actual, MAX_PATH);
  if (std::wcscmp(actual, expected) != 0) {
    std::fwprintf(stderr, L"List %d column %d: expected [%ls], got [%ls]\n", controlId, column, expected, actual);
    throw std::runtime_error("List value mismatch");
  }
}

void CheckPages() {
  Dialog editor(IDD_DIALOG_PE_EDITOR, PeEditorProc, (LPARAM)L"synthetic-pe32.exe");
  CheckText(editor.hwnd, IDC_EDIT_MAGIC_NUMBER, L"5A4D");
  Dialog fileHeader(IDD_DIALOG_IMAGE_FILE_HEADER, FileHeaderProc);
  CheckText(fileHeader.hwnd, IDC_EDIT_NUMBER_OF_SECTIONS, L"0001");
  Dialog optionalHeader(IDD_DIALOG_IMAGE_OPTIONAL_HEADER32, OptionalHeader32Proc);
  CheckText(optionalHeader.hwnd, IDC_EDIT_MAGIC, L"010B");
  CheckText(optionalHeader.hwnd, IDC_EDIT_IMAGE_BASE, L"00400000");
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
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 0, pe32_fixture::kImportName);
  CheckList(imports.hwnd, IDC_LIST_INT_THUNK, 4, L"1234");
  Dialog exports(IDD_DIALOG_EXPORT_DIRECTORY, ExportDirectoryProc);
  CheckList(exports.hwnd, IDC_LIST_EXPORT_FUNC, 3, pe32_fixture::kExportName);
  Dialog boundImports(IDD_DIALOG_BOUND_IMPORT_DIRECTORY, BoundImportDirectoryProc);
  CheckList(boundImports.hwnd, IDC_LIST_BOUND_IMPORT_DIRECTORY, 0, pe32_fixture::kBoundDll);
  Dialog debug(IDD_DIALOG_DEBUG_DIRECTORY, DebugDirectoryProc);
  CheckList(debug.hwnd, IDC_LIST_DEBUG_DIRECTORY, 4, L"CODEVIEW(2)");
  CheckList(debug.hwnd, IDC_LIST_DEBUG_DIRECTORY, 7, L"00000650");

  HBITMAP bitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP_PEVIEWER));
  Check(bitmap != NULL, "Bitmap resource is missing");
  DeleteObject(bitmap);
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
}  // namespace

int wmain(int argc, wchar_t** argv) {
  std::setvbuf(stdout, NULL, _IONBF, 0);
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
  LPVOID allocatedBuffer = NULL;
  try {
    Check(argc == 2, "Usage: portability-smoke.exe <generated-fixture-path>");
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
    std::printf("PASS: %s, PE32 reader, 10 dialogs, headers, sections, imports, exports, bound imports, debug, bitmap\n",
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
