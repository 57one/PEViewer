#include "pe/directoryDetails.h"
#include "fixtures/load-config.h"

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

const LoadConfigField& Field(const LoadConfigDetails& details, const wchar_t* name) {
  for (const auto& field : details.fields) if (field.name == name) return field;
  throw std::runtime_error("Load Config field missing from model");
}
}  // namespace

void CheckLoadConfigDetails() {
  PEContext empty;
  Check(!ReadLoadConfigDetails(empty).present, "Empty context has Load Config");
  for (bool wide : {false, true}) {
    std::vector<BYTE> image(pe32_fixture::kFileSize);
    PEContext context;
    auto reset = [&]() {
      load_config_fixture::Fill(image.data(), wide);
      Check(context.Initialize(image.data(), image.size()), "Load Config fixture rejected");
    };
    reset();
    auto directory = const_cast<IMAGE_DATA_DIRECTORY*>(context.Directory(IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG));
    auto result = ReadLoadConfigDetails(context);
    Check(result.present && result.hasSize && result.declaredSize == result.knownSize && result.message.empty(),
          "Load Config size/status mismatch");
    const auto cookie = Field(result, L"SecurityCookie");
    Check(cookie.available && cookie.size == (wide ? 8 : 4) && cookie.hasRva && cookie.rva == 0x1D00 &&
          cookie.hasFileOffset && cookie.fileOffset == 0xF00 &&
          cookie.value == (wide ? pe64_fixture::kImageBase : pe32_fixture::kImageBase) + 0x1D00,
          "Load Config VA width/conversion incorrect");
    Check(Field(result, L"ProcessHeapFlags").value == 0x400 &&
          Field(result, L"ProcessAffinityMask").value == (wide ? 0x100000002ULL : 0x1234) &&
          Field(result, L"CodeIntegrity.Flags").value == 3 &&
          Field(result, L"CodeIntegrity.CatalogOffset").value == 0x123456 &&
          Field(result, L"GuardMemcpyFunctionPointer").rva == 0x1D30,
          "Load Config member offsets/order or CodeIntegrity layout incorrect");
    if (wide) {
      Check(Field(result, L"DeCommitFreeBlockThreshold").value == 0x200001234ULL &&
            !result.safeSeh.applicable && result.safeSeh.entries.empty(), "PE32+ values truncated or SafeSEH decoded");
    } else {
      Check(result.safeSeh.applicable && result.safeSeh.hasFields && result.safeSeh.complete &&
            result.safeSeh.entries.size() == 3 && result.safeSeh.message.empty(), "SafeSEH table missing");
      const auto& entry = result.safeSeh.entries[1];
      Check(entry.slotRva == 0x1C04 && entry.slotFileOffset == 0xE04 && entry.handlerRva == 0x1810 &&
            entry.hasVa && entry.handlerVa == 0x401810 && entry.hasFileOffset && entry.fileOffset == 0xA10,
            "SafeSEH counted DWORD RVAs confused with VAs/pointers");
    }

    // An old 64-byte version is valid: unavailable extension fields are not zero values.
    DWORD size = 64;
    std::memcpy(image.data() + load_config_fixture::kRaw, &size, 4);
    result = ReadLoadConfigDetails(context);
    Check(result.message.empty() && !Field(result, L"GuardFlags").available &&
          Field(result, L"GuardFlags").message.find(L"version") != std::wstring::npos &&
          result.safeSeh.entries.empty(), "Old Load Config version treated as a full current header");
    // DataDirectory may contain more bytes than the version declares.
    Check(!Field(result, L"GuardMemcpyFunctionPointer").available, "Bytes after structure Size were interpreted");
    reset();
    const DWORD cookieOffset = wide ? offsetof(IMAGE_LOAD_CONFIG_DIRECTORY64, SecurityCookie) :
                                    offsetof(IMAGE_LOAD_CONFIG_DIRECTORY32, SecurityCookie);
    size = cookieOffset + 1;
    std::memcpy(image.data() + load_config_fixture::kRaw, &size, 4);
    result = ReadLoadConfigDetails(context);
    Check(!Field(result, L"SecurityCookie").available && !result.message.empty(), "Partial member read across structure Size");
    reset();
    directory->Size = cookieOffset + 1;
    result = ReadLoadConfigDetails(context);
    Check(!Field(result, L"SecurityCookie").available && Field(result, L"TimeDateStamp").available &&
          !result.message.empty(), "Directory truncation lost readable prefix or overread member");
    reset();
    size = result.knownSize + 32;
    std::memcpy(image.data() + load_config_fixture::kRaw, &size, 4);
    directory->Size = size;
    result = ReadLoadConfigDetails(context);
    Check(Field(result, L"GuardMemcpyFunctionPointer").available &&
          result.message.find(L"extension") != std::wstring::npos, "Newer version lost known fields");
    for (DWORD invalid : {0UL, 1UL, 3UL}) {
      std::memcpy(image.data() + load_config_fixture::kRaw, &invalid, 4);
      Check(ReadLoadConfigDetails(context).fields.empty(), "Invalid structure Size accepted");
    }
    reset();
    directory->Size = 3;
    Check(!ReadLoadConfigDetails(context).hasSize, "Truncated Size field read");
    directory->VirtualAddress = MAXDWORD - 1;
    directory->Size = 8;
    Check(!ReadLoadConfigDetails(context).hasSize, "Directory RVA overflow accepted");
    directory->VirtualAddress = 0x3000;
    Check(!ReadLoadConfigDetails(context).hasSize, "Unmapped directory dereferenced");
    reset();
    // Unaligned headers must be read with memcpy, not structure pointers.
    std::memmove(image.data() + load_config_fixture::kRaw + 1, image.data() + load_config_fixture::kRaw,
                 directory->Size);
    ++directory->VirtualAddress;
    Check(Field(ReadLoadConfigDetails(context), L"SecurityCookie").value == cookie.value,
          "Unaligned Load Config field read incorrectly");
    reset();
    DWORD cookieSize = wide ? 8 : 4;
    ULONGLONG badVa = 1;
    std::memcpy(image.data() + load_config_fixture::kRaw + cookieOffset, &badVa, cookieSize);
    result = ReadLoadConfigDetails(context);
    Check(Field(result, L"SecurityCookie").available && !Field(result, L"SecurityCookie").hasRva &&
          !Field(result, L"SecurityCookie").message.empty(), "Invalid VA discarded or wrapped");
    reset();
    // A readable Size at EOF does not make later fields readable.
    size = wide ? sizeof(IMAGE_LOAD_CONFIG_DIRECTORY64) : sizeof(IMAGE_LOAD_CONFIG_DIRECTORY32);
    std::memcpy(image.data() + image.size() - 4, &size, 4);
    directory->VirtualAddress = 0x1FFC;
    result = ReadLoadConfigDetails(context);
    Check(result.hasSize && Field(result, L"Size").available && !Field(result, L"TimeDateStamp").available,
          "File-tail field overread");
    *directory = {};
    Check(!ReadLoadConfigDetails(context).present, "Missing Load Config became present");
  }

  std::vector<BYTE> image(pe32_fixture::kFileSize);
  PEContext context;
  auto reset = [&]() {
    load_config_fixture::Fill(image.data());
    Check(context.Initialize(image.data(), image.size()), "SafeSEH fixture rejected");
  };
  reset();
  auto header = (PIMAGE_LOAD_CONFIG_DIRECTORY32)(image.data() + load_config_fixture::kRaw);
  auto nt = (PIMAGE_NT_HEADERS32)(image.data() + 0x80);
  DWORD handler = 0;
  std::memcpy(image.data() + load_config_fixture::kTableRaw, &handler, 4);
  auto result = ReadLoadConfigDetails(context);
  Check(result.safeSeh.entries.size() == 3 && result.safeSeh.complete &&
        !result.safeSeh.entries[0].message.empty() && result.safeSeh.entries[1].message.empty(),
        "Zero handler treated as terminator or invalid handler discarded valid siblings");
  handler = 0x1810;
  std::memcpy(image.data() + load_config_fixture::kTableRaw, &handler, 4);
  Check(ReadLoadConfigDetails(context).safeSeh.entries[1].message.find(L"sorted") != std::wstring::npos,
        "Duplicate SafeSEH handler not reported");
  handler = MAXDWORD;
  std::memcpy(image.data() + load_config_fixture::kTableRaw, &handler, 4);
  Check(!ReadLoadConfigDetails(context).safeSeh.entries[0].hasFileOffset, "Invalid handler has file offset");
  reset();
  header->SEHandlerTable = 0;
  Check(ReadLoadConfigDetails(context).safeSeh.entries.empty() && !ReadLoadConfigDetails(context).safeSeh.complete,
        "Null table with nonzero count accepted");
  header->SEHandlerCount = 0;
  Check(ReadLoadConfigDetails(context).safeSeh.complete, "Absent optional SafeSEH table rejected");
  reset();
  header->SEHandlerCount = 0;
  Check(!ReadLoadConfigDetails(context).safeSeh.message.empty(), "Table/count mismatch hidden");
  reset();
  header->SEHandlerTable = pe32_fixture::kImageBase - 1;
  Check(ReadLoadConfigDetails(context).safeSeh.entries.empty(), "SafeSEH VA subtraction underflow");
  reset();
  header->SEHandlerTable = pe32_fixture::kImageBase + 0x1FFC;
  std::memcpy(image.data() + image.size() - 4, &handler, 4);
  result = ReadLoadConfigDetails(context);
  Check(result.safeSeh.entries.size() == 1 && !result.safeSeh.complete, "Truncated SafeSEH table lost prefix or overread");
  reset();
  nt->FileHeader.Machine = IMAGE_FILE_MACHINE_ARMNT;
  Check(!ReadLoadConfigDetails(context).safeSeh.applicable && ReadLoadConfigDetails(context).safeSeh.entries.empty(),
        "Non-x86 PE32 interpreted as SafeSEH");
  reset();
  nt->OptionalHeader.SizeOfImage = 0x3000;
  auto section = const_cast<IMAGE_SECTION_HEADER*>(context.sectionHeaders);
  section->Misc.VirtualSize = 0x2000;
  header->SecurityCookie = pe32_fixture::kImageBase + 0x2100;
  header->SEHandlerTable = pe32_fixture::kImageBase + 0x2000;
  result = ReadLoadConfigDetails(context);
  Check(Field(result, L"SecurityCookie").hasRva && !Field(result, L"SecurityCookie").hasFileOffset &&
        result.safeSeh.entries.empty() && !result.safeSeh.complete, "Zero-fill mistaken for file-backed table");

  reset();
  const auto saved = ReadLoadConfigDetails(context);
  std::vector<BYTE> otherImage(pe32_fixture::kFileSize);
  load_config_fixture::Fill(otherImage.data(), true);
  PEContext other;
  Check(other.Initialize(otherImage.data(), otherImage.size()), "Independent Load Config context rejected");
  ReadLoadConfigDetails(other);
  Check(saved.safeSeh.entries.size() == 3 && ReadLoadConfigDetails(context).safeSeh.entries.size() == 3,
        "Load Config results share mutable state");

  constexpr DWORD count = 100001;
  image.resize(load_config_fixture::kTableRaw + (SIZE_T)count * 4);
  reset();
  nt = (PIMAGE_NT_HEADERS32)(image.data() + 0x80);
  section = IMAGE_FIRST_SECTION(nt);
  section->SizeOfRawData = (DWORD)image.size() - pe32_fixture::kSectionRaw;
  section->Misc.VirtualSize = section->SizeOfRawData;
  nt->OptionalHeader.SizeOfImage = pe32_fixture::kSectionRva + section->SizeOfRawData;
  header = (PIMAGE_LOAD_CONFIG_DIRECTORY32)(image.data() + load_config_fixture::kRaw);
  header->SEHandlerCount = count;
  for (DWORD i = 0; i < count; ++i) {
    handler = 0x1800 + i;
    std::memcpy(image.data() + load_config_fixture::kTableRaw + (SIZE_T)i * 4, &handler, 4);
  }
  Check(context.Initialize(image.data(), image.size()), "Large SafeSEH fixture rejected");
  result = ReadLoadConfigDetails(context);
  Check(result.safeSeh.entries.size() == 100000 && !result.safeSeh.complete &&
        result.safeSeh.message.find(L"100000") != std::wstring::npos, "SafeSEH count limit bypassed");
}
