#include "pe/directoryDetails.h"
#include "fixtures/delay-imports.h"

#include <stdexcept>
#include <vector>

namespace {
void Check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

void CheckFormat(bool wide, bool legacy) {
  std::vector<BYTE> image(pe32_fixture::kFileSize);
  PEContext context;
  auto reset = [&]() {
    delay_fixture::Fill(image.data(), wide, legacy);
    Check(context.Initialize(image.data(), image.size()), "Delay fixture rejected");
  };
  auto descriptors = (PIMAGE_DELAYLOAD_DESCRIPTOR)(image.data() + delay_fixture::kRaw);
  SIZE_T width = wide ? 8 : 4;
  ULONGLONG flag = wide ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32;
  ULONGLONG base = wide ? (legacy ? 0x10000000ULL : pe64_fixture::kImageBase) : pe32_fixture::kImageBase;
  auto address = [&](DWORD rva) { return (DWORD)(legacy ? base + rva : rva); };
  auto read = [&]() {
    auto modules = ReadDelayImportDetails(context);
    Check(!modules.modules.empty(), "Delay descriptor unexpectedly missing");
    return ReadDelayImportThunks(context, modules.modules[0]);
  };
  reset();
  auto directory = const_cast<IMAGE_DATA_DIRECTORY*>(context.Directory(IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT));
  auto modules = ReadDelayImportDetails(context);
  Check(modules.present && modules.terminated && modules.message.empty() && modules.modules.size() == 2,
        "Delay descriptor count, termination or status mismatch");
  const auto& first = modules.modules[0];
  Check(first.name == "delay-test.dll" && first.rvaBased == !legacy && first.supported &&
        first.descriptorRva == delay_fixture::kRva && first.message.empty() &&
        first.lookup.rva == delay_fixture::kLookupRva && first.iat.rva == delay_fixture::kIatRva &&
        first.lookup.fileOffset == delay_fixture::Raw(delay_fixture::kLookupRva),
        "Delay descriptor fields or VA conversion mismatch");
  auto functions = read();
  Check(functions.terminated && functions.message.empty() && functions.entries.size() == 3,
        "Delay lookup termination or count mismatch");
  Check(functions.entries[0].symbol.kind == ImportKind::Name && functions.entries[0].symbol.name == "DelayedFunc" &&
        functions.entries[0].symbol.hint == 0x1234 && functions.entries[1].symbol.kind == ImportKind::Ordinal &&
        functions.entries[1].symbol.ordinal == 42 && functions.entries[2].symbol.name == "SecondDelayFunc" &&
        functions.entries[2].symbol.hint == 0x5678, "Delay shared name/ordinal/Hint decoding failed");
  Check(functions.entries[1].lookup.rva == delay_fixture::kLookupRva + width &&
        functions.entries[1].iat.rva == delay_fixture::kIatRva + width &&
        functions.entries[1].iat.value == base + 0x1810 &&
        functions.entries[0].boundIat.value == (wide ? 0x00007FFB12345678ULL : 0x77001000ULL) &&
        functions.entries[0].unloadIat.value == base + 0x1800,
        "Delay pointer widths, IAT/helper values or optional tables lost data");
  auto other = ReadDelayImportThunks(context, modules.modules[1]);
  Check(other.terminated && other.message.empty() && other.entries.size() == 1 &&
        other.entries[0].symbol.ordinal == 7, "Second delay DLL read wrong thunks");

  *const_cast<IMAGE_DATA_DIRECTORY*>(context.Directory(IMAGE_DIRECTORY_ENTRY_IMPORT)) = {};
  *const_cast<IMAGE_DATA_DIRECTORY*>(context.Directory(IMAGE_DIRECTORY_ENTRY_IAT)) = {};
  Check(read().entries.size() == 3, "Delay imports depend on ordinary Import/IAT directories");

  reset();
  descriptors[0].ImportNameTableRVA = 0;
  functions = read();
  Check(!functions.terminated && functions.entries.empty() && !functions.message.empty(),
        "Delay helper addresses were used as an INT fallback");
  reset();
  descriptors[0].ImportAddressTableRVA = 0;
  functions = read();
  Check(functions.terminated && functions.entries.size() == 3 && !functions.message.empty() &&
        !functions.entries[0].iat.hasValue && functions.entries[0].symbol.name == "DelayedFunc",
        "Missing delay IAT discarded independently readable INT names");

  reset();
  delay_fixture::Value(image.data(), delay_fixture::kLookupRva, flag | 0x10000 | 3, wide);
  functions = read();
  Check(functions.terminated && functions.entries.size() == 3 &&
        functions.entries[0].symbol.kind == ImportKind::Invalid && functions.entries[2].symbol.name == "SecondDelayFunc",
        "Invalid delay ordinal lost its row or valid siblings");
  delay_fixture::Value(image.data(), delay_fixture::kLookupRva, legacy ? base - 1 : 0x7FFFFFFF, wide);
  Check(read().entries[0].symbol.kind == ImportKind::Invalid, "Invalid delay name address accepted");
  reset();
  delay_fixture::Value(image.data(), delay_fixture::kLookupRva, legacy ? base + 0x1FFD : 0x1FFD, wide);
  std::memset(image.data() + delay_fixture::Raw(0x1FFD), 'X', 3);
  Check(read().entries[0].symbol.kind == ImportKind::Invalid, "Unterminated delay function name accepted");

  reset();
  delay_fixture::Value(image.data(), delay_fixture::kIatRva, 0, wide);
  Check(!read().entries[0].message.empty(), "Early delay IAT terminator not reported");
  reset();
  delay_fixture::Value(image.data(), delay_fixture::kIatRva + (DWORD)width * 3, 1, wide);
  Check(read().message.find(L"IAT terminator") != std::wstring::npos, "Mismatched delay INT/IAT terminators accepted");
  reset();
  descriptors[0].BoundImportAddressTableRVA = address(0x1FFC);
  Check(!read().message.empty(), "Truncated bound delay table accepted");
  reset();
  descriptors[0].BoundImportAddressTableRVA = descriptors[0].UnloadInformationTableRVA = 0;
  functions = read();
  Check(functions.terminated && functions.message.empty() && !functions.entries[0].boundIat.hasValue,
        "Absent optional delay tables treated as errors");

  reset();
  descriptors[0].ImportNameTableRVA = address(0x2000 - (DWORD)width * 2);
  delay_fixture::Value(image.data(), 0x2000 - (DWORD)width * 2, flag | 1, wide);
  delay_fixture::Value(image.data(), 0x2000 - (DWORD)width, flag | 2, wide);
  functions = read();
  Check(!functions.terminated && functions.entries.size() == 2 &&
        functions.message.find(L"Truncated delay INT") != std::wstring::npos, "Unterminated delay INT read past file end");

  reset();
  descriptors[0].Attributes.AllAttributes |= 2;
  modules = ReadDelayImportDetails(context);
  Check(modules.terminated && modules.modules.size() == 2 && !modules.modules[0].supported &&
        modules.modules[1].name == "other-delay.dll" && read().entries.empty(),
        "Unknown delay attributes decoded pointers or lost the next DLL");
  reset();
  descriptors[0].DllNameRVA = address(0x1FFF);
  image.back() = 'X';
  Check(ReadDelayImportDetails(context).modules[0].name.empty() && read().entries.size() == 3,
        "Bad DLL name hid readable delay thunks");
  reset();
  descriptors[0].DllNameRVA = address(0x1A80);
  Check(!ReadDelayImportDetails(context).modules[0].message.empty() && read().entries.size() == 3,
        "Empty delay DLL name accepted or hid readable thunks");
  reset();
  descriptors[0].ModuleHandleRVA = address(0x2000);
  auto section = const_cast<IMAGE_SECTION_HEADER*>(context.sectionHeaders);
  section->Misc.VirtualSize = 0x2000;
  if (wide) ((PIMAGE_NT_HEADERS64)(image.data() + 0x80))->OptionalHeader.SizeOfImage = 0x3000;
  else ((PIMAGE_NT_HEADERS32)(image.data() + 0x80))->OptionalHeader.SizeOfImage = 0x3000;
  modules = ReadDelayImportDetails(context);
  Check(modules.modules[0].moduleHandle.hasRva && !modules.modules[0].moduleHandle.hasFileOffset &&
        modules.modules[0].message.empty(), "Delay HMODULE in zero-fill memory rejected");

  reset();
  directory->Size = 64;
  Check(!ReadDelayImportDetails(context).terminated, "Missing delay descriptor terminator accepted");
  directory->Size = 31;
  modules = ReadDelayImportDetails(context);
  Check(modules.present && !modules.terminated && modules.modules.empty() && !modules.message.empty(),
        "Partial delay descriptor accepted");
  directory->Size = 96;
  directory->VirtualAddress = MAXDWORD - 16;
  Check(!ReadDelayImportDetails(context).message.empty(), "Delay directory RVA overflow accepted");
  directory->VirtualAddress = 0x3000;
  Check(ReadDelayImportDetails(context).modules.empty(), "Unmapped delay descriptors read");
  *directory = {};
  Check(!ReadDelayImportDetails(context).present, "Missing delay directory became present");

  if (wide && legacy) {
    reset();
    ((PIMAGE_NT_HEADERS64)(image.data() + 0x80))->OptionalHeader.ImageBase = pe64_fixture::kImageBase;
    modules = ReadDelayImportDetails(context);
    Check(!modules.modules[0].lookup.hasRva && !modules.modules[0].message.empty(),
          "Legacy DWORD VA descriptor silently truncated a high ImageBase");
  }
  if (!wide && legacy) {
    reset();
    descriptors[0].ModuleHandleRVA = MAXDWORD - 1;
    ((PIMAGE_NT_HEADERS32)(image.data() + 0x80))->OptionalHeader.SizeOfImage = MAXDWORD;
    modules = ReadDelayImportDetails(context);
    Check(!modules.modules[0].moduleHandle.hasRva &&
          modules.modules[0].moduleHandle.message.find(L"overflows") != std::wstring::npos,
          "Legacy VA target range wrapped past the 32-bit address limit");
  }
}
}  // namespace

void CheckDelayImportDetails() {
  PEContext empty;
  Check(!ReadDelayImportDetails(empty).present, "Empty context has delay imports");
  for (bool wide : {false, true}) for (bool legacy : {false, true}) CheckFormat(wide, legacy);

  // Alternate independent contexts and retain the earlier returned model.
  std::vector<BYTE> narrow(pe32_fixture::kFileSize), wide(pe32_fixture::kFileSize);
  delay_fixture::Fill(narrow.data(), false);
  delay_fixture::Fill(wide.data(), true);
  PEContext first, second;
  Check(first.Initialize(narrow.data(), narrow.size()) && second.Initialize(wide.data(), wide.size()),
        "Delay isolation fixtures rejected");
  const auto original = ReadDelayImportDetails(first);
  const auto next = ReadDelayImportDetails(second);
  Check(ReadDelayImportThunks(second, next.modules[0]).entries[0].iat.value == pe64_fixture::kImageBase + 0x1800 &&
        ReadDelayImportThunks(first, original.modules[0]).entries[0].iat.value == pe32_fixture::kImageBase + 0x1800,
        "Delay parser leaked format or context state");

  constexpr DWORD count = 100001, bytes = (count + 1) * 8;
  std::vector<BYTE> image(0x1200 + bytes * 2);
  delay_fixture::Fill(image.data(), true);
  auto nt = (PIMAGE_NT_HEADERS64)(image.data() + 0x80);
  auto section = IMAGE_FIRST_SECTION(nt);
  section->SizeOfRawData = (DWORD)image.size() - pe32_fixture::kSectionRaw;
  section->Misc.VirtualSize = section->SizeOfRawData;
  nt->OptionalHeader.SizeOfImage = pe32_fixture::kSectionRva + section->SizeOfRawData;
  auto descriptors = (PIMAGE_DELAYLOAD_DESCRIPTOR)(image.data() + delay_fixture::kRaw);
  descriptors[0].ImportNameTableRVA = 0x2000;
  descriptors[0].ImportAddressTableRVA = 0x2000 + bytes;
  descriptors[0].BoundImportAddressTableRVA = descriptors[0].UnloadInformationTableRVA = 0;
  for (DWORD i = 0; i < count; ++i) {
    delay_fixture::Value(image.data(), 0x2000 + i * 8, IMAGE_ORDINAL_FLAG64 | 42, true);
    delay_fixture::Value(image.data(), 0x2000 + bytes + i * 8, pe64_fixture::kImageBase + 0x1800, true);
  }
  PEContext context;
  Check(context.Initialize(image.data(), image.size()), "Large delay fixture rejected");
  auto modules = ReadDelayImportDetails(context);
  auto functions = ReadDelayImportThunks(context, modules.modules[0]);
  Check(!functions.terminated && functions.entries.size() == 100000 &&
        functions.message.find(L"100000") != std::wstring::npos, "Delay function limit bypassed");
  delay_fixture::Value(image.data(), 0x2000 + 100000 * 8, 0, true);
  delay_fixture::Value(image.data(), 0x2000 + bytes + 100000 * 8, 0, true);
  functions = ReadDelayImportThunks(context, modules.modules[0]);
  Check(functions.terminated && functions.message.empty() && functions.entries.size() == 100000,
        "Delay terminator exactly at display limit rejected");
}
