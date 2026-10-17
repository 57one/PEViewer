#include "readPE.h"

#include "common/global.h"

#include <cstddef>
#include <climits>

VOID readDosHeader(HWND hwnd, const PEContext& context) {
  if (!context.IsValid()) return;
  // set DosHeader Text
  writeToText(hwnd, IDC_EDIT_MAGIC_NUMBER, TEXT("%04X"), context.dosHeader->e_magic);
  writeToText(hwnd, IDC_EDIT_BLP, TEXT("%04X"), context.dosHeader->e_cblp);
  writeToText(hwnd, IDC_EDIT_PAGES, TEXT("%04X"), context.dosHeader->e_cp);
  writeToText(hwnd, IDC_EDIT_RLC, TEXT("%04X"), context.dosHeader->e_crlc);
  writeToText(hwnd, IDC_EDIT_PARHDR, TEXT("%04X"), context.dosHeader->e_cparhdr);
  writeToText(hwnd, IDC_EDIT_MIN_ALLOC, TEXT("%04X"), context.dosHeader->e_minalloc);
  writeToText(hwnd, IDC_EDIT_MAX_ALLOC, TEXT("%04X"), context.dosHeader->e_maxalloc);
  writeToText(hwnd, IDC_EDIT_INIT_SS, TEXT("%04X"), context.dosHeader->e_ss);
  writeToText(hwnd, IDC_EDIT_INIT_SP, TEXT("%04X"), context.dosHeader->e_sp);
  writeToText(hwnd, IDC_EDIT_CHECKSUM, TEXT("%04X"), context.dosHeader->e_csum);
  writeToText(hwnd, IDC_EDIT_INIT_IP, TEXT("%04X"), context.dosHeader->e_ip);
  writeToText(hwnd, IDC_EDIT_INIT_CS, TEXT("%04X"), context.dosHeader->e_cs);
  writeToText(hwnd, IDC_EDIT_LFARLC, TEXT("%04X"), context.dosHeader->e_lfarlc);
  writeToText(hwnd, IDC_EDIT_OVNO, TEXT("%04X"), context.dosHeader->e_ovno);
  writeToText(hwnd, IDC_EDIT_OEM_ID, TEXT("%04X"), context.dosHeader->e_oemid);
  writeToText(hwnd, IDC_EDIT_OEM_INFO, TEXT("%04X"), context.dosHeader->e_oeminfo);
  writeToText(hwnd, IDC_EDIT_PE_ADDRESS, TEXT("%08X"), context.dosHeader->e_lfanew);
}

VOID readFileHeader(HWND hwnd, const PEContext& context) {
  if (!context.IsValid()) return;
  // set FileHeader
  writeToText(hwnd, IDC_EDIT_MACHINE, TEXT("%04X"), context.fileHeader->Machine);
  writeToText(hwnd, IDC_EDIT_NUMBER_OF_SECTIONS, TEXT("%04X"),
              context.fileHeader->NumberOfSections);
  writeToText(hwnd, IDC_EDIT_TIMEDATESTAMP, TEXT("%08X"),
              context.fileHeader->TimeDateStamp);
  writeToText(hwnd, IDC_EDIT_P_SYMTAB, TEXT("%08X"),
              context.fileHeader->PointerToSymbolTable);
  writeToText(hwnd, IDC_EDIT_NUMBER_OF_SYMBOL, TEXT("%08X"),
              context.fileHeader->NumberOfSymbols);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_OPT_HEADER, TEXT("%04X"),
              context.fileHeader->SizeOfOptionalHeader);
  writeToText(hwnd, IDC_EDIT_CHARACTERISTICS, TEXT("%04X"),
              context.fileHeader->Characteristics);
}

namespace {
template <typename OptionalHeader>
VOID readOptionalHeaderFields(HWND hwnd, const OptionalHeader& header) {
  writeToText(hwnd, IDC_EDIT_MAGIC, TEXT("%04X"), header.Magic);
  writeToText(hwnd, IDC_EDIT_MAJ_LINK_VER, TEXT("%02X"),
              header.MajorLinkerVersion);
  writeToText(hwnd, IDC_EDIT_MIN_LINK_VER, TEXT("%02X"),
              header.MinorLinkerVersion);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_CODE, TEXT("%08X"),
              header.SizeOfCode);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_INIT_DATA, TEXT("%08X"),
              header.SizeOfInitializedData);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_UNINIT_DATA, TEXT("%08X"),
              header.SizeOfUninitializedData);

  writeToText(hwnd, IDC_EDIT_ADDR_OF_ENTRY, TEXT("%08X"),
              header.AddressOfEntryPoint);
  writeToText(hwnd, IDC_EDIT_BASE_OF_CODE, TEXT("%08X"),
              header.BaseOfCode);
  writeToText(hwnd, IDC_EDIT_IMAGE_BASE, header.ImageBase);
  writeToText(hwnd, IDC_EDIT_SECTION_ALIGNMENT, TEXT("%08X"),
              header.SectionAlignment);
  writeToText(hwnd, IDC_EDIT_FILE_ALIGNMENT, TEXT("%08X"),
              header.FileAlignment);
  writeToText(hwnd, IDC_EDIT_MAJ_OS_VER, TEXT("%04X"),
              header.MajorOperatingSystemVersion);
  writeToText(hwnd, IDC_EDIT_MIN_OS_VER, TEXT("%04X"),
              header.MinorOperatingSystemVersion);
  writeToText(hwnd, IDC_EDIT_MAJ_IMAGE_VER, TEXT("%04X"),
              header.MajorImageVersion);
  writeToText(hwnd, IDC_EDIT_MIN_IMAGE_VER, TEXT("%04X"),
              header.MinorImageVersion);
  writeToText(hwnd, IDC_EDIT_MAJ_SUBSTM_VER, TEXT("%04X"),
              header.MajorSubsystemVersion);
  writeToText(hwnd, IDC_EDIT_MIN_SUBSTM_VER, TEXT("%04X"),
              header.MinorSubsystemVersion);
  writeToText(hwnd, IDC_EDIT_WIN32_VER_VAL, TEXT("%08X"),
              header.Win32VersionValue);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_IMAGE, TEXT("%08X"),
              header.SizeOfImage);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_HEADERS, TEXT("%08X"),
              header.SizeOfHeaders);
  writeToText(hwnd, IDC_EDIT_CHECKSUM, TEXT("%08X"),
              header.CheckSum);
  writeToText(hwnd, IDC_EDIT_SUB_STM, TEXT("%04X"),
              header.Subsystem);
  writeToText(hwnd, IDC_EDIT_DLL_CHARAC, TEXT("%04X"),
              header.DllCharacteristics);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_STACK_REV, header.SizeOfStackReserve);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_STACK_COMMIT, header.SizeOfStackCommit);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_HEAP_REV, header.SizeOfHeapReserve);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_HEAP_COMMIT, header.SizeOfHeapCommit);
  writeToText(hwnd, IDC_EDIT_LOADER_FLAGS, TEXT("%08X"),
              header.LoaderFlags);
  writeToText(hwnd, IDC_EDIT_NUM_OF_RVA_SIZES, TEXT("%08X"),
              header.NumberOfRvaAndSizes);
}

template <typename OptionalHeader>
VOID readHeaderInfoFields(HWND hwnd, const PEContext& context,
                          const OptionalHeader& header) {
  writeToText(hwnd, IDC_EDIT_HEADER_ADDR_OF_ENTRY, TEXT("%08X"),
              header.AddressOfEntryPoint);
  writeToText(hwnd, IDC_EDIT_HEADER_IMAGE_BASE, header.ImageBase);
  writeToText(hwnd, IDC_EDIT_HEADER_SIZE_OF_IMAGE, TEXT("%08X"),
              header.SizeOfImage);
  writeToText(hwnd, IDC_EDIT_HEADER_BASE_OF_CODE, TEXT("%08X"),
              header.BaseOfCode);
  writeToText(hwnd, IDC_EDIT_HEADER_SECTION_ALIGNMENT, TEXT("%08X"),
              header.SectionAlignment);
  writeToText(hwnd, IDC_EDIT_HEADER_FILE_ALIGNMENT, TEXT("%08X"),
              header.FileAlignment);
  writeToText(hwnd, IDC_EDIT_HEADER_MAGIC, TEXT("%04X"),
              header.Magic);
  writeToText(hwnd, IDC_EDIT_HEADER_SUB_STM, TEXT("%04X"),
              header.Subsystem);
  writeToText(hwnd, IDC_EDIT_HEADER_NUMBER_OF_SECTIONS, TEXT("%04X"),
              context.fileHeader->NumberOfSections);
  writeToText(hwnd, IDC_EDIT_HEADER_TIMEDATESTAMP, TEXT("%08X"),
              context.fileHeader->TimeDateStamp);
  writeToText(hwnd, IDC_EDIT_HEADER_SIZE_OF_HEADERS, TEXT("%08X"),
              header.SizeOfHeaders);
  writeToText(hwnd, IDC_EDIT_HEADER_CHARACTERISTICS, TEXT("%04X"),
              context.fileHeader->Characteristics);
  writeToText(hwnd, IDC_EDIT_HEADER_CHECKSUM, TEXT("%08X"),
              header.CheckSum);
  writeToText(hwnd, IDC_EDIT_HEADER_SIZE_OF_OPT_HEADER, TEXT("%04X"),
              context.fileHeader->SizeOfOptionalHeader);
  writeToText(hwnd, IDC_EDIT_HEADER_NUM_OF_RVA_SIZES, TEXT("%08X"),
              header.NumberOfRvaAndSizes);
}

}  // namespace

VOID readOptionalHeader(HWND hwnd, const PEContext& context) {
  if (context.format == PEFormat::PE32 && context.optionalHeader32) {
    SetWindowText(hwnd, TEXT("[IMAGE_OPTIONAL_HEADER32]"));
    SetDlgItemText(hwnd, IDC_GROUP_OPTIONAL_HEADER32, TEXT("Optional Header32 [HEX]"));
    readOptionalHeaderFields(hwnd, *context.optionalHeader32);
    TCHAR baseOfData[9] = {};
    _stprintf_s(baseOfData, TEXT("%08X"), context.optionalHeader32->BaseOfData);
    writeText(hwnd, IDC_EDIT_BASE_OF_DATA, baseOfData);
  } else if (context.format == PEFormat::PE32Plus && context.optionalHeader64) {
    SetWindowText(hwnd, TEXT("[IMAGE_OPTIONAL_HEADER64]"));
    SetDlgItemText(hwnd, IDC_GROUP_OPTIONAL_HEADER32, TEXT("Optional Header64 [HEX]"));
    readOptionalHeaderFields(hwnd, *context.optionalHeader64);
    writeText(hwnd, IDC_EDIT_BASE_OF_DATA, NULL);
  }
}

VOID readHeaderInfo(HWND hwnd, const PEContext& context) {
  if (context.format == PEFormat::PE32 && context.optionalHeader32) {
    SetWindowText(hwnd, TEXT("[Header Info - PE32]"));
    readHeaderInfoFields(hwnd, context, *context.optionalHeader32);
    TCHAR baseOfData[9] = {};
    _stprintf_s(baseOfData, TEXT("%08X"), context.optionalHeader32->BaseOfData);
    writeText(hwnd, IDC_EDIT_HEADER_BASE_OF_DATA, baseOfData);
  } else if (context.format == PEFormat::PE32Plus && context.optionalHeader64) {
    SetWindowText(hwnd, TEXT("[Header Info - PE32+]"));
    readHeaderInfoFields(hwnd, context, *context.optionalHeader64);
    writeText(hwnd, IDC_EDIT_HEADER_BASE_OF_DATA, NULL);
  }
}

VOID drawSections(HWND hListSection, int Row, SectionInfo& sectionInfo,
                  COLORREF colorRef) {
  LV_ITEM vitem;
  //初始化
  memset(&vitem, 0, sizeof(LV_ITEM));
  vitem.mask = LVIF_TEXT;

  vitem.pszText = sectionInfo.szIndex;
  vitem.iItem = Row;   //第几行
  vitem.iSubItem = 0;  //第几列
  SendMessage(hListSection, LVM_INSERTITEM, 0, (LPARAM)&vitem);
  SendMessage(hListSection, LVM_SETINSERTMARKCOLOR, 0, (DWORD)colorRef);

  //第一行写完之后 后面全是 SETITEM
  vitem.pszText = sectionInfo.szSectionName;
  vitem.iSubItem = 1;
  ListView_SetItem(hListSection, &vitem);

  vitem.pszText = sectionInfo.szVirtualSize;
  vitem.iSubItem = 2;
  ListView_SetItem(hListSection, &vitem);

  vitem.pszText = sectionInfo.szVirtualOffset;
  vitem.iSubItem = 3;
  ListView_SetItem(hListSection, &vitem);

  vitem.pszText = sectionInfo.szRawSize;
  vitem.iSubItem = 4;
  ListView_SetItem(hListSection, &vitem);

  vitem.pszText = sectionInfo.szRawOffset;
  vitem.iSubItem = 5;
  ListView_SetItem(hListSection, &vitem);

  vitem.pszText = sectionInfo.szCharacteristics;
  vitem.iSubItem = 6;
  ListView_SetItem(hListSection, &vitem);
}

VOID readSections(HWND hwnd, HWND hListSection, const PEContext& context) {
  if (!context.IsValid()) return;
  for (WORD i = 0; i < context.sectionCount; ++i) {
    const IMAGE_SECTION_HEADER* section = context.sectionHeaders + i;
    SectionInfo info;
    char name[IMAGE_SIZEOF_SHORT_NAME + 1] = {};
    memcpy(name, section->Name, IMAGE_SIZEOF_SHORT_NAME);
    MultiByteToWideChar(CP_OEMCP, 0, name, -1, info.szSectionName, MAX_PATH);
    wsprintf(info.szIndex, TEXT("%u"), i);
    wsprintf(info.szVirtualSize, TEXT("%08X"), section->Misc.VirtualSize);
    wsprintf(info.szVirtualOffset, TEXT("%08X"), section->VirtualAddress);
    wsprintf(info.szRawSize, TEXT("%08X"), section->SizeOfRawData);
    wsprintf(info.szRawOffset, TEXT("%08X"), section->PointerToRawData);
    wsprintf(info.szCharacteristics, TEXT("%08X"), section->Characteristics);
    drawSections(hListSection, i, info, colorWhite);
  }
}

VOID readDirectory(HWND hwnd, const PEContext& context) {
  if (!context.IsValid()) return;
  for (DWORD i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; ++i) {
    const IMAGE_DATA_DIRECTORY* entry =
        i < context.directoryCount ? context.dataDirectories + i : NULL;
    writeToText(hwnd, directoryRVAEditID[i], TEXT("%08X"), entry ? entry->VirtualAddress : 0);
    writeToText(hwnd, directorySizeEditID[i], TEXT("%08X"), entry ? entry->Size : 0);
    BOOL present = context.Directory(i) != NULL;
    EnableWindow(GetDlgItem(hwnd, directoryMoreInfoButtonID[i]), present);
    EnableWindow(GetDlgItem(hwnd, directoryHexButtonID[i]), present);
  }
}

VOID readSectionEdit(HWND hwnd, INT ID, const PEContext& context) {
  if (!context.IsValid() || ID < 0 || ID >= context.sectionCount) return;
  const IMAGE_SECTION_HEADER* section = context.sectionHeaders + ID;
  char name[IMAGE_SIZEOF_SHORT_NAME + 1] = {};
  TCHAR sectionName[MAX_PATH] = {};
  memcpy(name, section->Name, IMAGE_SIZEOF_SHORT_NAME);
  MultiByteToWideChar(CP_OEMCP, 0, name, -1, sectionName, MAX_PATH);
  SetWindowText(GetDlgItem(hwnd, IDC_EDIT_SECTION_NAME), sectionName);
  writeToText(hwnd, IDC_EDIT_SECTION_VIRTUAL_SIZE, TEXT("%08X"), section->Misc.VirtualSize);
  writeToText(hwnd, IDC_EDIT_SECTION_VIRTUAL_OFFSET, TEXT("%08X"), section->VirtualAddress);
  writeToText(hwnd, IDC_EDIT_SECTION_RAW_SIZE, TEXT("%08X"), section->SizeOfRawData);
  writeToText(hwnd, IDC_EDIT_SECTION_RAW_OffSET, TEXT("%08X"), section->PointerToRawData);
  writeToText(hwnd, IDC_EDIT_SECTION_CHARACTERISTICS, TEXT("%08X"), section->Characteristics);
}

VOID drawDllImportDirectory(HWND hListImportDLL, int Row,
                            ImportDescInfo& importDescInfo, COLORREF colorRef) {
  LV_ITEM vitem;
  //初始化
  memset(&vitem, 0, sizeof(LV_ITEM));
  vitem.mask = LVIF_TEXT;

  vitem.pszText = importDescInfo.szDllName;
  vitem.iItem = Row;   //第几行
  vitem.iSubItem = 0;  //第几列
  SendMessage(hListImportDLL, LVM_INSERTITEM, 0, (LPARAM)&vitem);
  SendMessage(hListImportDLL, LVM_SETINSERTMARKCOLOR, 0, (DWORD)colorRef);

  vitem.pszText = importDescInfo.szOriginalFirstThunk;
  vitem.iSubItem = 1;
  ListView_SetItem(hListImportDLL, &vitem);

  vitem.pszText = importDescInfo.szTimeDateStamp;
  vitem.iSubItem = 2;
  ListView_SetItem(hListImportDLL, &vitem);

  vitem.pszText = importDescInfo.szForwarderChain;
  vitem.iSubItem = 3;
  ListView_SetItem(hListImportDLL, &vitem);

  vitem.pszText = importDescInfo.szName;
  vitem.iSubItem = 4;
  ListView_SetItem(hListImportDLL, &vitem);

  vitem.pszText = importDescInfo.szFirstThunk;
  vitem.iSubItem = 5;
  ListView_SetItem(hListImportDLL, &vitem);
}

VOID readImportDirectory(HWND hwnd, HWND hListImportDLL, const PEContext& context) {
  const IMAGE_DATA_DIRECTORY* directory = context.Directory(IMAGE_DIRECTORY_ENTRY_IMPORT);
  if (!directory) return;
  const BYTE* descriptors = context.ReadRva(directory->VirtualAddress, directory->Size);
  if (!descriptors) return;
  DWORD row = 0;
  for (SIZE_T pos = 0; directory->Size - pos >= sizeof(IMAGE_IMPORT_DESCRIPTOR);
       pos += sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
    const IMAGE_IMPORT_DESCRIPTOR* descriptor = (const IMAGE_IMPORT_DESCRIPTOR*)(descriptors + pos);
    if (!descriptor->OriginalFirstThunk && !descriptor->FirstThunk) break;
    const char* name = context.ReadString(descriptor->Name);
    if (!name) return;
    ImportDescInfo info;
    MultiByteToWideChar(CP_OEMCP, 0, name, -1, info.szDllName, MAX_PATH);
    wsprintf(info.szOriginalFirstThunk, TEXT("%08X"), descriptor->OriginalFirstThunk);
    wsprintf(info.szTimeDateStamp, TEXT("%08X"), descriptor->TimeDateStamp);
    wsprintf(info.szForwarderChain, TEXT("%08X"), descriptor->ForwarderChain);
    wsprintf(info.szName, TEXT("%08X"), descriptor->Name);
    wsprintf(info.szFirstThunk, TEXT("%08X"), descriptor->FirstThunk);
    drawDllImportDirectory(hListImportDLL, row++, info, colorWhite);
  }
}

VOID drawIntThunk(HWND hListIntThunk, int Row, IntThunkInfo& intThunkInfo,
                  COLORREF colorRef) {
  LV_ITEM vitem;
  //初始化
  memset(&vitem, 0, sizeof(LV_ITEM));
  vitem.mask = LVIF_TEXT;

  vitem.pszText = intThunkInfo.szAPIName;
  vitem.iItem = Row;   //第几行
  vitem.iSubItem = 0;  //第几列
  SendMessage(hListIntThunk, LVM_INSERTITEM, 0, (LPARAM)&vitem);
  SendMessage(hListIntThunk, LVM_SETINSERTMARKCOLOR, 0, (DWORD)colorRef);

  vitem.pszText = intThunkInfo.szThunkRVA;
  vitem.iSubItem = 1;
  ListView_SetItem(hListIntThunk, &vitem);

  vitem.pszText = intThunkInfo.szThunkRAW;
  vitem.iSubItem = 2;
  ListView_SetItem(hListIntThunk, &vitem);

  vitem.pszText = intThunkInfo.szThunkValue;
  vitem.iSubItem = 3;
  ListView_SetItem(hListIntThunk, &vitem);

  vitem.pszText = intThunkInfo.szHint;
  vitem.iSubItem = 4;
  ListView_SetItem(hListIntThunk, &vitem);
}

namespace {
template <typename Thunk>
VOID readImportThunks(HWND list, DWORD thunkRva, const PEContext& context) {
  const ULONGLONG ordinalFlag = sizeof(Thunk) == sizeof(IMAGE_THUNK_DATA64)
                                   ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32;
  for (int row = 0; row < INT_MAX; ++row) {
    ULONGLONG rva = (ULONGLONG)thunkRva + (ULONGLONG)row * sizeof(Thunk);
    if (rva > MAXDWORD) return;
    const BYTE* bytes = context.ReadRva((DWORD)rva, sizeof(Thunk));
    if (!bytes) return;
    Thunk thunk = {};
    memcpy(&thunk, bytes, sizeof(thunk));
    ULONGLONG value = thunk.u1.Ordinal;
    if (!value) return;
    SIZE_T offset = 0;
    if (!context.RvaToFileOffset((DWORD)rva, sizeof(thunk), offset)) return;
    IntThunkInfo info;
    wsprintf(info.szThunkRVA, TEXT("%010X"), (DWORD)rva);
    _stprintf_s(info.szThunkRAW, TEXT("%08IX"), offset);
    _stprintf_s(info.szThunkValue, sizeof(Thunk) == 8 ? TEXT("%016I64X") : TEXT("%08I64X"), value);
    if ((value & ordinalFlag) != 0) {
      if ((value & ~(ordinalFlag | 0xFFFFULL)) != 0) return;
      wsprintf(info.szAPIName, TEXT("Ordinal #%u"), (WORD)(value & 0xFFFF));
      wsprintf(info.szHint, TEXT("-"));
    } else {
      // Name imports carry a 31-bit RVA even in a PE32+ lookup table.
      // Reject bound addresses and reserved high bits before narrowing.
      if (value > 0x7FFFFFFFULL) return;
      DWORD nameRva = (DWORD)value;
      const IMAGE_IMPORT_BY_NAME* imported =
          (const IMAGE_IMPORT_BY_NAME*)context.ReadRva(nameRva, sizeof(WORD));
      if (!imported || nameRva > MAXDWORD - offsetof(IMAGE_IMPORT_BY_NAME, Name)) return;
      const char* name = context.ReadString(nameRva + (DWORD)offsetof(IMAGE_IMPORT_BY_NAME, Name));
      if (!name) return;
      MultiByteToWideChar(CP_OEMCP, 0, name, -1, info.szAPIName, MAX_PATH);
      wsprintf(info.szHint, TEXT("%04X"), imported->Hint);
    }
    drawIntThunk(list, row, info, colorWhite);
  }
}
}  // namespace

VOID readIntThunk(HWND hwnd, HWND hListImportDLL, HWND hListIntThunk, const PEContext& context) {
  ListView_DeleteAllItems(hListIntThunk);
  if (!context.IsValid()) return;
  int selected = ListView_GetNextItem(hListImportDLL, -1, LVNI_SELECTED);
  const IMAGE_DATA_DIRECTORY* directory = context.Directory(IMAGE_DIRECTORY_ENTRY_IMPORT);
  if (selected < 0 || !directory ||
      (SIZE_T)selected >= directory->Size / sizeof(IMAGE_IMPORT_DESCRIPTOR)) return;
  const BYTE* bytes = context.ReadRva(directory->VirtualAddress, directory->Size);
  if (!bytes) return;
  const IMAGE_IMPORT_DESCRIPTOR* descriptor =
      (const IMAGE_IMPORT_DESCRIPTOR*)(bytes + (SIZE_T)selected * sizeof(IMAGE_IMPORT_DESCRIPTOR));
  // A bound IAT contains resolved addresses and cannot replace a missing lookup table.
  if (!descriptor->OriginalFirstThunk && descriptor->TimeDateStamp) return;
  DWORD thunkRva = descriptor->OriginalFirstThunk ? descriptor->OriginalFirstThunk
                                                : descriptor->FirstThunk;
  if (!thunkRva) return;
  if (context.format == PEFormat::PE32Plus)
    readImportThunks<IMAGE_THUNK_DATA64>(hListIntThunk, thunkRva, context);
  else
    readImportThunks<IMAGE_THUNK_DATA32>(hListIntThunk, thunkRva, context);
  ListView_SetColumnWidth(hListIntThunk, 2, LVSCW_AUTOSIZE_USEHEADER);
  ListView_SetColumnWidth(hListIntThunk, 3, LVSCW_AUTOSIZE_USEHEADER);
}

VOID writeText(HWND hwnd, INT controlId, LPCTSTR text) {
  SetDlgItemText(hwnd, controlId, text ? text : TEXT("N/A"));
  EnableWindow(GetDlgItem(hwnd, controlId), text != NULL);
}

VOID writeToText(HWND hwnd, INT TEXT_ID, CONST TCHAR* format, DWORD data) {
  TCHAR buffer[MAX_PATH] = {};
  _stprintf_s(buffer, format, data);
  SetWindowText(GetDlgItem(hwnd, TEXT_ID), buffer);
}

VOID writeToText(HWND hwnd, INT TEXT_ID, DWORD data) {
  SendDlgItemMessage(hwnd, TEXT_ID, EM_SETLIMITTEXT, 10, 0);
  writeToText(hwnd, TEXT_ID, TEXT("%08X"), data);
}

VOID writeToText(HWND hwnd, INT TEXT_ID, ULONGLONG data) {
  SendDlgItemMessage(hwnd, TEXT_ID, EM_SETLIMITTEXT, 18, 0);
  TCHAR buffer[MAX_PATH] = {};
  _stprintf_s(buffer, TEXT("%016I64X"), data);
  SetWindowText(GetDlgItem(hwnd, TEXT_ID), buffer);
}

VOID readExportDirectory(HWND hwnd, const PEContext& context) {
  const IMAGE_DATA_DIRECTORY* directory = context.Directory(IMAGE_DIRECTORY_ENTRY_EXPORT);
  if (!directory) return;
  const IMAGE_EXPORT_DIRECTORY* pExportDirectory =
      (const IMAGE_EXPORT_DIRECTORY*)context.ReadRva(directory->VirtualAddress, sizeof(IMAGE_EXPORT_DIRECTORY));
  if (!pExportDirectory) return;

  writeToText(hwnd, IDC_EDIT_EXPORT_DIRECTORY_CHARAC, TEXT("%08X"),
              pExportDirectory->Characteristics);
  writeToText(hwnd, IDC_EDIT_EXPORT_DIRECTORY_TIMEDATESTAMP, TEXT("%08X"),
              pExportDirectory->TimeDateStamp);
  writeToText(hwnd, IDC_EDIT_EXPORT_DIRECTORY_MAJ_VER, TEXT("%04X"),
              pExportDirectory->MajorVersion);
  writeToText(hwnd, IDC_EDIT_EXPORT_DIRECTORY_MIN_VER, TEXT("%04X"),
              pExportDirectory->MinorVersion);
  writeToText(hwnd, IDC_EDIT_EXPORT_DIRECTORY_NAME, TEXT("%08X"),
              pExportDirectory->Name);
  writeToText(hwnd, IDC_EDIT_EXPORT_DIRECTORY_BASE, TEXT("%08X"),
              pExportDirectory->Base);
  writeToText(hwnd, IDC_EDIT_EXPORT_DIRECTORY_NUM_OF_FUNCS, TEXT("%08X"),
              pExportDirectory->NumberOfFunctions);
  writeToText(hwnd, IDC_EDIT_EXPORT_DIRECTORY_NUM_OF_NAMES, TEXT("%08X"),
              pExportDirectory->NumberOfNames);
  writeToText(hwnd, IDC_EDIT_EXPORT_DIRECTORY_ADDR_OF_FUNCS, TEXT("%08X"),
              pExportDirectory->AddressOfFunctions);
  writeToText(hwnd, IDC_EDIT_EXPORT_DIRECTORY_ADDR_OF_NAMES, TEXT("%08X"),
              pExportDirectory->AddressOfNames);
  writeToText(hwnd, IDC_EDIT_EXPORT_DIRECTORY_ADDR_OF_NAME_ORDS, TEXT("%08X"),
              pExportDirectory->AddressOfNameOrdinals);
}

VOID drawExportFunc(HWND hListExportFunc, int Row,
                    ExportFuncInfo& exportFuncInfo, COLORREF colorRef) {
  LV_ITEM vitem;
  //初始化
  memset(&vitem, 0, sizeof(LV_ITEM));
  vitem.mask = LVIF_TEXT;

  vitem.pszText = exportFuncInfo.szOrdinal;
  vitem.iItem = Row;   //第几行
  vitem.iSubItem = 0;  //第几列
  SendMessage(hListExportFunc, LVM_INSERTITEM, 0, (LPARAM)&vitem);
  SendMessage(hListExportFunc, LVM_SETINSERTMARKCOLOR, 0, (DWORD)colorRef);

  vitem.pszText = exportFuncInfo.szFuncRVA;
  vitem.iSubItem = 1;
  ListView_SetItem(hListExportFunc, &vitem);

  vitem.pszText = exportFuncInfo.szFuncRAW;
  vitem.iSubItem = 2;
  ListView_SetItem(hListExportFunc, &vitem);

  vitem.pszText = exportFuncInfo.szFuncName;
  vitem.iSubItem = 3;
  ListView_SetItem(hListExportFunc, &vitem);
}

VOID readExportFunc(HWND hwnd, HWND hListExportFunc, const PEContext& context) {
  const IMAGE_DATA_DIRECTORY* directory = context.Directory(IMAGE_DIRECTORY_ENTRY_EXPORT);
  if (!directory) return;
  const IMAGE_EXPORT_DIRECTORY* exports =
      (const IMAGE_EXPORT_DIRECTORY*)context.ReadRva(directory->VirtualAddress, sizeof(IMAGE_EXPORT_DIRECTORY));
  if (!exports || exports->NumberOfFunctions > context.fileSize / sizeof(DWORD) ||
      exports->NumberOfNames > context.fileSize / sizeof(DWORD)) return;
  const DWORD* functions = (const DWORD*)context.ReadRva(
      exports->AddressOfFunctions, (SIZE_T)exports->NumberOfFunctions * sizeof(DWORD));
  if (!functions) return;
  std::map<DWORD, DWORD> namesByFunction;
  if (exports->NumberOfNames) {
    const DWORD* names = (const DWORD*)context.ReadRva(
        exports->AddressOfNames, (SIZE_T)exports->NumberOfNames * sizeof(DWORD));
    const WORD* ordinals = (const WORD*)context.ReadRva(
        exports->AddressOfNameOrdinals, (SIZE_T)exports->NumberOfNames * sizeof(WORD));
    if (!names || !ordinals) return;
    for (DWORD i = 0; i < exports->NumberOfNames; ++i) {
      if (ordinals[i] >= exports->NumberOfFunctions) return;
      namesByFunction.insert({ordinals[i], names[i]});
    }
  }
  DWORD row = 0;
  for (DWORD i = 0; i < exports->NumberOfFunctions; ++i) {
    if (!functions[i]) continue;
    ExportFuncInfo info;
    auto name = namesByFunction.find(i);
    if (name == namesByFunction.end()) {
      wsprintf(info.szFuncName, TEXT("-"));
    } else {
      const char* text = context.ReadString(name->second);
      if (!text) return;
      MultiByteToWideChar(CP_OEMCP, 0, text, -1, info.szFuncName, MAX_PATH);
    }
    if (i > MAXDWORD - exports->Base) return;
    wsprintf(info.szOrdinal, TEXT("%04X"), exports->Base + i);
    wsprintf(info.szFuncRVA, TEXT("%08X"), functions[i]);
    SIZE_T offset = 0;
    if (context.RvaToFileOffset(functions[i], 1, offset))
      _stprintf_s(info.szFuncRAW, TEXT("%08IX"), offset);
    else
      wsprintf(info.szFuncRAW, TEXT("-"));
    drawExportFunc(hListExportFunc, row++, info, colorWhite);
  }
}

VOID drawBoundImportDirectory(HWND hBoundImport, int row,
    BoundImportInfo& boundImportInfo,
    COLORREF colorRef) {
  LV_ITEM vitem;
  //初始化
  memset(&vitem, 0, sizeof(LV_ITEM));
  vitem.mask = LVIF_TEXT;

  vitem.pszText = boundImportInfo.szDllName;
  vitem.iItem = row;   //第几行
  vitem.iSubItem = 0;  //第几列
  SendMessage(hBoundImport, LVM_INSERTITEM, 0, (LPARAM)&vitem);
  SendMessage(hBoundImport, LVM_SETINSERTMARKCOLOR, 0, (DWORD)colorRef);

  vitem.pszText = boundImportInfo.szTimeDateStamp;
  vitem.iSubItem = 1;
  ListView_SetItem(hBoundImport, &vitem);

  vitem.pszText = boundImportInfo.szOffsetModuleName;
  vitem.iSubItem = 2;
  ListView_SetItem(hBoundImport, &vitem);

  vitem.pszText = boundImportInfo.szNumberOfModuleForwarderRefs;
  vitem.iSubItem = 3;
  ListView_SetItem(hBoundImport, &vitem);
}

VOID readBoundImportDirectory(HWND hwnd, HWND hBoundImport, const PEContext& context) {
  const IMAGE_DATA_DIRECTORY* directory = context.Directory(IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT);
  if (!directory) return;
  const BYTE* data = context.ReadRva(directory->VirtualAddress, directory->Size);
  if (!data) return;
  DWORD row = 0;
  SIZE_T pos = 0;
  while (directory->Size - pos >= sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR)) {
    const IMAGE_BOUND_IMPORT_DESCRIPTOR* descriptor =
        (const IMAGE_BOUND_IMPORT_DESCRIPTOR*)(data + pos);
    if (!descriptor->TimeDateStamp && !descriptor->OffsetModuleName) return;
    SIZE_T nameOffset = descriptor->OffsetModuleName;
    if (nameOffset >= directory->Size ||
        !memchr(data + nameOffset, 0, directory->Size - nameOffset)) return;
    BoundImportInfo info;
    MultiByteToWideChar(CP_OEMCP, 0, (const char*)(data + nameOffset), -1, info.szDllName, MAX_PATH);
    wsprintf(info.szTimeDateStamp, TEXT("%08X H"), descriptor->TimeDateStamp);
    wsprintf(info.szOffsetModuleName, TEXT("%04X H"), descriptor->OffsetModuleName);
    wsprintf(info.szNumberOfModuleForwarderRefs, TEXT("%04X H"), descriptor->NumberOfModuleForwarderRefs);
    drawBoundImportDirectory(hBoundImport, row++, info, colorWhite);
    SIZE_T step = ((SIZE_T)descriptor->NumberOfModuleForwarderRefs + 1) *
                 sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR);
    if (step > directory->Size - pos) return;
    pos += step;
  }
}

VOID drawBoundImportDirectory(HWND hDebug, int row, DebugInfo& debugInfo,
    COLORREF colorRef) {
  LV_ITEM vitem;
  //初始化
  memset(&vitem, 0, sizeof(LV_ITEM));
  vitem.mask = LVIF_TEXT;

  vitem.pszText = debugInfo.szCharacteristics;
  vitem.iItem = row;   //第几行
  vitem.iSubItem = 0;  //第几列
  SendMessage(hDebug, LVM_INSERTITEM, 0, (LPARAM)&vitem);
  SendMessage(hDebug, LVM_SETINSERTMARKCOLOR, 0, (DWORD)colorRef);

  vitem.pszText = debugInfo.szTimeDateStamp;
  vitem.iSubItem = 1;
  ListView_SetItem(hDebug, &vitem);

  vitem.pszText = debugInfo.szMajorVersion;
  vitem.iSubItem = 2;
  ListView_SetItem(hDebug, &vitem);

  vitem.pszText = debugInfo.szMinorVersion;
  vitem.iSubItem = 3;
  ListView_SetItem(hDebug, &vitem);

  vitem.pszText = debugInfo.szType;
  vitem.iSubItem = 4;
  ListView_SetItem(hDebug, &vitem);

  vitem.pszText = debugInfo.szSizeOfData;
  vitem.iSubItem = 5;
  ListView_SetItem(hDebug, &vitem);

  vitem.pszText = debugInfo.szAddressOfRawData;
  vitem.iSubItem = 6;
  ListView_SetItem(hDebug, &vitem);
  
  vitem.pszText = debugInfo.szPointerToRawData;
  vitem.iSubItem = 7;
  ListView_SetItem(hDebug, &vitem);
}

VOID readDebugDirectory(HWND hwnd, HWND hDebug, const PEContext& context) {
  const IMAGE_DATA_DIRECTORY* directory = context.Directory(IMAGE_DIRECTORY_ENTRY_DEBUG);
  if (!directory) return;
  const IMAGE_DEBUG_DIRECTORY* entries =
      (const IMAGE_DEBUG_DIRECTORY*)context.ReadRva(directory->VirtualAddress, directory->Size);
  if (!entries) return;
  SIZE_T count = directory->Size / sizeof(IMAGE_DEBUG_DIRECTORY);
  for (SIZE_T i = 0; i < count; ++i) {
    const IMAGE_DEBUG_DIRECTORY& entry = entries[i];
    DebugInfo info;
    wsprintf(info.szCharacteristics, TEXT("%08X"), entry.Characteristics);
    wsprintf(info.szTimeDateStamp, TEXT("%08X"), entry.TimeDateStamp);
    wsprintf(info.szMajorVersion, TEXT("%04X"), entry.MajorVersion);
    wsprintf(info.szMinorVersion, TEXT("%04X"), entry.MinorVersion);
    LPCTSTR typeName = entry.Type < 7 ? szDebugInfoType[entry.Type] :
                       entry.Type == IMAGE_DEBUG_TYPE_BORLAND ? TEXT("BORLAND") : TEXT("UNKNOWN");
    wsprintf(info.szType, TEXT("%s(%X)"), typeName, entry.Type);
    wsprintf(info.szSizeOfData, TEXT("%08X"), entry.SizeOfData);
    wsprintf(info.szAddressOfRawData, TEXT("%08X"), entry.AddressOfRawData);
    wsprintf(info.szPointerToRawData, TEXT("%08X"), entry.PointerToRawData);
    drawBoundImportDirectory(hDebug, (int)i, info, colorWhite);
  }
}
