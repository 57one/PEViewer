#pragma once

#include "pe64.h"

namespace tls_fixture {
constexpr DWORD kRva = 0x1900;
constexpr DWORD kRaw = 0xB00;
constexpr DWORD kCallbacksRva = 0x1980;
constexpr DWORD kCallbacksRaw = 0xB80;
constexpr DWORD kIndexRva = 0x19C0;
constexpr DWORD kDataRva = 0x1A00;

inline void WriteValue(BYTE* data, ULONGLONG value, bool wide) {
  std::memcpy(data, &value, wide ? 8 : 4);
}

template <typename Header>
inline void HeaderValues(Header& header, ULONGLONG base) {
  // Explicit narrowing here matches the disk header selected by the fixture.
  header.StartAddressOfRawData = (decltype(header.StartAddressOfRawData))(base + kDataRva);
  header.EndAddressOfRawData = (decltype(header.EndAddressOfRawData))(base + kDataRva + 16);
  header.AddressOfIndex = (decltype(header.AddressOfIndex))(base + kIndexRva);
  header.AddressOfCallBacks = (decltype(header.AddressOfCallBacks))(base + kCallbacksRva);
  header.SizeOfZeroFill = 32;
  header.Characteristics = IMAGE_SCN_ALIGN_16BYTES;
}

inline void Fill(BYTE* image, bool wide) {
  if (wide) pe64_fixture::Fill(image); else pe32_fixture::Fill(image);
  ULONGLONG base = wide ? pe64_fixture::kImageBase : pe32_fixture::kImageBase;
  if (wide) {
    auto nt = (PIMAGE_NT_HEADERS64)(image + 0x80);
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS] = {kRva, sizeof(IMAGE_TLS_DIRECTORY64)};
    IMAGE_TLS_DIRECTORY64 header = {};
    HeaderValues(header, base);
    std::memcpy(image + kRaw, &header, sizeof(header));
  } else {
    auto nt = (PIMAGE_NT_HEADERS32)(image + 0x80);
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS] = {kRva, sizeof(IMAGE_TLS_DIRECTORY32)};
    IMAGE_TLS_DIRECTORY32 header = {};
    HeaderValues(header, base);
    std::memcpy(image + kRaw, &header, sizeof(header));
  }
  // The array and template are intentionally outside the TLS directory's Size.
  std::memset(image + kCallbacksRaw, 0, 24);
  WriteValue(image + kCallbacksRaw, base + 0x1800, wide);
  WriteValue(image + kCallbacksRaw + (wide ? 8 : 4), base + 0x1810, wide);
  std::memset(image + kDataRva - 0xE00, 0x5A, 16);
}
}  // namespace tls_fixture
