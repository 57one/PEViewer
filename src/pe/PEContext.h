#pragma once

#include <windows.h>

enum class PEFormat { Unknown, PE32, PE32Plus };

// Header pointers are views into fileBuffer and remain valid until Reset().
// LoadFile owns its allocation; Initialize borrows the caller's buffer.
struct PEContext {
  const BYTE* fileBuffer = NULL;
  SIZE_T fileSize = 0;
  PEFormat format = PEFormat::Unknown;
  const IMAGE_DOS_HEADER* dosHeader = NULL;
  const DWORD* ntSignature = NULL;
  const IMAGE_FILE_HEADER* fileHeader = NULL;
  const IMAGE_OPTIONAL_HEADER32* optionalHeader32 = NULL;
  const IMAGE_OPTIONAL_HEADER64* optionalHeader64 = NULL;
  const IMAGE_SECTION_HEADER* sectionHeaders = NULL;
  WORD sectionCount = 0;
  const IMAGE_DATA_DIRECTORY* dataDirectories = NULL;
  DWORD directoryCount = 0;
  DWORD sizeOfHeaders = 0;
  LPCTSTR errorMessage = TEXT("");

  PEContext() = default;
  ~PEContext();
  PEContext(const PEContext&) = delete;
  PEContext& operator=(const PEContext&) = delete;

  BOOL LoadFile(LPCTSTR path);
  BOOL Initialize(const void* buffer, SIZE_T size);
  VOID Reset();
  BOOL IsValid() const;
  BOOL ContainsRange(SIZE_T offset, SIZE_T size) const;
  BOOL ContainsPointer(const void* pointer, SIZE_T size) const;
  BOOL RvaToFileOffset(DWORD rva, SIZE_T size, SIZE_T& offset) const;
  const BYTE* ReadRva(DWORD rva, SIZE_T size) const;
  const char* ReadString(DWORD rva) const;
  const IMAGE_DATA_DIRECTORY* Directory(DWORD index) const;

 private:
  BYTE* ownedBuffer = NULL;
  BOOL ParseHeaders();
  BOOL Fail(LPCTSTR message);
  BOOL RvaRange(DWORD rva, SIZE_T& offset, SIZE_T& available) const;
};
