#include "pe/directoryDetails.h"
#include "fixtures/pe32.h"
#include "fixtures/pe64.h"

#include <cstring>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}

void Store(BYTE* target, ULONGLONG value, SIZE_T width) {
  std::memcpy(target, &value, width);
}

void CheckFormat(bool wide) {
  std::vector<BYTE> image(pe32_fixture::kFileSize);
  PEContext context;
  auto reset = [&]() {
    if (wide) pe64_fixture::Fill(image.data());
    else pe32_fixture::Fill(image.data());
    Check(context.Initialize(image.data(), image.size()), "Directory details fixture rejected");
  };
  reset();
  auto directories = const_cast<IMAGE_DATA_DIRECTORY*>(context.dataDirectories);
  auto descriptor = (PIMAGE_IMPORT_DESCRIPTOR)(image.data() + 0x400);
  SIZE_T width = wide ? 8 : 4;
  DWORD iatRva = wide ? pe64_fixture::kIatRva : pe32_fixture::kIatRva;
  DWORD iatRaw = wide ? pe64_fixture::kIatRaw : pe32_fixture::kIatRaw;
  DWORD intRva = wide ? pe64_fixture::kThunkRva : pe32_fixture::kThunkRva;
  DWORD intRaw = wide ? pe64_fixture::kThunkRaw : pe32_fixture::kThunkRaw;
  SIZE_T slots = wide ? 4 : 2;
  auto table = ReadIatDetails(context);
  Check(table.message.empty() && table.entries.size() == slots, "IAT slot count / status mismatch");
  const auto& first = table.entries[0];
  Check(first.rva == iatRva && first.fileOffset == iatRaw && first.value == 0x1270,
        "IAT slot address, file offset or raw value mismatch");
  Check(first.dll == "test.dll" && first.symbol.name == "ImportedFunc" && first.hasLookup &&
        first.lookupRva == intRva && first.lookupValue == 0x1270, "INT association mismatch");
  Check(table.entries.back().value == 0 && table.entries.back().note == L"Terminator",
        "IAT terminator was omitted");
  if (wide) {
    Check((table.entries[1].symbol.kind == ImportKind::Ordinal && table.entries[1].symbol.ordinal == 42) &&
          table.entries[1].rva == iatRva + 8 && table.entries[2].symbol.name == "SecondFunc",
          "PE32+ IAT uses incorrect stride or ordinal flag");
  }

  // Bound IAT values are addresses, while the unchanged INT still identifies the import.
  ULONGLONG address = wide ? 0x00007FF812345678ULL : 0x76543210ULL;
  Store(image.data() + iatRaw, address, width);
  descriptor->TimeDateStamp = 0x12345678;
  table = ReadIatDetails(context);
  Check(table.entries[0].value == address && table.entries[0].symbol.name == "ImportedFunc" &&
        table.entries[0].lookupValue == 0x1270, "Bound IAT address was treated as a name RVA");
  descriptor->OriginalFirstThunk = 0;
  table = ReadIatDetails(context);
  Check(!table.entries[0].hasLookup && table.entries[0].symbol.name.empty() &&
        table.entries[0].dll == "test.dll" && !table.entries[0].note.empty(),
        "Bound IAT without INT invented a function name");

  reset();
  descriptor->OriginalFirstThunk = 0;
  table = ReadIatDetails(context);
  Check(table.entries[0].symbol.name == "ImportedFunc" && table.entries[0].lookupRva == iatRva,
        "Unbound FirstThunk fallback failed");
  reset();
  directories[IMAGE_DIRECTORY_ENTRY_IMPORT] = {};
  table = ReadIatDetails(context);
  Check(table.entries.size() == slots && table.entries[0].dll.empty(),
        "IAT raw slots depend on import descriptors");
  directories[IMAGE_DIRECTORY_ENTRY_IAT].Size += 1;
  table = ReadIatDetails(context);
  Check(table.entries.size() == slots && !table.message.empty(), "Truncated IAT slot was not reported");
  directories[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = MAXDWORD;
  table = ReadIatDetails(context);
  Check(table.entries.empty() && !table.message.empty(), "Invalid IAT RVA was read");
  directories[IMAGE_DIRECTORY_ENTRY_IAT] = {};
  Check(ReadIatDetails(context).entries.empty(), "Missing IAT should be empty");

  reset();
  descriptor->OriginalFirstThunk = 0x1FFF;
  table = ReadIatDetails(context);
  Check(table.entries[0].symbol.name.empty() && !table.entries[0].hasLookup &&
        !table.entries[0].note.empty(), "Truncated INT slot was read");
  reset();
  Store(image.data() + intRaw, wide ? 0x0000000100001270ULL : 0x70001270ULL, width);
  table = ReadIatDetails(context);
  Check(table.entries[0].symbol.kind == ImportKind::Invalid && !table.entries[0].note.empty(),
        "Invalid/bound lookup value was narrowed to a valid name RVA");
  Store(image.data() + intRaw, wide ? 0x800000000001002AULL : 0x8001002AULL, width);
  table = ReadIatDetails(context);
  Check(table.entries[0].symbol.kind == ImportKind::Invalid && !table.entries[0].note.empty(),
        "Reserved ordinal bits were accepted");
  Store(image.data() + intRaw, 0x1FFF, width);
  table = ReadIatDetails(context);
  Check(table.entries[0].symbol.kind == ImportKind::Invalid && !table.entries[0].note.empty(),
        "Invalid import-by-name record was accepted");
  reset();
  Store(image.data() + intRaw, 0x1900, width);
  std::memset(image.data() + 0xB02, 'X', image.size() - 0xB02);
  table = ReadIatDetails(context);
  Check(table.entries[0].symbol.kind == ImportKind::Invalid && !table.entries[0].note.empty(),
        "Unterminated import name was accepted");

  // Two DLLs and two terminators in the same IAT range.
  reset();
  directories[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = sizeof(IMAGE_IMPORT_DESCRIPTOR) * 3;
  descriptor[1] = descriptor[0];
  std::memset(&descriptor[2], 0, sizeof(descriptor[2]));
  descriptor[0].FirstThunk = 0x1600;
  descriptor[1].FirstThunk = 0x1600 + (DWORD)(width * 2);
  descriptor[0].OriginalFirstThunk = 0x1700;
  descriptor[1].OriginalFirstThunk = 0x1740;
  descriptor[1].Name = 0x1750;
  std::memcpy(image.data() + 0x950, "other.dll", 10);
  ULONGLONG ordinal = (wide ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32) | 65535;
  Store(image.data() + 0x800, 0x1270, width);
  Store(image.data() + 0x800 + width * 2, ordinal, width);
  Store(image.data() + 0x900, 0x1270, width);
  Store(image.data() + 0x940, ordinal, width);
  directories[IMAGE_DIRECTORY_ENTRY_IAT] = {0x1600, (DWORD)(width * 4)};
  table = ReadIatDetails(context);
  Check(table.message.empty() && table.entries.size() == 4 &&
        table.entries[0].dll == "test.dll" && table.entries[2].dll == "other.dll" &&
        (table.entries[2].symbol.kind == ImportKind::Ordinal && table.entries[2].symbol.ordinal == 65535), "Multi-DLL IAT mapping failed");
  directories[IMAGE_DIRECTORY_ENTRY_IAT].Size = (DWORD)(width * 3);
  Check(!ReadIatDetails(context).message.empty(), "Missing IAT terminator was not reported");

  reset();
  auto relocs = ReadRelocationDetails(context);
  Check(relocs.message.empty() && relocs.blocks.size() == 2, "Relocation blocks not parsed");
  const auto& block = relocs.blocks[0];
  Check(block.rva == 0x1500 && block.fileOffset == 0x700 && block.pageRva == 0x1000 &&
        block.size == 12 && block.entries.size() == 2, "Relocation block header mismatch");
  const auto& entry = block.entries[0];
  Check(entry.type == (wide ? IMAGE_REL_BASED_DIR64 : IMAGE_REL_BASED_HIGHLOW) &&
        entry.offset == 0x800 && entry.targetRva == 0x1800 && entry.fileOffset == 0xA00 &&
        entry.hasValue && entry.value == (wide ? pe64_fixture::kImageBase : pe32_fixture::kImageBase) + 0x1800,
        "Relocation target or original value mismatch");
  Check(!block.entries[1].hasTarget && !block.entries[1].hasValue,
        "ABSOLUTE padding was interpreted as a relocation");

  auto header = (PIMAGE_BASE_RELOCATION)(image.data() + pe32_fixture::kRelocRaw);
  auto words = (WORD*)(image.data() + pe32_fixture::kRelocRaw + sizeof(*header));
  words[0] = (IMAGE_REL_BASED_HIGHADJ << 12) | 0x800;
  words[1] = 0xFABC;
  relocs = ReadRelocationDetails(context);
  Check(relocs.message.empty() && relocs.blocks[0].entries.size() == 1 &&
        relocs.blocks[0].entries[0].hasAdjustment &&
        (WORD)relocs.blocks[0].entries[0].adjustment == 0xFABC,
        "HIGHADJ adjustment was treated as another relocation");
  header->SizeOfBlock = 10;
  directories[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = 10;
  Check(!ReadRelocationDetails(context).message.empty(), "Truncated HIGHADJ pair was accepted");

  const DWORD invalidSizes[] = {0, 7, 9, 25, MAXDWORD};
  for (DWORD size : invalidSizes) {
    reset();
    header->SizeOfBlock = size;
    relocs = ReadRelocationDetails(context);
    Check(relocs.blocks.empty() && !relocs.message.empty(), "Invalid SizeOfBlock was accepted");
  }
  reset();
  directories[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = 7;
  Check(!ReadRelocationDetails(context).message.empty(), "Truncated relocation header was accepted");
  reset();
  header->VirtualAddress = 0x1001;
  Check(!ReadRelocationDetails(context).message.empty(), "Unaligned Page RVA was accepted");
  reset();
  directories[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = MAXDWORD;
  Check(ReadRelocationDetails(context).blocks.empty(), "Invalid relocation directory was read");
  reset();
  words[0] = (IMAGE_REL_BASED_DIR64 << 12) | 0xFFF;
  relocs = ReadRelocationDetails(context);
  Check(relocs.blocks.size() == 2 && !relocs.blocks[0].entries[0].hasValue &&
        !relocs.blocks[0].entries[0].note.empty(), "Truncated relocation target was read");
  words[0] = (6 << 12) | 0x800;
  relocs = ReadRelocationDetails(context);
  Check(!relocs.blocks[0].entries[0].hasValue && relocs.blocks[0].entries[0].type == 6,
        "Unknown relocation type was decoded as a pointer");
}
void CheckSeparatedDirectories(bool wide) {
  std::vector<BYTE> image(pe32_fixture::kFileSize);
  PEContext context;
  auto reset = [&]() {
    if (wide) pe64_fixture::Fill(image.data()); else pe32_fixture::Fill(image.data());
    Check(context.Initialize(image.data(), image.size()), "Separated-directory fixture rejected");
  };
  reset();
  auto directories = const_cast<IMAGE_DATA_DIRECTORY*>(context.dataDirectories);
  auto descriptor = (PIMAGE_IMPORT_DESCRIPTOR)(image.data() + 0x400);
  auto imports = ReadImportDetails(context);
  Check(imports.present && imports.terminated && imports.message.empty() && imports.modules.size() == 1 &&
        imports.modules[0].name == "test.dll", "Import descriptor model mismatch");
  auto thunks = ReadImportThunks(context, imports.modules[0].descriptor);
  Check(thunks.message.empty() && thunks.terminated && thunks.entries.size() == (wide ? 3 : 1) &&
        thunks.entries[0].symbol.kind == ImportKind::Name && thunks.entries[0].symbol.hint == 0x1234 &&
        thunks.entries[0].symbol.name == "ImportedFunc", "Import model lost its name or Hint");
  auto iat = ReadIatDetails(context);
  Check(iat.entries[0].symbol.hint == thunks.entries[0].symbol.hint &&
        iat.entries[0].symbol.name == thunks.entries[0].symbol.name,
        "IAT and import page use different symbol decoding");
  if (wide) Check(thunks.entries[1].symbol.kind == ImportKind::Ordinal &&
                  thunks.entries[1].symbol.ordinal == 42 && thunks.entries[2].symbol.hint == 0x5678,
                  "64-bit import model lost ordinal or second Hint");
  directories[IMAGE_DIRECTORY_ENTRY_IAT] = {};
  imports = ReadImportDetails(context);
  thunks = ReadImportThunks(context, imports.modules[0].descriptor);
  Check(thunks.terminated && thunks.entries[0].symbol.hint == 0x1234 &&
        thunks.entries.size() == (wide ? 3 : 1), "Import parsing depends on IAT directory");
  descriptor->OriginalFirstThunk = 0;
  thunks = ReadImportThunks(context, *descriptor);
  Check(thunks.fallback && thunks.terminated && thunks.entries[0].symbol.hint == 0x1234,
        "FirstThunk fallback lost Hint without IAT directory");
  descriptor->TimeDateStamp = MAXDWORD;
  thunks = ReadImportThunks(context, *descriptor);
  Check(thunks.entries.empty() && !thunks.message.empty(), "Bound IAT was used as missing INT");
  reset();
  directories[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = sizeof(IMAGE_IMPORT_DESCRIPTOR);
  imports = ReadImportDetails(context);
  Check(!imports.terminated && !imports.message.empty() && imports.modules.size() == 1,
        "Missing descriptor terminator was not reported");
  descriptor->Name = MAXDWORD;
  imports = ReadImportDetails(context);
  Check(imports.modules.size() == 1 && imports.modules[0].descriptorIndex == 0 &&
        !imports.modules[0].message.empty(), "Invalid DLL name discarded descriptor identity");
  directories[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = MAXDWORD;
  imports = ReadImportDetails(context);
  Check(imports.present && imports.modules.empty() && !imports.message.empty(), "Invalid import range accepted");

  reset();
  auto exports = ReadExportDetails(context);
  Check(exports.present && exports.hasHeader && exports.message.empty() && exports.entries.size() == 1 &&
        exports.entries[0].ordinal == 1 && exports.entries[0].name == "ExportedFunc" &&
        exports.entries[0].fileOffset == 0xA00, "Export model mismatch");
  auto exportHeader = (PIMAGE_EXPORT_DIRECTORY)(image.data() + 0x300);
  exportHeader->Base = 0x100;
  exportHeader->NumberOfNames = 0;
  exports = ReadExportDetails(context);
  Check(exports.entries[0].ordinal == 0x100 && exports.entries[0].name.empty(), "Unnamed export lost Base");
  reset();
  *(WORD*)(image.data() + 0x348) = 1;
  exports = ReadExportDetails(context);
  Check(exports.entries.empty() && !exports.message.empty(), "Out-of-range export ordinal accepted");
  reset();
  exportHeader->AddressOfFunctions = MAXDWORD;
  Check(!ReadExportDetails(context).message.empty(), "Invalid export array accepted");
  reset();
  exportHeader->NumberOfFunctions = 2;
  exportHeader->NumberOfNames = 0;
  exportHeader->Base = MAXDWORD;
  *(DWORD*)(image.data() + 0x344) = 0x1804;
  exports = ReadExportDetails(context);
  Check(exports.entries.size() == 1 && !exports.message.empty(), "Export ordinal overflow was not reported");
  reset();
  directories[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = sizeof(IMAGE_EXPORT_DIRECTORY) - 1;
  Check(!ReadExportDetails(context).hasHeader, "Truncated export header accepted");

  reset();
  auto bound = ReadBoundImportDetails(context);
  Check(bound.present && bound.terminated && bound.message.empty() && bound.entries.size() == 1 &&
        bound.entries[0].name == "bound.dll", "Bound import model mismatch");
  directories[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0x60;
  std::memset(image.data() + 0x500, 0, 0x60);
  auto parent = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)(image.data() + 0x500);
  parent->TimeDateStamp = 0x11223344;
  parent->OffsetModuleName = 0x20;
  parent->NumberOfModuleForwarderRefs = 1;
  auto forwarder = (PIMAGE_BOUND_FORWARDER_REF)(image.data() + 0x508);
  forwarder->TimeDateStamp = 0x55667788;
  forwarder->OffsetModuleName = 0x30;
  std::memcpy(image.data() + 0x520, "parent.dll", 11);
  std::memcpy(image.data() + 0x530, "child.dll", 10);
  bound = ReadBoundImportDetails(context);
  Check(bound.terminated && bound.message.empty() && bound.entries.size() == 1 &&
        bound.entries[0].forwarders.size() == 1 && bound.entries[0].forwarders[0].name == "child.dll" &&
        bound.entries[0].forwarders[0].descriptor.TimeDateStamp == 0x55667788,
        "Bound forwarder record was skipped or treated as a main descriptor");
  std::memset(image.data() + 0x530, 'X', 0x30);
  bound = ReadBoundImportDetails(context);
  Check(!bound.message.empty() && !bound.entries[0].forwarders[0].message.empty(),
        "Unterminated bound forwarder name accepted");
  parent->NumberOfModuleForwarderRefs = 0xFFFF;
  Check(!ReadBoundImportDetails(context).message.empty(), "Truncated bound forwarder array accepted");

  reset();
  auto debug = ReadDebugDetails(context);
  Check(debug.present && debug.message.empty() && debug.entries.size() == 1 &&
        debug.entries[0].Type == IMAGE_DEBUG_TYPE_CODEVIEW && debug.entries[0].PointerToRawData == 0x650,
        "Debug model mismatch");
  ((PIMAGE_DEBUG_DIRECTORY)(image.data() + 0x600))->Type = IMAGE_DEBUG_TYPE_REPRO;
  Check(ReadDebugDetails(context).entries[0].Type == IMAGE_DEBUG_TYPE_REPRO, "Raw debug type was reformatted in PE layer");
  directories[IMAGE_DIRECTORY_ENTRY_DEBUG].Size += 1;
  debug = ReadDebugDetails(context);
  Check(debug.entries.size() == 1 && !debug.message.empty(), "Truncated debug record was not reported");
  directories[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = MAXDWORD;
  Check(ReadDebugDetails(context).entries.empty(), "Invalid debug range accepted");
  directories[IMAGE_DIRECTORY_ENTRY_IMPORT] = {};
  directories[IMAGE_DIRECTORY_ENTRY_EXPORT] = {};
  directories[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT] = {};
  directories[IMAGE_DIRECTORY_ENTRY_DEBUG] = {};
  Check(!ReadImportDetails(context).present && !ReadExportDetails(context).present &&
        !ReadBoundImportDetails(context).present && !ReadDebugDetails(context).present,
        "Missing directory was treated as malformed present data");
}
void CheckExportForwarders(bool wide) {
  std::vector<BYTE> image(pe32_fixture::kFileSize);
  PEContext context;
  auto reset = [&]() {
    if (wide) pe64_fixture::Fill(image.data()); else pe32_fixture::Fill(image.data());
    pe32_fixture::FillForwardedExports(image.data());
    Check(context.Initialize(image.data(), image.size()), "Forwarded-export fixture rejected");
    auto directories = const_cast<IMAGE_DATA_DIRECTORY*>(context.dataDirectories);
    directories[IMAGE_DIRECTORY_ENTRY_EXPORT].Size = pe32_fixture::kForwardedExportSize;
  };
  reset();
  auto directory = const_cast<IMAGE_DATA_DIRECTORY*>(context.Directory(IMAGE_DIRECTORY_ENTRY_EXPORT));
  auto header = (PIMAGE_EXPORT_DIRECTORY)(image.data() + 0x300);
  auto functions = (DWORD*)(image.data() + 0x340);
  auto exports = ReadExportDetails(context);
  Check(exports.message.empty() && exports.entries.size() == 3,
        "Mixed export/forwarder list or empty EAT slot handling failed");
  Check(!exports.entries[0].isForwarder && exports.entries[0].rva == 0x1800 &&
        exports.entries[0].forwarder.empty(), "Ordinary export was decoded as a forwarder");
  Check(exports.entries[1].isForwarder && exports.entries[1].rva == 0x1190 &&
        exports.entries[1].ordinal == 2 && exports.entries[1].fileOffset == 0x390 &&
        exports.entries[1].name == "ForwardedName" && exports.entries[1].forwarder == "MYDLL.expfunc",
        "Named forwarder lost its raw address, identity or target");
  Check(exports.entries[2].isForwarder && exports.entries[2].forwarder == "MYDLL.#27",
        "Ordinal forwarder was not preserved");

  header->NumberOfNames = 0;
  header->Base = 0x100;
  exports = ReadExportDetails(context);
  Check(exports.entries[1].name.empty() && exports.entries[1].ordinal == 0x101 &&
        exports.entries[1].forwarder == "MYDLL.expfunc", "Unnamed forwarder lost its ordinal or target");

  // The range comes from DataDirectory, rather than the EAT or the containing section.
  reset();
  functions[0] = 0x1200;
  Check(!ReadExportDetails(context).entries[0].isForwarder, "Export range upper bound is not exclusive");
  functions[0] = 0x10FF;
  Check(!ReadExportDetails(context).entries[0].isForwarder, "RVA below the export range became a forwarder");
  functions[0] = directory->VirtualAddress;
  std::memcpy(&header->Characteristics, "M.X", sizeof("M.X"));
  exports = ReadExportDetails(context);
  Check(exports.entries[0].isForwarder && exports.entries[0].forwarder == "M.X",
        "Export range lower bound is not inclusive");

  reset();
  functions[0] = 0x11FC;
  std::memcpy(image.data() + 0x3FC, "M.X", sizeof("M.X"));
  exports = ReadExportDetails(context);
  Check(exports.message.empty() && exports.entries[0].forwarder == "M.X",
        "Forwarder ending at the directory boundary was rejected");
  std::memset(image.data() + 0x3FC, 'X', 4);
  image[0x400] = 0;  // A null outside the export directory must not terminate the string.
  exports = ReadExportDetails(context);
  Check(exports.entries.size() == 3 && exports.entries[0].isForwarder &&
        exports.entries[0].forwarder.empty() && !exports.entries[0].message.empty() &&
        !exports.message.empty() && exports.entries[2].forwarder == "MYDLL.#27",
        "Unterminated forwarder escaped its directory or discarded subsequent exports");
  image[0x3FC] = 0;
  Check(!ReadExportDetails(context).entries[0].message.empty(), "Empty forwarder target was accepted");

  reset();
  directory->Size = 0x1000;
  functions[0] = 0x1FFF;
  image.back() = 'X';
  exports = ReadExportDetails(context);
  Check(exports.entries[0].isForwarder && exports.entries[0].hasFileOffset &&
        !exports.entries[0].message.empty(), "Forwarder crossed the file-backed range");
  functions[0] = 0x2000;
  exports = ReadExportDetails(context);
  Check(exports.entries[0].isForwarder && !exports.entries[0].hasFileOffset &&
        !exports.entries[0].message.empty(), "Unmapped forwarder target was read");
  directory->Size = MAXDWORD;
  exports = ReadExportDetails(context);
  Check(exports.hasHeader && exports.entries.empty() && !exports.message.empty(),
        "Export directory RVA range overflow was not reported");
}
}  // namespace

void CheckDirectoryDetails() {
  CheckFormat(false);
  CheckFormat(true);
  CheckSeparatedDirectories(false);
  CheckSeparatedDirectories(true);
  CheckExportForwarders(false);
  CheckExportForwarders(true);
}
