#pragma once

#include "pe64.h"

namespace exception_fixture {
constexpr DWORD kRva = 0x1900;
constexpr DWORD kRaw = 0xB00;
constexpr DWORD kRecordSize = 12;
constexpr DWORD kSize = kRecordSize * 3;

inline void Record(BYTE* image, SIZE_T index, DWORD begin, DWORD end, DWORD unwind) {
  const DWORD fields[] = {begin, end, unwind};
  std::memcpy(image + kRaw + index * kRecordSize, fields, sizeof(fields));
}

inline void Fill(BYTE* image, bool wide = true) {
  if (wide) pe64_fixture::Fill(image); else pe32_fixture::Fill(image);
  auto directories = wide ? ((PIMAGE_NT_HEADERS64)(image + 0x80))->OptionalHeader.DataDirectory :
                            ((PIMAGE_NT_HEADERS32)(image + 0x80))->OptionalHeader.DataDirectory;
  directories[IMAGE_DIRECTORY_ENTRY_EXCEPTION] = {kRva, kSize};
  Record(image, 0, 0x1800, 0x1810, 0x1A00);
  Record(image, 1, 0x1810, 0x1830, 0x1A04);
  Record(image, 2, 0x1840, 0x1860, 0x1A08);
  const DWORD header = 1;  // Minimal v1 UNWIND_INFO with no unwind codes.
  for (SIZE_T i = 0; i < 3; ++i) std::memcpy(image + 0xC00 + i * 4, &header, sizeof(header));
}
}  // namespace exception_fixture
