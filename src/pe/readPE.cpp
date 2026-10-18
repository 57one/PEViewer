#include "readPE.h"

#include "common/global.h"


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
