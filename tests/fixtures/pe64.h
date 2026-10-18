#pragma once

#include "pe32.h"

namespace pe64_fixture {
constexpr ULONGLONG kImageBase = 0x0000000140000000ULL;
constexpr ULONGLONG kStackReserve = 0x0000000200000000ULL;
constexpr ULONGLONG kStackCommit = 0x0000000100001000ULL;
constexpr ULONGLONG kHeapReserve = 0x0000000300000000ULL;
constexpr ULONGLONG kHeapCommit = 0x0000000100002000ULL;
constexpr DWORD kThunkRva = 0x1280;
constexpr DWORD kThunkRaw = 0x480;
constexpr DWORD kIatRva = 0x12A0;
constexpr DWORD kIatRaw = 0x4A0;

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
  nt->OptionalHeader.MajorLinkerVersion = 14;
  nt->OptionalHeader.AddressOfEntryPoint = 0x1800;
  nt->OptionalHeader.BaseOfCode = pe32_fixture::kSectionRva;
  nt->OptionalHeader.ImageBase = kImageBase;
  nt->OptionalHeader.SectionAlignment = 0x1000;
  nt->OptionalHeader.FileAlignment = 0x200;
  nt->OptionalHeader.SizeOfHeaders = pe32_fixture::kSectionRaw;
  nt->OptionalHeader.SizeOfImage = 0x2000;
  nt->OptionalHeader.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
  nt->OptionalHeader.CheckSum = 0xABCD1234;
  nt->OptionalHeader.SizeOfStackReserve = kStackReserve;
  nt->OptionalHeader.SizeOfStackCommit = kStackCommit;
  nt->OptionalHeader.SizeOfHeapReserve = kHeapReserve;
  nt->OptionalHeader.SizeOfHeapCommit = kHeapCommit;
  nt->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
  std::memcpy(nt->OptionalHeader.DataDirectory, directories, sizeof(directories));
  nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT] = {kIatRva, 32};
  *IMAGE_FIRST_SECTION(nt) = section;

  // Use real 8-byte lookup/IAT entries: name, ordinal, name, terminator.
  auto imports = (PIMAGE_IMPORT_DESCRIPTOR)(data + 0x400);
  imports->OriginalFirstThunk = kThunkRva;
  imports->FirstThunk = kIatRva;
  std::memset(data + 0x450, 0, 0x20);
  auto thunks = (PIMAGE_THUNK_DATA64)(data + kThunkRaw);
  thunks[0].u1.AddressOfData = 0x1270;
  thunks[1].u1.Ordinal = IMAGE_ORDINAL_FLAG64 | 42;
  thunks[2].u1.AddressOfData = 0x12C0;
  std::memcpy(data + kIatRaw, thunks, sizeof(IMAGE_THUNK_DATA64) * 4);
  auto secondName = (PIMAGE_IMPORT_BY_NAME)(data + 0x4C0);
  secondName->Hint = 0x5678;
  std::memcpy(secondName->Name, "SecondFunc", 11);
  *(WORD*)(data + pe32_fixture::kRelocRaw + 8) = (IMAGE_REL_BASED_DIR64 << 12) | 0x800;
  *(ULONGLONG*)(data + 0xA00) = kImageBase + 0x1800;
}
}  // namespace pe64_fixture
