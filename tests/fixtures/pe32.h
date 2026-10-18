#pragma once

#include <windows.h>
#include <cstring>

namespace pe32_fixture {
constexpr SIZE_T kFileSize = 0x1200;
constexpr DWORD kImageBase = 0x00400000;
constexpr DWORD kSectionRva = 0x1000;
constexpr DWORD kSectionRaw = 0x200;
constexpr DWORD kImportRva = 0x1200;
constexpr DWORD kThunkRva = 0x1250;
constexpr DWORD kThunkRaw = 0x450;
constexpr DWORD kIatRva = 0x1260;
constexpr DWORD kIatRaw = 0x460;
constexpr DWORD kRelocRva = 0x1500;
constexpr DWORD kRelocRaw = 0x700;
constexpr DWORD kExportRva = 0x1100;
constexpr DWORD kBoundImportRva = 0x1300;
constexpr DWORD kDebugRva = 0x1400;
constexpr WORD kImportHint = 0x1234;
constexpr wchar_t kSectionName[] = L".test";
constexpr wchar_t kImportDll[] = L"test.dll";
constexpr wchar_t kImportName[] = L"ImportedFunc";
constexpr wchar_t kExportName[] = L"ExportedFunc";
constexpr wchar_t kBoundDll[] = L"bound.dll";

// A deterministic disk-layout PE32 fixture, independent of compiler output.
inline void Fill(BYTE* data) {
  std::memset(data, 0, kFileSize);
  auto dos = (PIMAGE_DOS_HEADER)data;
  dos->e_magic = IMAGE_DOS_SIGNATURE;
  dos->e_lfanew = 0x80;
  auto nt = (PIMAGE_NT_HEADERS32)(data + dos->e_lfanew);
  nt->Signature = IMAGE_NT_SIGNATURE;
  nt->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
  nt->FileHeader.NumberOfSections = 1;
  nt->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);
  nt->FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE;
  auto& optional = nt->OptionalHeader;
  optional.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
  optional.ImageBase = kImageBase;
  optional.AddressOfEntryPoint = 0x1800;
  optional.BaseOfCode = kSectionRva;
  optional.SectionAlignment = 0x1000;
  optional.FileAlignment = 0x200;
  optional.SizeOfHeaders = kSectionRaw;
  optional.SizeOfImage = 0x2000;
  optional.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
  optional.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
  optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] = {kExportRva, 0x80};
  optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] = {kImportRva, 40};
  optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT] = {kBoundImportRva, 32};
  optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG] = {kDebugRva, sizeof(IMAGE_DEBUG_DIRECTORY)};
  optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT] = {kIatRva, 8};
  optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC] = {kRelocRva, 24};
  auto section = (PIMAGE_SECTION_HEADER)(data + 0x80 + sizeof(IMAGE_NT_HEADERS32));
  std::memcpy(section->Name, ".test", 6);
  section->VirtualAddress = kSectionRva;
  section->Misc.VirtualSize = 0x1000;
  section->PointerToRawData = kSectionRaw;
  section->SizeOfRawData = 0x1000;
  section->Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ;

  auto exports = (PIMAGE_EXPORT_DIRECTORY)(data + 0x300);
  exports->Base = 1;
  exports->NumberOfFunctions = 1;
  exports->NumberOfNames = 1;
  exports->Name = 0x1180;
  exports->AddressOfFunctions = 0x1140;
  exports->AddressOfNames = 0x1144;
  exports->AddressOfNameOrdinals = 0x1148;
  *(DWORD*)(data + 0x340) = 0x1800;
  *(DWORD*)(data + 0x344) = 0x1170;
  std::memcpy(data + 0x370, "ExportedFunc", 13);
  std::memcpy(data + 0x380, "test.dll", 9);

  auto imports = (PIMAGE_IMPORT_DESCRIPTOR)(data + 0x400);
  imports->OriginalFirstThunk = kThunkRva;
  imports->Name = 0x1240;
  imports->FirstThunk = 0x1260;
  std::memcpy(data + 0x440, "test.dll", 9);
  *(DWORD*)(data + kThunkRaw) = 0x1270;
  *(DWORD*)(data + 0x460) = 0x1270;
  auto importName = (PIMAGE_IMPORT_BY_NAME)(data + 0x470);
  importName->Hint = kImportHint;
  std::memcpy(importName->Name, "ImportedFunc", 13);

  auto bound = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)(data + 0x500);
  bound->TimeDateStamp = 0x12345678;
  bound->OffsetModuleName = 16;
  std::memcpy(data + 0x510, "bound.dll", 10);

  auto debug = (PIMAGE_DEBUG_DIRECTORY)(data + 0x600);
  debug->TimeDateStamp = 0x11223344;
  debug->Type = IMAGE_DEBUG_TYPE_CODEVIEW;
  debug->SizeOfData = 16;
  debug->AddressOfRawData = 0x1450;
  debug->PointerToRawData = 0x650;

  // Two relocation blocks: one real HIGHLOW target and one padding-only block.
  auto reloc = (PIMAGE_BASE_RELOCATION)(data + kRelocRaw);
  reloc->VirtualAddress = 0x1000;
  reloc->SizeOfBlock = 12;
  *(WORD*)(data + kRelocRaw + 8) = (IMAGE_REL_BASED_HIGHLOW << 12) | 0x800;
  *(DWORD*)(data + 0xA00) = kImageBase + 0x1800;
  reloc = (PIMAGE_BASE_RELOCATION)(data + kRelocRaw + 12);
  reloc->VirtualAddress = 0;
  reloc->SizeOfBlock = 12;
}
}  // namespace pe32_fixture
