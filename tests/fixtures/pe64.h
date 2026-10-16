#pragma once

#include "pe32.h"

namespace pe64_fixture {
constexpr ULONGLONG kImageBase = 0x0000000140000000ULL;

inline void Fill(BYTE* data) {
  pe32_fixture::Fill(data);
  auto nt32 = (PIMAGE_NT_HEADERS32)(data + 0x80);
  IMAGE_SECTION_HEADER section = *IMAGE_FIRST_SECTION(nt32);
  IMAGE_DATA_DIRECTORY directories[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
  std::memcpy(directories, nt32->OptionalHeader.DataDirectory, sizeof(directories));
  std::memset(data + 0x80, 0, sizeof(IMAGE_NT_HEADERS64) + sizeof(section));
  auto nt = (PIMAGE_NT_HEADERS64)(data + 0x80);
  nt->Signature = IMAGE_NT_SIGNATURE;
  nt->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
  nt->FileHeader.NumberOfSections = 1;
  nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
  nt->FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE;
  nt->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
  nt->OptionalHeader.ImageBase = kImageBase;
  nt->OptionalHeader.SectionAlignment = 0x1000;
  nt->OptionalHeader.FileAlignment = 0x200;
  nt->OptionalHeader.SizeOfHeaders = pe32_fixture::kSectionRaw;
  nt->OptionalHeader.SizeOfImage = 0x2000;
  nt->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
  std::memcpy(nt->OptionalHeader.DataDirectory, directories, sizeof(directories));
  *IMAGE_FIRST_SECTION(nt) = section;
}
}  // namespace pe64_fixture
