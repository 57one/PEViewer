#include "PEContext.h"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <tchar.h>

PEContext::~PEContext() { Reset(); }

VOID PEContext::Reset() {
  std::free(ownedBuffer);
  ownedBuffer = NULL;
  fileBuffer = NULL;
  fileSize = 0;
  format = PEFormat::Unknown;
  dosHeader = NULL;
  ntSignature = NULL;
  fileHeader = NULL;
  optionalHeader32 = NULL;
  optionalHeader64 = NULL;
  sectionHeaders = NULL;
  sectionCount = 0;
  dataDirectories = NULL;
  directoryCount = 0;
  sizeOfHeaders = 0;
  errorMessage = TEXT("");
}

BOOL PEContext::Fail(LPCTSTR message) {
  Reset();
  errorMessage = message;
  return FALSE;
}

BOOL PEContext::ContainsRange(SIZE_T offset, SIZE_T size) const {
  return fileBuffer && offset <= fileSize && size <= fileSize - offset;
}

BOOL PEContext::ContainsPointer(const void* pointer, SIZE_T size) const {
  DWORD_PTR address = (DWORD_PTR)pointer;
  DWORD_PTR base = (DWORD_PTR)fileBuffer;
  return fileBuffer && address >= base && ContainsRange(address - base, size);
}

BOOL PEContext::IsValid() const { return format != PEFormat::Unknown; }

BOOL PEContext::LoadFile(LPCTSTR path) {
  Reset();
  if (!path || !*path) return Fail(TEXT("No PE file was specified."));
  FILE* file = NULL;
  if (_tfopen_s(&file, path, TEXT("rb")) != 0 || !file)
    return Fail(TEXT("Cannot open the PE file."));

  BOOL seekSucceeded = _fseeki64(file, 0, SEEK_END) == 0;
  __int64 length = seekSucceeded ? _ftelli64(file) : -1;
  if (length <= 0 || (ULONGLONG)length > (ULONGLONG)(SIZE_T)-1 ||
      _fseeki64(file, 0, SEEK_SET) != 0) {
    std::fclose(file);
    return Fail(TEXT("The file is empty or too large to read in this viewer."));
  }
  fileSize = (SIZE_T)length;
  ownedBuffer = (BYTE*)std::malloc(fileSize);
  if (!ownedBuffer) {
    std::fclose(file);
    return Fail(TEXT("Cannot allocate the PE file buffer."));
  }
  fileBuffer = ownedBuffer;
  SIZE_T bytesRead = std::fread(ownedBuffer, 1, fileSize, file);
  int closeResult = std::fclose(file);
  if (bytesRead != fileSize || closeResult != 0)
    return Fail(TEXT("Cannot read the complete PE file."));
  return ParseHeaders();
}

BOOL PEContext::Initialize(const void* buffer, SIZE_T size) {
  // Borrowing this context's own allocation would leave a dangling view.
  if (ownedBuffer && ContainsPointer(buffer, 0))
    return Fail(TEXT("Cannot borrow a buffer owned by the same PE context."));
  Reset();
  fileBuffer = (const BYTE*)buffer;
  fileSize = size;
  return ParseHeaders();
}

BOOL PEContext::ParseHeaders() {
  if (!ContainsRange(0, sizeof(IMAGE_DOS_HEADER)))
    return Fail(TEXT("The DOS header is truncated."));
  dosHeader = (const IMAGE_DOS_HEADER*)fileBuffer;
  if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE || dosHeader->e_lfanew < 0)
    return Fail(TEXT("Invalid DOS signature or NT header offset."));

  SIZE_T ntOffset = (SIZE_T)dosHeader->e_lfanew;
  if (!ContainsRange(ntOffset, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER)))
    return Fail(TEXT("The NT signature or file header is truncated."));
  ntSignature = (const DWORD*)(fileBuffer + ntOffset);
  if (*ntSignature != IMAGE_NT_SIGNATURE)
    return Fail(TEXT("Invalid PE signature."));
  fileHeader = (const IMAGE_FILE_HEADER*)(fileBuffer + ntOffset + sizeof(DWORD));
  SIZE_T optionalOffset = ntOffset + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);
  SIZE_T optionalSize = fileHeader->SizeOfOptionalHeader;
  if (optionalSize < sizeof(WORD) || !ContainsRange(optionalOffset, optionalSize))
    return Fail(TEXT("The optional header is truncated."));

  WORD magic = 0;
  std::memcpy(&magic, fileBuffer + optionalOffset, sizeof(magic));
  SIZE_T directoryOffset = 0;
  DWORD declaredDirectories = 0;
  if (magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    directoryOffset = offsetof(IMAGE_OPTIONAL_HEADER32, DataDirectory);
    if (optionalSize < directoryOffset)
      return Fail(TEXT("The PE32 optional header is truncated."));
    optionalHeader32 = (const IMAGE_OPTIONAL_HEADER32*)(fileBuffer + optionalOffset);
    declaredDirectories = optionalHeader32->NumberOfRvaAndSizes;
    sizeOfHeaders = optionalHeader32->SizeOfHeaders;
    format = PEFormat::PE32;
  } else if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    directoryOffset = offsetof(IMAGE_OPTIONAL_HEADER64, DataDirectory);
    if (optionalSize < directoryOffset)
      return Fail(TEXT("The PE32+ optional header is truncated."));
    optionalHeader64 = (const IMAGE_OPTIONAL_HEADER64*)(fileBuffer + optionalOffset);
    declaredDirectories = optionalHeader64->NumberOfRvaAndSizes;
    sizeOfHeaders = optionalHeader64->SizeOfHeaders;
    format = PEFormat::PE32Plus;
  } else {
    return Fail(TEXT("Unsupported optional header format."));
  }

  SIZE_T directoryCapacity = (optionalSize - directoryOffset) / sizeof(IMAGE_DATA_DIRECTORY);
  if (declaredDirectories > directoryCapacity)
    return Fail(TEXT("The data directories exceed the optional header."));
  directoryCount = declaredDirectories < IMAGE_NUMBEROF_DIRECTORY_ENTRIES
                       ? declaredDirectories : IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
  if (directoryCount)
    dataDirectories = (const IMAGE_DATA_DIRECTORY*)(fileBuffer + optionalOffset + directoryOffset);

  SIZE_T sectionOffset = optionalOffset + optionalSize;
  sectionCount = fileHeader->NumberOfSections;
  if (sectionOffset > fileSize ||
      sectionCount > (fileSize - sectionOffset) / sizeof(IMAGE_SECTION_HEADER))
    return Fail(TEXT("The section table is truncated."));
  SIZE_T sectionEnd = sectionOffset + sectionCount * sizeof(IMAGE_SECTION_HEADER);
  if (sizeOfHeaders < sectionEnd || sizeOfHeaders > fileSize)
    return Fail(TEXT("SizeOfHeaders does not cover the file headers."));
  sectionHeaders = (const IMAGE_SECTION_HEADER*)(fileBuffer + sectionOffset);
  return TRUE;
}

BOOL PEContext::RvaRange(DWORD rva, SIZE_T& offset, SIZE_T& available) const {
  offset = 0;
  available = 0;
  if (!IsValid()) return FALSE;
  if (rva < sizeOfHeaders) {
    offset = rva;
    available = sizeOfHeaders - rva;
    return ContainsRange(offset, available);
  }
  for (WORD i = 0; i < sectionCount; ++i) {
    const IMAGE_SECTION_HEADER& section = sectionHeaders[i];
    if (rva < section.VirtualAddress) continue;
    DWORD delta = rva - section.VirtualAddress;
    DWORD span = section.Misc.VirtualSize > section.SizeOfRawData
                     ? section.Misc.VirtualSize : section.SizeOfRawData;
    if (delta >= span) continue;
    if (delta >= section.SizeOfRawData || !section.PointerToRawData) return FALSE;
    ULONGLONG raw = (ULONGLONG)section.PointerToRawData + delta;
    if (raw >= fileSize) return FALSE;
    offset = (SIZE_T)raw;
    SIZE_T rawRemaining = section.SizeOfRawData - delta;
    available = rawRemaining < fileSize - offset ? rawRemaining : fileSize - offset;
    ULONGLONG rvaRemaining = (ULONGLONG)MAXDWORD + 1 - rva;
    if (available > rvaRemaining) available = (SIZE_T)rvaRemaining;
    return TRUE;
  }
  return FALSE;
}

BOOL PEContext::RvaToFileOffset(DWORD rva, SIZE_T size, SIZE_T& offset) const {
  SIZE_T available = 0;
  SIZE_T result = 0;
  offset = 0;
  if (!RvaRange(rva, result, available) || size > available) return FALSE;
  offset = result;
  return TRUE;
}

const BYTE* PEContext::ReadRva(DWORD rva, SIZE_T size) const {
  SIZE_T offset = 0;
  return RvaToFileOffset(rva, size, offset) ? fileBuffer + offset : NULL;
}

const char* PEContext::ReadString(DWORD rva, SIZE_T maxBytes) const {
  SIZE_T offset = 0;
  SIZE_T available = 0;
  if (!RvaRange(rva, offset, available)) return NULL;
  if (available > maxBytes) available = maxBytes;
  if (!std::memchr(fileBuffer + offset, 0, available)) return NULL;
  return (const char*)(fileBuffer + offset);
}

const IMAGE_DATA_DIRECTORY* PEContext::Directory(DWORD index) const {
  if (!IsValid() || index >= directoryCount) return NULL;
  const IMAGE_DATA_DIRECTORY* entry = dataDirectories + index;
  return entry->VirtualAddress && entry->Size ? entry : NULL;
}
