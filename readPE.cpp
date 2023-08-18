#include "readPE.h"

#include "global.h"

LPVOID pFileBuffer = NULL;
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

VOID readDosHeader(HWND hwnd, LPVOID pFileBuffer) {
  memset(buffer, 0, sizeof(buffer));
  PIMAGE_DOS_HEADER pDosHeader = NULL;
  pDosHeader = (PIMAGE_DOS_HEADER)pFileBuffer;
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
  PIMAGE_DOS_HEADER pDosHeader = NULL;
  PIMAGE_NT_HEADERS pNTHeader = NULL;
  PIMAGE_FILE_HEADER pFileHeader = NULL;
  pDosHeader = (PIMAGE_DOS_HEADER)pFileBuffer;
  pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)pFileBuffer + pDosHeader->e_lfanew);
  pFileHeader = (PIMAGE_FILE_HEADER)(((DWORD)pNTHeader) + 4);
  // set FileHeader
  writeToText(hwnd, IDC_EDIT_MACHINE, TEXT("%04X"), pFileHeader->Machine);
  writeToText(hwnd, IDC_EDIT_NUMBER_OF_SECTIONS, TEXT("%04X"), pFileHeader->NumberOfSections);
  writeToText(hwnd, IDC_EDIT_TIMEDATESTAMP, TEXT("%08X"), pFileHeader->TimeDateStamp);
  writeToText(hwnd, IDC_EDIT_P_SYMTAB, TEXT("%08X"), pFileHeader->PointerToSymbolTable);
  writeToText(hwnd, IDC_EDIT_NUMBER_OF_SYMBOL, TEXT("%08X"), pFileHeader->NumberOfSymbols);
  writeToText(hwnd, IDC_EDIT_SIZE_OF_OPT_HEADER, TEXT("%04X"), pFileHeader->SizeOfOptionalHeader);
  writeToText(hwnd, IDC_EDIT_CHARACTERISTICS, TEXT("%04X"), pFileHeader->Characteristics);
}

VOID writeToText(HWND hwnd, INT TEXT_ID, CONST TCHAR* format, WORD data) {
  _stprintf_s(buffer, format, data);
  SetWindowText(GetDlgItem(hwnd, TEXT_ID), buffer);
}