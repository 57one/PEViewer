#pragma once

#include "pe64.h"

namespace resource_fixture {
constexpr DWORD kRva = 0x1900;
constexpr DWORD kRaw = 0xB00;
constexpr DWORD kSize = 0x200;

inline void Directory(BYTE* base, DWORD offset, WORD named, WORD ids) {
  IMAGE_RESOURCE_DIRECTORY header = {};
  header.NumberOfNamedEntries = named;
  header.NumberOfIdEntries = ids;
  std::memcpy(base + offset, &header, sizeof(header));
}

inline void Entry(BYTE* base, DWORD offset, DWORD name, DWORD target) {
  std::memcpy(base + offset, &name, sizeof(name));
  std::memcpy(base + offset + sizeof(name), &target, sizeof(target));
}

inline void Name(BYTE* base, DWORD offset, const wchar_t* name, WORD length) {
  std::memcpy(base + offset, &length, sizeof(length));
  std::memcpy(base + offset + sizeof(length), name, (SIZE_T)length * sizeof(WCHAR));
}

inline void Fill(BYTE* image, bool wide) {
  if (wide) pe64_fixture::Fill(image); else pe32_fixture::Fill(image);
  auto directories = wide ? ((PIMAGE_NT_HEADERS64)(image + 0x80))->OptionalHeader.DataDirectory :
                            ((PIMAGE_NT_HEADERS32)(image + 0x80))->OptionalHeader.DataDirectory;
  directories[IMAGE_DIRECTORY_ENTRY_RESOURCE] = {kRva, kSize};
  BYTE* base = image + kRaw;
  Directory(base, 0, 1, 1);
  Entry(base, 0x10, 0x80000180, 0x80000030);
  Entry(base, 0x18, 10, 0x80000090);
  Directory(base, 0x30, 1, 0);
  Entry(base, 0x40, 0x800001A0, 0x80000060);
  Directory(base, 0x60, 0, 2);
  Entry(base, 0x70, 0x409, 0xE0);
  Entry(base, 0x78, 0x804, 0xF0);
  Directory(base, 0x90, 0, 1);
  Entry(base, 0xA0, 101, 0x800000B0);
  Directory(base, 0xB0, 0, 1);
  Entry(base, 0xC0, 0x409, 0x100);
  Name(base, 0x180, L"\u81EA\u5B9A\u4E49\u7C7B\u578B", 5);
  Name(base, 0x1A0, L"\u914D\u7F6E", 2);
  const IMAGE_RESOURCE_DATA_ENTRY data[] = {{0x1D00, 4, 1200, 0}, {0x1D10, 3, 65001, 0}, {0x1D20, 2, 0, 0}};
  std::memcpy(base + 0xE0, data, sizeof(data));
  std::memcpy(image + 0xF00, "AB\0D", 4);
  std::memcpy(image + 0xF10, "xyz", 3);
  std::memcpy(image + 0xF20, "12", 2);
}
}  // namespace resource_fixture
