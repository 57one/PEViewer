#include "readPE.h"

#include "global.h"

LPVOID pFileBuffer = NULL;
PIMAGE_DOS_HEADER pDosHeader = NULL;
PIMAGE_NT_HEADERS pNTHeader = NULL;
PIMAGE_FILE_HEADER pFileHeader = NULL;
PIMAGE_OPTIONAL_HEADER32 pOptionalHeader32 = NULL;
TCHAR buffer[MAX_PATH] = {0};
DWORD readPeFile(IN PTCHAR lpszFile, OUT LPVOID* pFileBuffer) {
  FILE* file = NULL;
  DWORD fileSize = 0;
  _tfopen_s(&file, lpszFile, TEXT("rb"));
  if (!file) {
#ifdef DEV
    _tprintf(TEXT("cannot open file"));
#endif  // DEV
    return NULL;
  }
  // get size of file
  fseek(file, 0, SEEK_END);
  fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);
  *pFileBuffer = malloc(fileSize);
  if (!pFileBuffer) {
#ifdef DEV
    _tprintf(TEXT("malloc failed"));
#endif  // DEV
    fclose(file);
    return NULL;
  }
  size_t n = fread(*pFileBuffer, fileSize, 1, file);
  if (!n) {
#ifdef DEV
    _tprintf(TEXT("read data failed"));
#endif  // DEV
    free(*pFileBuffer);
    fclose(file);
    return NULL;
  }
  fclose(file);
  return n;
}

VOID initPE() {
  pDosHeader = (PIMAGE_DOS_HEADER)pFileBuffer;
  pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)pFileBuffer + pDosHeader->e_lfanew);
  pFileHeader = (PIMAGE_FILE_HEADER)(((DWORD)pNTHeader) + 4);
  pOptionalHeader32 =
      (PIMAGE_OPTIONAL_HEADER32)((DWORD)pFileHeader + IMAGE_SIZEOF_FILE_HEADER);
}

VOID readDosHeader(HWND hwnd, LPVOID pFileBuffer) {
  memset(buffer, 0, sizeof(buffer));
  // set DosHeader Text
  writeToText(hwnd, IDC_EDIT_MAGIC_NUMBER, TEXT("%04X"), pDosHeader->e_magic);
  writeToText(hwnd, IDC_EDIT_BLP, TEXT("%04X"), pDosHeader->e_cblp);
  writeToText(hwnd, IDC_EDIT_PAGES, TEXT("%04X"), pDosHeader->e_cp);
  writeToText(hwnd, IDC_EDIT_RLC, TEXT("%04X"), pDosHeader->e_crlc);
  writeToText(hwnd, IDC_EDIT_PARHDR, TEXT("%04X"), pDosHeader->e_cparhdr);
  writeToText(hwnd, IDC_EDIT_MIN_ALLOC, TEXT("%04X"), pDosHeader->e_minalloc);
  writeToText(hwnd, IDC_EDIT_MAX_ALLOC, TEXT("%04X"), pDosHeader->e_maxalloc);
  writeToText(hwnd, IDC_EDIT_INIT_SS, TEXT("%04X"), pDosHeader->e_ss);
  writeToText(hwnd, IDC_EDIT_INIT_SP, TEXT("%04X"), pDosHeader->e_sp);
  writeToText(hwnd, IDC_EDIT_CHECKSUM, TEXT("%04X"), pDosHeader->e_csum);
  writeToText(hwnd, IDC_EDIT_INIT_IP, TEXT("%04X"), pDosHeader->e_ip);
  writeToText(hwnd, IDC_EDIT_INIT_CS, TEXT("%04X"), pDosHeader->e_cs);
  writeToText(hwnd, IDC_EDIT_LFARLC, TEXT("%04X"), pDosHeader->e_lfarlc);
  writeToText(hwnd, IDC_EDIT_OVNO, TEXT("%04X"), pDosHeader->e_ovno);
  writeToText(hwnd, IDC_EDIT_OEM_ID, TEXT("%04X"), pDosHeader->e_oemid);
  writeToText(hwnd, IDC_EDIT_OEM_INFO, TEXT("%04X"), pDosHeader->e_oeminfo);
  writeToText(hwnd, IDC_EDIT_PE_ADDRESS, TEXT("%08X"), pDosHeader->e_lfanew);
}

VOID readFileHeader(HWND hwnd, LPVOID pFileBuffer) {
  memset(buffer, 0, sizeof(buffer));
  // set FileHeader
  writeToText(hwnd, IDC_EDIT_MACHINE, TEXT("%04X"), pFileHeader->Machine);
  writeToText(hwnd, IDC_EDIT_NUMBER_OF_SECTIONS, TEXT("%04X"),
              pFileHeader->NumberOfSections);
  writeToText(hwnd, IDC_EDIT_TIMEDATESTAMP, TEXT("%08X"),
              pFileHeader->TimeDateStamp);
  writeToText(hwnd, IDC_EDIT_P_SYMTAB, TEXT("%08X"),
              pFileHeader->PointerToSymbolTable);
  writeToText(hwnd, IDC_EDIT_NUMBER_OF_SYMBOL, TEXT("%08X"),
              pFileHeader->NumberOfSymbols);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_OPT_HEADER, TEXT("%04X"),
              pFileHeader->SizeOfOptionalHeader);
  writeToText(hwnd, IDC_EDIT_CHARACTERISTICS, TEXT("%04X"),
              pFileHeader->Characteristics);
}

VOID readOptinalHeader32(HWND hwnd, LPVOID pFileBuffer) {
  // set OptinalHeader32
  writeToText(hwnd, IDC_EDIT_MAGIC, TEXT("%04X"), pOptionalHeader32->Magic);
  writeToText(hwnd, IDC_EDIT_MAJ_LINK_VER, TEXT("%02X"),
              pOptionalHeader32->MajorLinkerVersion);
  writeToText(hwnd, IDC_EDIT_MIN_LINK_VER, TEXT("%02X"),
              pOptionalHeader32->MinorLinkerVersion);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_CODE, TEXT("%08X"),
              pOptionalHeader32->SizeOfCode);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_INIT_DATA, TEXT("%08X"),
              pOptionalHeader32->SizeOfInitializedData);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_UNINIT_DATA, TEXT("%08X"),
              pOptionalHeader32->SizeOfUninitializedData);

  writeToText(hwnd, IDC_EDIT_ADDR_OF_ENTRY, TEXT("%08X"),
              pOptionalHeader32->AddressOfEntryPoint);
  writeToText(hwnd, IDC_EDIT_BASE_OF_CODE, TEXT("%08X"),
              pOptionalHeader32->BaseOfCode);
  writeToText(hwnd, IDC_EDIT_BASE_OF_DATA, TEXT("%08X"),
              pOptionalHeader32->BaseOfData);
  writeToText(hwnd, IDC_EDIT_IMAGE_BASE, TEXT("%08X"),
              pOptionalHeader32->ImageBase);
  writeToText(hwnd, IDC_EDIT_SECTION_ALIGNMENT, TEXT("%08X"),
              pOptionalHeader32->SectionAlignment);
  writeToText(hwnd, IDC_EDIT_FILE_ALIGNMENT, TEXT("%08X"),
              pOptionalHeader32->FileAlignment);
  writeToText(hwnd, IDC_EDIT_MAJ_OS_VER, TEXT("%04X"),
              pOptionalHeader32->MajorOperatingSystemVersion);
  writeToText(hwnd, IDC_EDIT_MIN_OS_VER, TEXT("%04X"),
              pOptionalHeader32->MinorOperatingSystemVersion);
  writeToText(hwnd, IDC_EDIT_MAJ_IMAGE_VER, TEXT("%04X"),
              pOptionalHeader32->MajorImageVersion);
  writeToText(hwnd, IDC_EDIT_MIN_IMAGE_VER, TEXT("%04X"),
              pOptionalHeader32->MinorImageVersion);
  writeToText(hwnd, IDC_EDIT_MAJ_SUBSTM_VER, TEXT("%04X"),
              pOptionalHeader32->MajorSubsystemVersion);
  writeToText(hwnd, IDC_EDIT_MIN_SUBSTM_VER, TEXT("%04X"),
              pOptionalHeader32->MinorSubsystemVersion);
  writeToText(hwnd, IDC_EDIT_WIN32_VER_VAL, TEXT("%08X"),
              pOptionalHeader32->Win32VersionValue);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_IMAGE, TEXT("%08X"),
              pOptionalHeader32->SizeOfImage);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_HEADERS, TEXT("%08X"),
              pOptionalHeader32->SizeOfHeaders);
  writeToText(hwnd, IDC_EDIT_CHECKSUM, TEXT("%08X"),
              pOptionalHeader32->CheckSum);
  writeToText(hwnd, IDC_EDIT_SUB_STM, TEXT("%04X"),
              pOptionalHeader32->Subsystem);
  writeToText(hwnd, IDC_EDIT_DLL_CHARAC, TEXT("%04X"),
              pOptionalHeader32->DllCharacteristics);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_STACK_REV, TEXT("%08X"),
              pOptionalHeader32->SizeOfStackReserve);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_STACK_COMMIT, TEXT("%08X"),
              pOptionalHeader32->SizeOfStackCommit);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_HEAP_REV, TEXT("%08X"),
              pOptionalHeader32->SizeOfHeapReserve);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_HEAP_COMMIT, TEXT("%08X"),
              pOptionalHeader32->SizeOfHeapCommit);
  writeToText(hwnd, IDC_EDIT_LOADER_FLAGS, TEXT("%08X"),
              pOptionalHeader32->LoaderFlags);
  writeToText(hwnd, IDC_EDIT_NUM_OF_RVA_SIZES, TEXT("%08X"),
              pOptionalHeader32->NumberOfRvaAndSizes);
}

VOID readHeaderInfo(HWND hwnd, LPVOID pFileBuffer) {
  // set HeaderInfo
  writeToText(hwnd, IDC_EDIT_HEADER_ADDR_OF_ENTRY, TEXT("%08X"),
              pOptionalHeader32->AddressOfEntryPoint);
  writeToText(hwnd, IDC_EDIT_HEADER_IMAGE_BASE, TEXT("%08X"),
              pOptionalHeader32->ImageBase);
  writeToText(hwnd, IDC_EDIT_HEADER_SIZE_OF_IMAGE, TEXT("%08X"),
              pOptionalHeader32->SizeOfImage);
  writeToText(hwnd, IDC_EDIT_HEADER_BASE_OF_CODE, TEXT("%08X"),
              pOptionalHeader32->BaseOfCode);
  writeToText(hwnd, IDC_EDIT_HEADER_BASE_OF_DATA, TEXT("%08X"),
              pOptionalHeader32->BaseOfData);
  writeToText(hwnd, IDC_EDIT_HEADER_SECTION_ALIGNMENT, TEXT("%08X"),
              pOptionalHeader32->SectionAlignment);
  writeToText(hwnd, IDC_EDIT_HEADER_FILE_ALIGNMENT, TEXT("%08X"),
              pOptionalHeader32->FileAlignment);
  writeToText(hwnd, IDC_EDIT_HEADER_MAGIC, TEXT("%04X"),
              pOptionalHeader32->Magic);
  writeToText(hwnd, IDC_EDIT_HEADER_SUB_STM, TEXT("%04X"),
              pOptionalHeader32->Subsystem);
  writeToText(hwnd, IDC_EDIT_HEADER_NUMBER_OF_SECTIONS, TEXT("%04X"),
              pFileHeader->NumberOfSections);
  writeToText(hwnd, IDC_EDIT_HEADER_TIMEDATESTAMP, TEXT("%08X"),
              pFileHeader->TimeDateStamp);
  writeToText(hwnd, IDC_EDIT_HEADER_SIZE_OF_HEADERS, TEXT("%08X"),
              pOptionalHeader32->SizeOfHeaders);
  writeToText(hwnd, IDC_EDIT_HEADER_CHARACTERISTICS, TEXT("%04X"),
              pFileHeader->Characteristics);
  writeToText(hwnd, IDC_EDIT_HEADER_CHECKSUM, TEXT("%08X"),
              pOptionalHeader32->CheckSum);
  writeToText(hwnd, IDC_EDIT_HEADER_SIZE_OF_OPT_HEADER, TEXT("%04X"),
              pFileHeader->SizeOfOptionalHeader);
  writeToText(hwnd, IDC_EDIT_HEADER_NUM_OF_RVA_SIZES, TEXT("%08X"),
              pOptionalHeader32->NumberOfRvaAndSizes);
}

VOID writeToText(HWND hwnd, INT TEXT_ID, CONST TCHAR* format, DWORD data) {
  _stprintf_s(buffer, format, data);
  SetWindowText(GetDlgItem(hwnd, TEXT_ID), buffer);
}