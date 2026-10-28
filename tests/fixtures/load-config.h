#pragma once

#include "pe64.h"

namespace load_config_fixture {
constexpr DWORD kRva = 0x1900;
constexpr DWORD kRaw = 0xB00;
constexpr DWORD kTableRva = 0x1C00;
constexpr DWORD kTableRaw = 0xE00;

template <typename Header>
inline void HeaderBytes(BYTE* image, ULONGLONG base) {
  Header header = {};
  header.Size = sizeof(header);
  header.TimeDateStamp = 0x12345678;
  header.MajorVersion = 1;
  header.MinorVersion = 2;
  header.GlobalFlagsClear = 0x100;
  header.GlobalFlagsSet = 0x200;
  header.ProcessHeapFlags = 0x400;
  header.ProcessAffinityMask = 0x1234;
  header.SecurityCookie = (decltype(header.SecurityCookie))(base + 0x1D00);
  header.GuardCFCheckFunctionPointer = (decltype(header.GuardCFCheckFunctionPointer))(base + 0x1D10);
  header.GuardCFFunctionTable = (decltype(header.GuardCFFunctionTable))(base + 0x1D20);
  header.GuardCFFunctionCount = 2;
  header.GuardFlags = IMAGE_GUARD_CF_INSTRUMENTED | IMAGE_GUARD_CF_FUNCTION_TABLE_PRESENT;
  header.CodeIntegrity.Flags = 3;
  header.CodeIntegrity.CatalogOffset = 0x123456;
  header.GuardMemcpyFunctionPointer = (decltype(header.GuardMemcpyFunctionPointer))(base + 0x1D30);
  if (sizeof(header.SecurityCookie) == 4) {
    header.SEHandlerTable = (decltype(header.SEHandlerTable))(base + kTableRva);
    header.SEHandlerCount = 3;
  } else {
    header.DeCommitFreeBlockThreshold = (decltype(header.DeCommitFreeBlockThreshold))0x200001234ULL;
    header.ProcessAffinityMask = (decltype(header.ProcessAffinityMask))0x100000002ULL;
  }
  std::memcpy(image + kRaw, &header, sizeof(header));
}

inline void Fill(BYTE* image, bool wide = false) {
  if (wide) pe64_fixture::Fill(image); else pe32_fixture::Fill(image);
  auto directories = wide ? ((PIMAGE_NT_HEADERS64)(image + 0x80))->OptionalHeader.DataDirectory :
                            ((PIMAGE_NT_HEADERS32)(image + 0x80))->OptionalHeader.DataDirectory;
  directories[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG] = {kRva,
      (DWORD)(wide ? sizeof(IMAGE_LOAD_CONFIG_DIRECTORY64) : sizeof(IMAGE_LOAD_CONFIG_DIRECTORY32))};
  if (wide) HeaderBytes<IMAGE_LOAD_CONFIG_DIRECTORY64>(image, pe64_fixture::kImageBase);
  else HeaderBytes<IMAGE_LOAD_CONFIG_DIRECTORY32>(image, pe32_fixture::kImageBase);
  const DWORD handlers[] = {0x1800, 0x1810, 0x1820};
  std::memcpy(image + kTableRaw, handlers, sizeof(handlers));
}
}  // namespace load_config_fixture
