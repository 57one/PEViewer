#pragma once

#include "pe64.h"

namespace delay_fixture {
constexpr DWORD kRva = 0x1900;
constexpr DWORD kRaw = 0xB00;
constexpr DWORD kLookupRva = 0x1B00;
constexpr DWORD kIatRva = 0x1B40;
constexpr DWORD kBoundRva = 0x1B80;
constexpr DWORD kUnloadRva = 0x1BC0;
constexpr DWORD kOtherLookupRva = 0x1D00;
constexpr DWORD kOtherIatRva = 0x1D40;

inline DWORD Raw(DWORD rva) { return rva - 0xE00; }

inline void Value(BYTE* image, DWORD rva, ULONGLONG value, bool wide) {
  std::memcpy(image + Raw(rva), &value, wide ? 8 : 4);
}

inline void Fill(BYTE* image, bool wide, bool legacy = false) {
  if (wide) pe64_fixture::Fill(image); else pe32_fixture::Fill(image);
  ULONGLONG base = wide ? pe64_fixture::kImageBase : pe32_fixture::kImageBase;
  if (legacy && wide) base = 0x10000000;
  if (wide) {
    auto nt = (PIMAGE_NT_HEADERS64)(image + 0x80);
    nt->OptionalHeader.ImageBase = base;
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT] = {kRva, 96};
  } else {
    auto nt = (PIMAGE_NT_HEADERS32)(image + 0x80);
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT] = {kRva, 96};
  }
  std::memset(image + kRaw, 0, 0x500);
  auto address = [&](DWORD rva) { return (DWORD)(legacy ? base + rva : rva); };
  IMAGE_DELAYLOAD_DESCRIPTOR descriptors[3] = {};
  auto& first = descriptors[0];
  first.Attributes.AllAttributes = legacy ? 0 : 1;
  first.DllNameRVA = address(0x1A00);
  first.ModuleHandleRVA = address(0x1A40);
  first.ImportAddressTableRVA = address(kIatRva);
  first.ImportNameTableRVA = address(kLookupRva);
  first.BoundImportAddressTableRVA = address(kBoundRva);
  first.UnloadInformationTableRVA = address(kUnloadRva);
  first.TimeDateStamp = 0x12345678;
  auto& second = descriptors[1];
  second.Attributes.AllAttributes = legacy ? 0 : 1;
  second.DllNameRVA = address(0x1A20);
  second.ModuleHandleRVA = address(0x1A50);
  second.ImportAddressTableRVA = address(kOtherIatRva);
  second.ImportNameTableRVA = address(kOtherLookupRva);
  std::memcpy(image + kRaw, descriptors, sizeof(descriptors));
  std::memcpy(image + Raw(0x1A00), "delay-test.dll", sizeof("delay-test.dll"));
  std::memcpy(image + Raw(0x1A20), "other-delay.dll", sizeof("other-delay.dll"));
  WORD hint = 0x1234;
  std::memcpy(image + Raw(0x1C00), &hint, sizeof(hint));
  std::memcpy(image + Raw(0x1C02), "DelayedFunc", sizeof("DelayedFunc"));
  hint = 0x5678;
  std::memcpy(image + Raw(0x1C40), &hint, sizeof(hint));
  std::memcpy(image + Raw(0x1C42), "SecondDelayFunc", sizeof("SecondDelayFunc"));
  SIZE_T width = wide ? 8 : 4;
  ULONGLONG flag = wide ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32;
  Value(image, kLookupRva, legacy ? base + 0x1C00 : 0x1C00, wide);
  Value(image, kLookupRva + (DWORD)width, flag | 42, wide);
  Value(image, kLookupRva + (DWORD)width * 2, legacy ? base + 0x1C40 : 0x1C40, wide);
  for (DWORD i = 0; i < 3; ++i) {
    Value(image, kIatRva + i * (DWORD)width, base + 0x1800 + i * 0x10, wide);
    Value(image, kUnloadRva + i * (DWORD)width, base + 0x1800 + i * 0x10, wide);
    Value(image, kBoundRva + i * (DWORD)width, (wide ? 0x00007FFB12345678ULL : 0x77001000ULL) + i, wide);
  }
  Value(image, kOtherLookupRva, flag | 7, wide);
  Value(image, kOtherIatRva, base + 0x1830, wide);
}
}  // namespace delay_fixture
