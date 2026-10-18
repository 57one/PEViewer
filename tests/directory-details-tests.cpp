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
  Check(first.dll == "test.dll" && first.symbol == "ImportedFunc" && first.hasLookup &&
        first.lookupRva == intRva && first.lookupValue == 0x1270, "INT association mismatch");
  Check(table.entries.back().value == 0 && table.entries.back().note == L"Terminator",
        "IAT terminator was omitted");
  if (wide) {
    Check(table.entries[1].symbol == "Ordinal #42" &&
          table.entries[1].rva == iatRva + 8 && table.entries[2].symbol == "SecondFunc",
          "PE32+ IAT uses incorrect stride or ordinal flag");
  }

  // Bound IAT values are addresses, while the unchanged INT still identifies the import.
  ULONGLONG address = wide ? 0x00007FF812345678ULL : 0x76543210ULL;
  Store(image.data() + iatRaw, address, width);
  descriptor->TimeDateStamp = 0x12345678;
  table = ReadIatDetails(context);
  Check(table.entries[0].value == address && table.entries[0].symbol == "ImportedFunc" &&
        table.entries[0].lookupValue == 0x1270, "Bound IAT address was treated as a name RVA");
  descriptor->OriginalFirstThunk = 0;
  table = ReadIatDetails(context);
  Check(!table.entries[0].hasLookup && table.entries[0].symbol.empty() &&
        table.entries[0].dll == "test.dll" && !table.entries[0].note.empty(),
        "Bound IAT without INT invented a function name");

  reset();
  descriptor->OriginalFirstThunk = 0;
  table = ReadIatDetails(context);
  Check(table.entries[0].symbol == "ImportedFunc" && table.entries[0].lookupRva == iatRva,
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
  Check(table.entries[0].symbol.empty() && !table.entries[0].hasLookup &&
        !table.entries[0].note.empty(), "Truncated INT slot was read");
  reset();
  Store(image.data() + intRaw, wide ? 0x0000000100001270ULL : 0x70001270ULL, width);
  table = ReadIatDetails(context);
  Check(table.entries[0].symbol.empty() && !table.entries[0].note.empty(),
        "Invalid/bound lookup value was narrowed to a valid name RVA");
  Store(image.data() + intRaw, wide ? 0x800000000001002AULL : 0x8001002AULL, width);
  table = ReadIatDetails(context);
  Check(table.entries[0].symbol.empty() && !table.entries[0].note.empty(),
        "Reserved ordinal bits were accepted");
  Store(image.data() + intRaw, 0x1FFF, width);
  table = ReadIatDetails(context);
  Check(table.entries[0].symbol.empty() && !table.entries[0].note.empty(),
        "Invalid import-by-name record was accepted");
  reset();
  Store(image.data() + intRaw, 0x1900, width);
  std::memset(image.data() + 0xB02, 'X', image.size() - 0xB02);
  table = ReadIatDetails(context);
  Check(table.entries[0].symbol.empty() && !table.entries[0].note.empty(),
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
        table.entries[2].symbol == "Ordinal #65535", "Multi-DLL IAT mapping failed");
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
}  // namespace

void CheckDirectoryDetails() {
  CheckFormat(false);
  CheckFormat(true);
}
