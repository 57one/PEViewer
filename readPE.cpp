#include "readPE.h"

#include "global.h"

LPVOID pFileBuffer = NULL;
PIMAGE_DOS_HEADER pDosHeader = NULL;                // dos头
PIMAGE_NT_HEADERS pNTHeader = NULL;                 // NT头
PIMAGE_FILE_HEADER pFileHeader = NULL;              // PE头
PIMAGE_OPTIONAL_HEADER32 pOptionalHeader32 = NULL;  // 32位 可选PE头
PIMAGE_SECTION_HEADER pSectionHeader = NULL;        // 第一个节区头
DWORD sizeOfDataDirectory = 0;
PIMAGE_DATA_DIRECTORY pDataDirectory = NULL;
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

DWORD rvaToFileOffset(IN LPVOID pFileBuffer, IN DWORD dwRva) {
  //节区数
  int sectionNums = pFileHeader->NumberOfSections;

  //判断rva是否在 headers+节表处
  if (dwRva < pOptionalHeader32->SizeOfHeaders) {
    //不需要进行处理 没有拉伸 在文件和内存中都是一样的
    return dwRva;
  }
  //计算相对偏移 dwRva 就是相对偏移了 va - pImageBuffer
  //遍历节区表进行比较
  for (int i = 0; i < sectionNums; i++) {
    //此时name是正常的 没有占八位 可以直接输出
    // NAME
    // printf("%s\n",&(pSectionHeader->Name));
    if (dwRva >= (DWORD)(pSectionHeader->VirtualAddress) &&
        dwRva < (DWORD)(pSectionHeader->VirtualAddress) +
                    (DWORD)(pSectionHeader->Misc.VirtualSize))
      break;
    pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pSectionHeader +
                                             IMAGE_SIZEOF_SECTION_HEADER);
  }
  //计算 RAW
  DWORD RAW =
      dwRva - pSectionHeader->VirtualAddress + pSectionHeader->PointerToRawData;
  return RAW;
}

VOID initPE() {
  pDosHeader = (PIMAGE_DOS_HEADER)pFileBuffer;
  pNTHeader = (PIMAGE_NT_HEADERS)((DWORD)pFileBuffer + pDosHeader->e_lfanew);
  pFileHeader = (PIMAGE_FILE_HEADER)(((DWORD)pNTHeader) + 4);
  pOptionalHeader32 =
      (PIMAGE_OPTIONAL_HEADER32)((DWORD)pFileHeader + IMAGE_SIZEOF_FILE_HEADER);
  int sizeOfOptionalHeader = pFileHeader->SizeOfOptionalHeader;
  pSectionHeader =
      (PIMAGE_SECTION_HEADER)((DWORD)pOptionalHeader32 + sizeOfOptionalHeader);
  sizeOfDataDirectory = pOptionalHeader32->NumberOfRvaAndSizes;
  pDataDirectory = pOptionalHeader32->DataDirectory;
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

VOID drawSections(HWND hListSection, int Row, SectionInfo& sectionInfo,
                  COLORREF colorRef) {
  LV_ITEM vitem;
  //初始化
  memset(&vitem, 0, sizeof(LV_ITEM));
  vitem.mask = LVIF_TEXT;

  vitem.pszText = sectionInfo.szIndex;
  vitem.iItem = Row;   //第几行
  vitem.iSubItem = 0;  //第几列
  SendMessage(hListSection, LVM_INSERTITEM, 0, (DWORD)&vitem);
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

VOID readSections(HWND hwnd, HWND hListSection) {
  // number of sections
  int sectionNums = pFileHeader->NumberOfSections;
  SectionInfo sectionInfo;
  TCHAR szSectionName[MAX_PATH] = TEXT("<unknown>");
  COLORREF colorRef;
  colorRef = colorWhite;
  // colorRef = (count % 2 == 1) ? colorGrey : colorWhite;
  for (int i = 0; i < sectionNums; i++) {
    wsprintf(sectionInfo.szIndex, TEXT("%d"), i);
    MultiByteToWideChar(CP_OEMCP, 0, (char*)(pSectionHeader->Name), -1,
                        szSectionName, MAX_PATH + 1);
    wsprintf(sectionInfo.szSectionName, TEXT("%s"), szSectionName);
    wsprintf(sectionInfo.szVirtualSize, TEXT("%08X"), pSectionHeader->Misc);
    wsprintf(sectionInfo.szVirtualOffset, TEXT("%08X"),
             pSectionHeader->VirtualAddress);
    wsprintf(sectionInfo.szRawSize, TEXT("%08X"),
             pSectionHeader->SizeOfRawData);
    wsprintf(sectionInfo.szRawOffset, TEXT("%08X"),
             pSectionHeader->PointerToRawData);
    wsprintf(sectionInfo.szCharacteristics, TEXT("%08X"),
             pSectionHeader->Characteristics);
    drawSections(hListSection, i, sectionInfo, colorRef);
    pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pSectionHeader +
                                             IMAGE_SIZEOF_SECTION_HEADER);
  }
  // restore pSectionHeader
  int sizeOfOptionalHeader = pFileHeader->SizeOfOptionalHeader;
  pSectionHeader =
      (PIMAGE_SECTION_HEADER)((DWORD)pOptionalHeader32 + sizeOfOptionalHeader);
}

VOID readDirectory(HWND hwnd) {
  // set Directory
  for (int i = 0; i < sizeOfDataDirectory; i++) {
    // printf("%x\n%x\n",pOptionalHeader->DataDirectory[i].VirtualAddress,pOptionalHeader->DataDirectory[i].VirtualAddress);
    writeToText(hwnd, directoryRVAEditID[i], TEXT("%08X"),
                pDataDirectory[i].VirtualAddress);
    writeToText(hwnd, directorySizeEditID[i], TEXT("%08X"),
                pDataDirectory[i].Size);
    if (!pDataDirectory[i].VirtualAddress && !pDataDirectory[i].Size) {
      EnableWindow(GetDlgItem(hwnd, directoryMoreInfoButtonID[i]), FALSE);
      EnableWindow(GetDlgItem(hwnd, directoryHexButtonID[i]), FALSE);
    }
  }
}

VOID readSectionEdit(HWND hwnd, INT ID) {
  // number of sections
  int sectionNums = pFileHeader->NumberOfSections;
  SectionInfo sectionInfo;
  TCHAR szSectionName[MAX_PATH] = TEXT("<unknown>");
  COLORREF colorRef;
  colorRef = colorWhite;
  // colorRef = (count % 2 == 1) ? colorGrey : colorWhite;
  pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)pSectionHeader +
                                           IMAGE_SIZEOF_SECTION_HEADER * ID);

  MultiByteToWideChar(CP_OEMCP, 0, (char*)(pSectionHeader->Name), -1,
                      szSectionName, MAX_PATH + 1);
  SetWindowText(GetDlgItem(hwnd, IDC_EDIT_SECTION_NAME), szSectionName);
  writeToText(hwnd, IDC_EDIT_SECTION_VIRTUAL_SIZE, TEXT("%08X"),
              (pSectionHeader->Misc.VirtualSize));
  writeToText(hwnd, IDC_EDIT_SECTION_VIRTUAL_SIZE, TEXT("%08X"),
              pSectionHeader->Misc.VirtualSize);
  writeToText(hwnd, IDC_EDIT_SECTION_VIRTUAL_OFFSET, TEXT("%08X"),
              pSectionHeader->VirtualAddress);
  writeToText(hwnd, IDC_EDIT_SECTION_RAW_SIZE, TEXT("%08X"),
              pSectionHeader->SizeOfRawData);
  writeToText(hwnd, IDC_EDIT_SECTION_RAW_OffSET, TEXT("%08X"),
              pSectionHeader->PointerToRawData);
  writeToText(hwnd, IDC_EDIT_SECTION_CHARACTERISTICS, TEXT("%08X"),
              pSectionHeader->Characteristics);

  // restore pSectionHeader
  int sizeOfOptionalHeader = pFileHeader->SizeOfOptionalHeader;
  pSectionHeader =
      (PIMAGE_SECTION_HEADER)((DWORD)pOptionalHeader32 + sizeOfOptionalHeader);
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
  SendMessage(hListImportDLL, LVM_INSERTITEM, 0, (DWORD)&vitem);
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

VOID readImportDirectory(HWND hwnd, HWND hListImportDLL) {
  PIMAGE_DATA_DIRECTORY pImportDataDirectory = pDataDirectory + 1;
  PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = NULL;
  ImportDescInfo importDescInfo;
  TCHAR szDllName[MAX_PATH] = TEXT("<unknown>");
  DWORD row = 0;
  COLORREF colorRef = colorWhite;
  // 定位导入表
  // 导入表描述符 IMAGE_IMPORT_DESCRIPTOR RVA->RAW
  pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)rvaToFileOffset(
      pFileBuffer, pImportDataDirectory->VirtualAddress);
  pImportDescriptor =
      (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)pFileBuffer + (DWORD)pImportDescriptor);
  while (TRUE) {
    if (pImportDescriptor->OriginalFirstThunk == 0 &&
        pImportDescriptor->FirstThunk == 0) {
      break;
    }

    wsprintf(importDescInfo.szOriginalFirstThunk, TEXT("%08X"),
             pImportDescriptor->OriginalFirstThunk);
    wsprintf(importDescInfo.szTimeDateStamp, TEXT("%08X"),
             pImportDescriptor->TimeDateStamp);
    wsprintf(importDescInfo.szForwarderChain, TEXT("%08X"),
             pImportDescriptor->ForwarderChain);
    wsprintf(importDescInfo.szName, TEXT("%08X"), pImportDescriptor->Name);
    wsprintf(importDescInfo.szFirstThunk, TEXT("%08X"),
             pImportDescriptor->FirstThunk);
    DWORD addressOfName = NULL;
    // dll name RVA->RAW
    addressOfName = rvaToFileOffset(pFileBuffer, pImportDescriptor->Name);
    addressOfName = (DWORD)pFileBuffer + addressOfName;
    MultiByteToWideChar(CP_OEMCP, 0, (char*)(addressOfName), -1, szDllName,
                        MAX_PATH + 1);
    wsprintf(importDescInfo.szDllName, TEXT("%s"), szDllName);
    drawDllImportDirectory(hListImportDLL, row, importDescInfo, colorRef);

    pImportDescriptor += 1;
    row += 1;
  }

  // here would be a bug and i dont know why
  /*HWND hListIntThunk = GetDlgItem(hwnd, IDC_LIST_INT_THUNK);
  readIntThunk(hwnd, hListIntThunk, pImportDescriptor - row);*/

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
  SendMessage(hListIntThunk, LVM_INSERTITEM, 0, (DWORD)&vitem);
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

VOID readIntThunk(HWND hwnd, HWND hListIntThunk,
                  PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor) {
  DWORD originalFirstThunk = NULL;
  DWORD thunkData = 0;
  IntThunkInfo intThunkInfo;
  TCHAR szAPIName[MAX_PATH] = TEXT("<unknown>");
  DWORD row = 0;
  COLORREF colorRef = colorWhite;
  originalFirstThunk =
      rvaToFileOffset(pFileBuffer, pImportDescriptor->OriginalFirstThunk);
  originalFirstThunk = (DWORD)pFileBuffer + originalFirstThunk;
  while (TRUE) {
    if (((PIMAGE_THUNK_DATA)originalFirstThunk)->u1.Ordinal == 0x00000000)
      break;
    //最高位为1 表示存放的是函数的导出序号
    if (((PIMAGE_THUNK_DATA)originalFirstThunk)->u1.Ordinal &
        0x80000000 == 0x80000000) {
      thunkData = *((DWORD*)originalFirstThunk) & 0x7FFFFFFF;
    } else {
      thunkData =
          ((PIMAGE_THUNK_DATA)originalFirstThunk)->u1.Ordinal & 0x7FFFFFFF;
      // RVA->PRAW
      thunkData = rvaToFileOffset(pFileBuffer, thunkData);
      thunkData = (DWORD)pFileBuffer + thunkData;

      wsprintf(intThunkInfo.szThunkRVA, TEXT("%010X"),
               pImportDescriptor->OriginalFirstThunk + 4 * row);
      wsprintf(intThunkInfo.szThunkRAW, TEXT("%08X"), originalFirstThunk);
      wsprintf(intThunkInfo.szThunkValue, TEXT("%08X"),
               ((PIMAGE_THUNK_DATA)originalFirstThunk)->u1.Ordinal);
      wsprintf(intThunkInfo.szHint, TEXT("%04X"),
               ((PIMAGE_IMPORT_BY_NAME)thunkData)->Hint);
      MultiByteToWideChar(CP_OEMCP, 0,
                          (char*)(((PIMAGE_IMPORT_BY_NAME)thunkData)->Name), -1,
                          szAPIName, MAX_PATH + 1);
      wsprintf(intThunkInfo.szAPIName, TEXT("%s"), szAPIName);
      drawIntThunk(hListIntThunk, row, intThunkInfo, colorRef);
    }
    originalFirstThunk += 4;
    row += 1;
  }
}

VOID writeToText(HWND hwnd, INT TEXT_ID, CONST TCHAR* format, DWORD data) {
  _stprintf_s(buffer, format, data);
  SetWindowText(GetDlgItem(hwnd, TEXT_ID), buffer);
}
