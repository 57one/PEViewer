#include "pe/PEContext.h"
#include "fixtures/pe32.h"
#include "fixtures/pe64.h"

#include <cstdio>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <tchar.h>
#include <vector>

namespace {
void Check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

std::vector<BYTE> Image() {
  std::vector<BYTE> image(pe32_fixture::kFileSize);
  pe32_fixture::Fill(image.data());
  return image;
}

PIMAGE_NT_HEADERS32 Headers(std::vector<BYTE>& image) {
  return (PIMAGE_NT_HEADERS32)(image.data() + 0x80);
}

void InvalidImage(const std::vector<BYTE>& image) {
  PEContext context;
  auto valid = Image();
  Check(context.Initialize(valid.data(), valid.size()), "Initial context setup failed");
  Check(!context.Initialize(image.data(), image.size()), "Invalid headers were accepted");
  Check(!context.IsValid() && !context.fileBuffer && !context.fileSize &&
        !context.dosHeader && !context.ntSignature && !context.fileHeader &&
        !context.optionalHeader32 && !context.optionalHeader64 &&
        !context.sectionHeaders && !context.sectionCount &&
        !context.dataDirectories && !context.directoryCount &&
        context.errorMessage && *context.errorMessage,
        "Failed initialization left stale context state");
}
}  // namespace

void CheckPEContext(LPCTSTR filePath) {
  auto image = Image();
  std::vector<BYTE> image64(pe32_fixture::kFileSize);
  pe64_fixture::Fill(image64.data());
  PEContext pe32;
  PEContext pe64;
  Check(pe32.Initialize(image.data(), image.size()), "PE32 initialization failed");
  Check(pe64.Initialize(image64.data(), image64.size()), "PE32+ initialization failed");
  Check(pe32.format == PEFormat::PE32 && pe32.optionalHeader32 && !pe32.optionalHeader64,
        "PE32 format selection failed");
  Check(pe64.format == PEFormat::PE32Plus && !pe64.optionalHeader32 && pe64.optionalHeader64 &&
        pe64.optionalHeader64->ImageBase == pe64_fixture::kImageBase,
        "PE32+ format selection or 64-bit field preservation failed");
  Check(pe32.fileSize == image.size() && pe32.sectionCount == 1 &&
        pe32.directoryCount == IMAGE_NUMBEROF_DIRECTORY_ENTRIES,
        "Context metadata differs from the file");
  pe64.Reset();
  Check(pe32.IsValid() && pe32.optionalHeader32->ImageBase == pe32_fixture::kImageBase,
        "Resetting another context changed the active file");

  SIZE_T offset = 123;
  Check(pe32.RvaToFileOffset(0, 1, offset) && offset == 0,
        "A valid zero file offset was mistaken for failure");
  Check(pe32.RvaToFileOffset(pe32_fixture::kThunkRva, sizeof(DWORD), offset) &&
        offset == pe32_fixture::kThunkRaw, "Section RVA mapping failed");
  Check(!pe32.RvaToFileOffset(pe32.sizeOfHeaders - 1, 2, offset) && offset == 0,
        "A read crossed the header mapping boundary");
  Check(!pe32.RvaToFileOffset(0x800, 1, offset), "An unmapped RVA was accepted");
  Check(!pe32.ContainsRange((SIZE_T)-1, 2) && !pe32.ContainsRange(1, (SIZE_T)-1),
        "Range checks overflowed");
  Check(pe32.ReadString(0x1240) && std::strcmp(pe32.ReadString(0x1240), "test.dll") == 0,
        "Bounded RVA string lookup failed");
  Check(pe32.Directory(IMAGE_DIRECTORY_ENTRY_IMPORT) &&
        !pe32.Directory(IMAGE_DIRECTORY_ENTRY_RESOURCE) && !pe32.Directory(16),
        "Directory lookup did not respect valid entries");

  auto section = IMAGE_FIRST_SECTION(Headers(image));
  section->Misc.VirtualSize = 0x2000;
  Check(!pe32.RvaToFileOffset(0x2000, 1, offset), "Virtual zero-fill was treated as file data");
  section->Misc.VirtualSize = 0x100;
  Check(pe32.RvaToFileOffset(pe32_fixture::kThunkRva, 4, offset),
        "Raw alignment bytes were rejected");
  section->VirtualAddress = 0xFFFFFFF0;
  Check(!pe32.RvaToFileOffset(0xFFFFFFF8, 16, offset), "An RVA read wrapped past 32 bits");
  pe32_fixture::Fill(image.data());
  std::memset(image.data() + image.size() - 4, 'X', 4);
  Check(!pe32.ReadString(0x1FFC), "An unterminated string was accepted");
  pe32.Reset();
  pe32.Reset();
  Check(image.front() == 'M', "Reset freed or modified a borrowed buffer");

  PEContext loaded;
  Check(loaded.LoadFile(filePath) && loaded.fileSize == pe32_fixture::kFileSize,
        "File loading did not populate the context");
  Check(loaded.LoadFile(filePath), "Reloading the current document failed");
  Check(!loaded.LoadFile(TEXT("")) && !loaded.fileBuffer && !loaded.IsValid(),
        "A failed load retained the previous file");
  Check(!loaded.LoadFile(NULL), "A null file path was accepted");
  Check(!loaded.Initialize(NULL, 0), "An empty memory view was accepted");
  std::basic_string<TCHAR> invalidPath(filePath);
  invalidPath += TEXT(".invalid");
  FILE* invalidFile = NULL;
  Check(_tfopen_s(&invalidFile, invalidPath.c_str(), TEXT("wb")) == 0,
        "Cannot create the invalid file fixture");
  BYTE invalidBytes[4] = {};
  SIZE_T written = std::fwrite(invalidBytes, 1, sizeof(invalidBytes), invalidFile);
  int closeResult = std::fclose(invalidFile);
  Check(written == sizeof(invalidBytes) && closeResult == 0, "Cannot write the invalid file fixture");
  Check(!loaded.LoadFile(invalidPath.c_str()) && !loaded.fileBuffer && !loaded.fileSize &&
        !loaded.IsValid(), "A failed file parse retained its owned allocation");

  InvalidImage({});
  InvalidImage(std::vector<BYTE>(sizeof(IMAGE_DOS_HEADER) - 1));
  auto invalid = Image();
  ((PIMAGE_DOS_HEADER)invalid.data())->e_magic = 0;
  InvalidImage(invalid);
  invalid = Image();
  ((PIMAGE_DOS_HEADER)invalid.data())->e_lfanew = -1;
  InvalidImage(invalid);
  invalid = Image();
  ((PIMAGE_DOS_HEADER)invalid.data())->e_lfanew = MAXLONG;
  InvalidImage(invalid);
  invalid = Image();
  Headers(invalid)->Signature = 0;
  InvalidImage(invalid);
  invalid = Image();
  Headers(invalid)->FileHeader.SizeOfOptionalHeader = 1;
  InvalidImage(invalid);
  invalid = Image();
  Headers(invalid)->OptionalHeader.Magic = 0x107;
  InvalidImage(invalid);
  invalid = Image();
  Headers(invalid)->OptionalHeader.NumberOfRvaAndSizes = 17;
  InvalidImage(invalid);
  invalid = Image();
  Headers(invalid)->FileHeader.NumberOfSections = MAXWORD;
  InvalidImage(invalid);
  invalid = Image();
  Headers(invalid)->OptionalHeader.SizeOfHeaders = 1;
  InvalidImage(invalid);
  invalid = Image();
  Headers(invalid)->OptionalHeader.SizeOfHeaders = (DWORD)invalid.size() + 1;
  InvalidImage(invalid);

  image = Image();
  auto nt = Headers(image);
  IMAGE_SECTION_HEADER savedSection = *IMAGE_FIRST_SECTION(nt);
  nt->FileHeader.SizeOfOptionalHeader = (WORD)offsetof(IMAGE_OPTIONAL_HEADER32, DataDirectory);
  nt->OptionalHeader.NumberOfRvaAndSizes = 0;
  *IMAGE_FIRST_SECTION(nt) = savedSection;
  Check(pe32.Initialize(image.data(), image.size()) && pe32.directoryCount == 0 &&
        !pe32.dataDirectories && !pe32.Directory(0), "An optional header without directories was rejected");
  image = Image();
  nt = Headers(image);
  savedSection = *IMAGE_FIRST_SECTION(nt);
  nt->FileHeader.SizeOfOptionalHeader += sizeof(IMAGE_DATA_DIRECTORY);
  nt->OptionalHeader.NumberOfRvaAndSizes = 17;
  *IMAGE_FIRST_SECTION(nt) = savedSection;
  Check(pe32.Initialize(image.data(), image.size()) &&
        pe32.directoryCount == IMAGE_NUMBEROF_DIRECTORY_ENTRIES && !pe32.Directory(16),
        "Extended directory entries exceeded the supported directory count");
  std::puts("PASS: PEContext formats, ownership, header validation, bounded RVA and strings");
}
